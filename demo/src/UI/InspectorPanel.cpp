//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "UI/Panels.hpp"
#include "UI/Theme.hpp"
#include "Game/RuleTrace.hpp"
#include "OpenGlassBox/Simulation.hpp"

#include <algorithm>
#include <map>
#include <string>
#include <vector>

namespace ogb {
namespace ui {
using namespace ogb::theme;


// ----------------------------------------------------------------------------
//! \brief Draw one resource as a labelled progress bar, which reads much faster
//! than a number when scanning a list.
// ----------------------------------------------------------------------------
static void drawResource(Resource const& resource)
{
    uint32_t const capacity = std::max(1u, resource.getCapacity());
    float const ratio = std::min(1.0f, float(resource.getAmount()) / float(capacity));

    char overlay[64];
    std::snprintf(overlay, sizeof(overlay), "%u / %u", resource.getAmount(),
                  resource.getCapacity());

    ImGui::Text("%s", resource.type().c_str());
    ImGui::SameLine(120.0f);
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::ProgressBar(ratio, ImVec2(-1.0f, 0.0f), overlay);
}

// ----------------------------------------------------------------------------
static void drawResources(char const* label, std::vector<Resource> const& bin)
{
    if (bin.empty())
    {
        ImGui::TextDisabled("%s: none", label);
        return;
    }

    ImGui::SeparatorText(label);
    for (Resource const& resource: bin)
    {
        drawResource(resource);
    }
}

// ----------------------------------------------------------------------------
//! \brief List the rules of an entity with their period and the number of ticks
//! left before the next attempt. A rule that never fires with many ticks to go
//! is simply not due yet; one that is due every tick and still does nothing is
//! blocked, and the Rule Log says by what.
// ----------------------------------------------------------------------------
template<typename RuleContainer>
static void drawRules(RuleContainer const& rules, uint32_t ticks)
{
    if (rules.empty())
    {
        ImGui::TextDisabled("no rule");
        return;
    }

    if (!ImGui::BeginTable("rules", 4,
                           ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                           ImGuiTableFlags_SizingStretchProp))
        return;

    ImGui::TableSetupColumn("rule");
    ImGui::TableSetupColumn("does");
    ImGui::TableSetupColumn("every");
    ImGui::TableSetupColumn("in");
    ImGui::TableHeadersRow();

    for (auto const* rule: rules)
    {
        if (rule == nullptr)
            continue;

        uint32_t const rate = std::max(1u, rule->rate());
        // executeRules() increments the counter first, so the rule fires when
        // (ticks + n) % rate == 0.
        uint32_t const remaining = (rate - ((ticks + 1u) % rate)) % rate;

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(rule->type().c_str());

        // The name a script gives a rule says nothing about what it does, so
        // spell out the commands it runs, in order.
        ImGui::TableNextColumn();
        std::string commands;
        for (IRuleCommand* command: rule->commands())
        {
            if (command == nullptr)
                continue;
            if (!commands.empty())
                commands += ", ";
            commands += command->type();
        }
        if (commands.empty())
            ImGui::TextDisabled("nothing");
        else
            ImGui::TextWrapped("%s", commands.c_str());

        ImGui::TableNextColumn();
        ImGui::Text("%u tick%s", rate, (rate > 1u) ? "s" : "");
        ImGui::TableNextColumn();
        if (remaining == 0u)
        {
            ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(theme::ACCENT),
                               "now");
        }
        else
        {
            ImGui::Text("%u", remaining);
        }
    }

