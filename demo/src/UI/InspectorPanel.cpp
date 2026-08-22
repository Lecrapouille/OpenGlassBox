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

    if (!ImGui::BeginTable("rules", 3,
                           ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                           ImGuiTableFlags_SizingStretchProp))
        return;

    ImGui::TableSetupColumn("rule");
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
        ImGui::TextDisabled("Click a unit, an agent, a road, a node or a cell\n"
                            "on the map to inspect it.");
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
                       "Unit %s", unit->type().c_str());
    ImGui::TextDisabled("city %s, cell (%d, %d), radius %u",
                        state.selection.city.c_str(), unit->mapU(), unit->mapV(),
                        unit->mapRadius());

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
        ImGui::SeparatorText("Current road");
        ImGui::Text("%s, %.0f%% travelled", way->type().c_str(),
                    100.0f * agent->offset());
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