    ImGui::EndTable();
}

// ----------------------------------------------------------------------------
//! \brief One line of an agent listing: what it is, what it carries and how
//! far it still has to go.
// ----------------------------------------------------------------------------
static void drawAgentLine(Agent const& agent)
{
    std::string carried;
    for (Resource const& resource: agent.resources())
    {
        if (resource.getAmount() == 0u)
            continue;
        if (!carried.empty())
            carried += ", ";
        carried += std::to_string(resource.getAmount()) + " " + resource.type();
    }
    if (carried.empty())
        carried = "empty";

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TextColored(
        ImGui::ColorConvertU32ToFloat4(theme::fromScript(agent.color())),
        "%s #%u", agent.type().c_str(), agent.id());
    ImGui::TableNextColumn();
    ImGui::TextUnformatted(carried.c_str());
    ImGui::TableNextColumn();
    ImGui::TextUnformatted(agent.searchTarget().c_str());
    ImGui::TableNextColumn();
    if (agent.currentWay() == nullptr)
        ImGui::TextDisabled("waiting");
    else
        ImGui::Text("%.1f s", agent.remainingCost());
}

// ----------------------------------------------------------------------------
//! \brief The agents this Unit sent out and the agents on their way to it.
//!
//! The engine deletes an Agent the moment it unloads, so there is no such
//! thing as a population sitting inside a building. What a building does have
//! is traffic: what left it and what is coming.
// ----------------------------------------------------------------------------
static void drawUnitAgents(City& city, Unit const& unit)
{
    std::vector<Agent const*> outbound;
    std::vector<Agent const*> inbound;

    for (auto const& agent: city.agents())
    {
        if (agent->owner() == &unit)
        {
            outbound.push_back(agent.get());
            continue;
        }
        if (agent->route().destination == &unit)
        {
            inbound.push_back(agent.get());
        }
    }

    ImGui::Text("%zu agent(s) sent out, %zu on their way in", outbound.size(),
                inbound.size());

    if (outbound.empty() && inbound.empty())
    {
        ImGui::TextDisabled("No traffic. Either no rule of this building has\n"
                            "fired yet, or every agent it made has arrived.");
        return;
    }

    ImGuiTableFlags const flags = ImGuiTableFlags_Borders |
                                  ImGuiTableFlags_RowBg |
                                  ImGuiTableFlags_SizingStretchProp;

    auto const table = [&](char const* id, char const* first,
                           std::vector<Agent const*> const& agents) {
        if (agents.empty())
            return;
        if (!ImGui::BeginTable(id, 4, flags))
            return;
        ImGui::TableSetupColumn(first);
        ImGui::TableSetupColumn("carries");
        ImGui::TableSetupColumn("looks for");
        ImGui::TableSetupColumn("left");
        ImGui::TableHeadersRow();
        for (Agent const* agent: agents)
            drawAgentLine(*agent);
        ImGui::EndTable();
    };

    table("outbound", "sent out", outbound);
    table("inbound", "coming in", inbound);
}

// ----------------------------------------------------------------------------
void InspectorPanel::draw(Simulation& simulation, game::DebugState& state,
                          game::RuleTrace const& trace)
{
    if (!ImGui::Begin("Inspector"))
    {
        ImGui::End();
        return;
    }

    switch (state.selection.kind)
    {
    case game::Selection::Kind::None:
        ImGui::TextDisabled("Click a building, an agent, a road, a node, a zone\n"
                            "or a cell on the map to inspect it.");
        break;
    case game::Selection::Kind::Unit:
        drawUnit(simulation, state, trace);
        break;
    case game::Selection::Kind::Agent:
        drawAgent(simulation, state);
        break;
    case game::Selection::Kind::Node:
        drawNode(state);
        break;
    case game::Selection::Kind::Way:
        drawWay(state);
        break;
    case game::Selection::Kind::Area:
        drawArea(simulation, state);
        break;
    case game::Selection::Kind::Cell:
        drawCell(simulation, state);
        break;
    }

    ImGui::End();
}

// ----------------------------------------------------------------------------
void InspectorPanel::drawUnit(Simulation& simulation, game::DebugState& state,
                              game::RuleTrace const& trace)
{
    Unit* const unit = state.selection.unit;
    if (unit == nullptr)
    {
        state.selection.clear();
        return;
    }

    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(theme::fromScript(unit->color())),
                       "Unit %s #%u", unit->type().c_str(), unit->id());
    ImGui::TextDisabled("city %s, cell (%d, %d), radius %u",
                        state.selection.city.c_str(), unit->mapU(), unit->mapV(),
                        unit->mapRadius());

    auto const cityIt = simulation.cities().find(state.selection.city);
    City* const city =
        (cityIt == simulation.cities().end()) ? nullptr : cityIt->second.get();

    if (city != nullptr)
    {
        // Which zone the building stands in decides which area rules may
        // upgrade or demolish it, so it belongs next to its own rules.
        std::string zones;
        for (auto const& area: city->areas())
        {
            if (!area->contains(unit->mapU(), unit->mapV()))
                continue;
            if (!zones.empty())
                zones += ", ";
            zones += area->type();
        }
        if (zones.empty())
            ImGui::TextDisabled("outside any zone");
        else
            ImGui::TextDisabled("in zone %s", zones.c_str());
    }

    if (unit->node() != nullptr)
    {
        ImGui::TextDisabled("on node #%u", unit->node()->id());
    }
    else if (unit->way() != nullptr)
    {
        ImGui::TextDisabled("on %s #%u at %.0f%%", unit->way()->type().c_str(),
                            unit->way()->id(), 100.0f * unit->wayOffset());
    }

    if (!unit->hasWays())
    {
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(theme::FAILURE),
                           "Not connected to any road: the agents it creates\n"
                           "cannot leave, and none can reach it.");
    }

    ImGui::Checkbox("Show the radius on the map", &state.showSelectionRadius);

    drawResources("Resources", unit->resources().container());

    ImGui::SeparatorText("Targets");
    if (unit->targets().empty())
    {
        ImGui::TextDisabled("accepts nothing");
    }
    else
    {
        for (std::string const& target: unit->targets())
        {
            ImGui::BulletText("%s", target.c_str());
        }
    }

    ImGui::SeparatorText("Agents");
    if (city != nullptr)
    {
        drawUnitAgents(*city, *unit);
    }

    ImGui::SeparatorText("Rules");
    drawRules(unit->rules(), unit->ticks());

    // Outcome of the last attempts recorded for this very unit type.
    ImGui::SeparatorText("Last attempts");
    if (!trace.recording())
    {
        ImGui::TextDisabled("Enable the recording in the Rule Log panel\n"
                            "to see why a rule does or does not fire.");
        return;
    }

    std::string const entity = "Unit " + unit->type();
    int shown = 0;
    size_t index = trace.size();
    while ((index-- > 0u) && (shown < 6))
    {
        game::RuleEvent const& event = trace.at(index);
        if (event.entity != entity)
            continue;
        if (event.city != state.selection.city)
            continue;

        ++shown;
        ImU32 const color = event.success ? theme::SUCCESS : theme::FAILURE;
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(color), "%s",
                           event.success ? "ok" : "blocked");
        ImGui::SameLine();
        if (event.success)
        {
            ImGui::Text("%s (tick %llu)", event.rule.c_str(),
                        (unsigned long long)event.tick);
        }
        else
        {
            ImGui::Text("%s by %s (tick %llu)", event.rule.c_str(),
                        event.blockedBy.c_str(), (unsigned long long)event.tick);
        }
    }

    if (shown == 0)
    {
        ImGui::TextDisabled("nothing recorded yet");
    }
}

// ----------------------------------------------------------------------------
void InspectorPanel::drawAgent(Simulation& simulation, game::DebugState& state)
{
    Agent* const agent = state.selection.resolveAgent(simulation);
    if (agent == nullptr)
    {
        ImGui::TextDisabled("The agent has delivered its load and is gone.");
        return;
    }

    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(theme::fromScript(agent->color())),
                       "Agent %s #%u", agent->type().c_str(), agent->id());
    ImGui::TextDisabled("city %s", state.selection.city.c_str());
    ImGui::Text("looking for a unit of type %s", agent->searchTarget().c_str());
    ImGui::Text("speed %.2f world units per second", agent->speed());
    ImGui::Text("position (%.1f, %.1f)", agent->position().x, agent->position().y);

    Way const* const way = agent->currentWay();
    if (way == nullptr)
    {
        ImGui::TextDisabled("waiting on a node for a route");
    }
    else
    {
        ImGui::Text("on %s, %.0f%% travelled", way->type().c_str(),
                    100.0f * agent->offset());
    }

    ImGui::SeparatorText("Itinerary");
    ImGui::Text("remaining cost %.2f s", agent->remainingCost());
    Route const& route = agent->route();
    if (!route.found)
    {
        ImGui::TextDisabled("no cached route");
    }
    else
    {
        ImGui::Text("%zu remaining node(s)", route.nodes.size());
        if (route.destination != nullptr)
        {
            ImGui::Text("destination %s #%u", route.destination->type().c_str(),
                        route.destination->id());
        }
        if (route.approachWay != nullptr)
        {
            ImGui::Text("approach on %s at %.0f%%",
                        route.approachWay->type().c_str(),
                        100.0f * route.approachOffset);
        }
    }

    drawResources("Carried", agent->resources());
}

// ----------------------------------------------------------------------------
void InspectorPanel::drawNode(game::DebugState& state)
{
    Node* const node = state.selection.node;
    if (node == nullptr)
    {
        state.selection.clear();
        return;
    }

    ImGui::Text("Node #%u", node->id());
    ImGui::TextDisabled("city %s", state.selection.city.c_str());
    ImGui::Text("position (%.1f, %.1f)", node->position().x, node->position().y);

    ImGui::SeparatorText("Roads");
    if (node->ways().empty())
    {
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(theme::FAILURE),
                           "orphan node: no agent can reach it");
    }
    for (Way const* way: node->ways())
    {
        ImGui::BulletText("%s, %.1f long, %.2f s, %.0f%% full",
                          way->type().c_str(), way->magnitude(),
                          way->travelTime(), 100.0f * way->saturation());
    }

    ImGui::SeparatorText("Units");
    if (node->units().empty())
    {
        ImGui::TextDisabled("none");
    }
    for (Unit const* unit: node->units())
    {
        ImGui::BulletText("%s", unit->type().c_str());
    }
}

// ----------------------------------------------------------------------------
void InspectorPanel::drawWay(game::DebugState& state)
{
    Way* const way = state.selection.way;
    if (way == nullptr)
    {
        state.selection.clear();
        return;
    }

    ImGui::Text("Way %s #%u", way->type().c_str(), way->id());
    ImGui::TextDisabled("city %s", state.selection.city.c_str());
    ImGui::Text("from node #%u to #%u, %.1f long",
                way->from().id(), way->to().id(), way->magnitude());
    ImGui::Text("free flow %.2f s, now %.2f s", way->freeFlowTime(),
                way->travelTime());

    float const saturation = way->saturation();
    char overlay[64];
    std::snprintf(overlay, sizeof(overlay), "%.0f / %.0f agents",
                  way->flow(), way->capacity());
    ImGui::Text("saturation");
    ImGui::SameLine(120.0f);
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram,
                          ImGui::ColorConvertU32ToFloat4(
                              theme::congestionColor(saturation)));
    ImGui::ProgressBar(std::min(1.0f, saturation), ImVec2(-1.0f, 0.0f), overlay);
    ImGui::PopStyleColor();
}

// ----------------------------------------------------------------------------
void InspectorPanel::drawArea(Simulation& simulation, game::DebugState& state)
{
    Area* const area = state.selection.area;
    if (area == nullptr)
    {
        state.selection.clear();
        return;
    }

    MapRegion const& footprint = area->footprint();

    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(theme::fromScript(area->color())),
                       "Zone %s #%u", area->type().c_str(), area->id());
    ImGui::TextDisabled("city %s, %u x %u cells from (%d, %d)",
                        state.selection.city.c_str(), footprint.sizeU,
                        footprint.sizeV, footprint.u0, footprint.v0);

    std::vector<Unit*> const units = area->unitsInside();

    ImGui::SeparatorText("Buildings inside");
    if (units.empty())
    {
        ImGui::TextDisabled("Empty. An area rule has to spawn something, and\n"
                            "its conditions are listed below.");
    }
    else
    {
        // Grouped by type: what matters about a zone is its mix, not the
        // identity of each building, which the Units themselves report.
        std::map<std::string, uint32_t> counts;
        for (Unit const* unit: units)
            ++counts[unit->type()];

        for (auto const& it: counts)
        {
            ImGui::BulletText("%u x %s", it.second, it.first.c_str());
        }
        ImGui::TextDisabled("%zu building(s) on %llu cell(s)", units.size(),
                            (unsigned long long)footprint.area());
    }

    ImGui::SeparatorText("Rules");
    drawRules(area->rules(), area->ticks());

    ImGui::SeparatorText("Cell under the zone");
    auto const it = simulation.cities().find(state.selection.city);
    if (it == simulation.cities().end())
        return;

    City& city = *it->second;
    ImGui::TextDisabled("cell (%d, %d)", state.selection.u, state.selection.v);
    for (auto& mapIt: city.maps())
    {
        Map& map = *mapIt.second;
        ImGui::TextColored(
            ImGui::ColorConvertU32ToFloat4(theme::fromScript(map.color())),
            "%s", map.type().c_str());
        ImGui::SameLine(120.0f);
        ImGui::Text(": %u / %u",
                    map.getResource(state.selection.u, state.selection.v),
                    map.getCapacity());
    }
}

// ----------------------------------------------------------------------------
void InspectorPanel::drawCell(Simulation& simulation, game::DebugState& state)
{
    auto const it = simulation.cities().find(state.selection.city);
    if (it == simulation.cities().end())
    {
        state.selection.clear();
        return;
    }

    City& city = *it->second;
    ImGui::Text("Cell (%d, %d) of %s", state.selection.u, state.selection.v,
                city.name().c_str());

    ImGui::SeparatorText("Maps");
    for (auto& mapIt: city.maps())
    {
        Map& map = *mapIt.second;
        uint32_t const amount = map.getResource(state.selection.u, state.selection.v);
        uint32_t const capacity = std::max(1u, map.getCapacity());

        char overlay[64];
        std::snprintf(overlay, sizeof(overlay), "%u / %u", amount, map.getCapacity());

        ImGui::TextColored(
            ImGui::ColorConvertU32ToFloat4(theme::fromScript(map.color())),
            "%s", map.type().c_str());
        ImGui::SameLine(120.0f);
        ImGui::ProgressBar(float(amount) / float(capacity), ImVec2(-1.0f, 0.0f),
                           overlay);
    }

    ImGui::SeparatorText("City globals");
    drawResources("Globals", city.globals().container());
}
} // namespace ui
} // namespace ogb
