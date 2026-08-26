//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "Game/RuleTrace.hpp"
#include "OpenGlassBox/Simulation.hpp"
#include "UI/Panels.hpp"
#include "UI/Theme.hpp"

#include <algorithm>
#include <cctype>
#include <cstdarg>
#include <map>
#include <string>
#include <vector>

namespace ogb
{
namespace ui
{
using namespace ogb::theme;

//! \brief Width of the label column of the identity blocks and of the resource
//! bars. Everything the panel writes lines up on it, which is what makes a
//! dense panel readable.
static constexpr float LABEL_WIDTH = 120.0f;

// ----------------------------------------------------------------------------
//! \brief Open a two column block where every label lines up. Returns false
//! when the table could not be opened, in which case field() and endFields()
//! must not be called.
// ----------------------------------------------------------------------------
static bool beginFields(char const* id)
{
    if (!ImGui::BeginTable(id, 2, ImGuiTableFlags_SizingFixedFit))
        return false;

    ImGui::TableSetupColumn(
        "label", ImGuiTableColumnFlags_WidthFixed, LABEL_WIDTH);
    ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch);
    return true;
}

// ----------------------------------------------------------------------------
static void field(char const* label, char const* format, ...)
{
    char value[256];
    va_list args;
    va_start(args, format);
    std::vsnprintf(value, sizeof(value), format, args);
    va_end(args);

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TextDisabled("%s", label);
    ImGui::TableNextColumn();
    ImGui::TextUnformatted(value);
}

// ----------------------------------------------------------------------------
static void endFields()
{
    ImGui::EndTable();
    ImGui::Spacing();
}

// ----------------------------------------------------------------------------
//! \brief Draw one resource as a labelled progress bar, which reads much faster
//! than a number when scanning a list.
// ----------------------------------------------------------------------------
static void drawResource(Resource const& resource)
{
    uint32_t const capacity = std::max(1u, resource.getCapacity());
    float const ratio =
        std::min(1.0f, float(resource.getAmount()) / float(capacity));

    char overlay[64];
    std::snprintf(overlay,
                  sizeof(overlay),
                  "%u / %u",
                  resource.getAmount(),
                  resource.getCapacity());

    ImGui::TextUnformatted(resource.type().c_str());
    ImGui::SameLine(LABEL_WIDTH);
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::ProgressBar(ratio, ImVec2(-1.0f, 0.0f), overlay);
}

// ----------------------------------------------------------------------------
static void drawResources(char const* label, std::vector<Resource> const& bin)
{
    ImGui::SeparatorText(label);
    if (bin.empty())
    {
        ImGui::TextDisabled("none");
        return;
    }

    for (Resource const& resource : bin)
    {
        drawResource(resource);
    }
    ImGui::Spacing();
}

// ----------------------------------------------------------------------------
//! \brief Spell a number of ticks out as game time. A tick means nothing to a
//! reader who does not know how many of them make a minute, and how many that
//! is depends on SimulationConfig::ticksPerMinute.
// ----------------------------------------------------------------------------
static std::string gameTimeText(uint32_t ticks, uint32_t ticksPerMinute)
{
    uint32_t const perMinute = std::max(1u, ticksPerMinute);
    uint32_t const minutes = ticks / perMinute;
    if (minutes == 0u)
        return "under a game minute";

    uint32_t const days = minutes / (60u * 24u);
    uint32_t const hours = (minutes / 60u) % 24u;
    uint32_t const rest = minutes % 60u;

    std::string out;
    auto append = [&out](uint32_t value, char const* unit)
    {
        if (value == 0u)
            return;
        if (!out.empty())
            out += " ";
        out += std::to_string(value);
        out += " ";
        out += unit;
        if ((value > 1u) && (unit[0] == 'd'))
            out += "s";
    };
    append(days, "day");
    append(hours, "h");
    append(rest, "min");
    return out;
}

// ----------------------------------------------------------------------------
//! \brief The commands a rule runs, in the order the script wrote them. The
//! name a script gives a rule says nothing about what it does.
// ----------------------------------------------------------------------------
static std::string commandsText(IRule& rule)
{
    std::string out;
    for (IRuleCommand* command : rule.commands())
    {
        if (command == nullptr)
            continue;
        if (!out.empty())
            out += "\n";
        out += command->type();
    }
    return out;
}

// ----------------------------------------------------------------------------
//! \brief How many ticks are left before the next attempt of a rule whose
//! entity has already counted \c ticks of them.
// ----------------------------------------------------------------------------
static uint32_t ticksToGo(uint32_t period, uint32_t ticks)
{
    // executeRules() increments the counter first, so the rule fires when
    // (ticks + n) % period == 0.
    return (period - ((ticks + 1u) % period)) % period;
}

// ----------------------------------------------------------------------------
//! \brief The hours one rule keeps, so that a rule sleeping until eight in the
//! morning is not read as a rule that does nothing.
// ----------------------------------------------------------------------------
static OpeningHours ruleHours(IRule const& rule)
{
    OpeningHours hours;
    hours.add(rule);
    return hours;
}

// ----------------------------------------------------------------------------
OpeningStatus openingStatus(Unit const& unit, uint32_t hourOfDay)
{
    OpeningStatus status;

    if (unit.rules().empty())
    {
        status.known = true;
        status.open = false;
        status.text = "Inactive (no rules)";
        return status;
    }

    OpeningHours const hours = unit.openingHours();

    if (!hours.bounded())
    {
        status.known = true;
        status.open = true;
        status.text = "Active (always)";
        return status;
    }

    status.known = true;
    status.open = hours.isOpen(hourOfDay);
    if (status.open)
    {
        uint32_t const last = hours.closingAfter(hourOfDay);
        status.text = (last == OpeningHours::NEVER)
                          ? "Active"
                          : "Active until " +
                                std::to_string((last + 1u) %
                                               OpeningHours::HOURS_PER_DAY) +
                                "h";
        return status;
    }

    uint32_t const next = hours.nextOpening(hourOfDay);
    status.text = (next == OpeningHours::NEVER)
                      ? "Inactive (never opens)"
                      : "Inactive until " + std::to_string(next) + "h";
    return status;
}

// ----------------------------------------------------------------------------
//! \brief List the rules of the selected entity: how often each one is
//! attempted and how long until the next attempt, both counted in ticks, which
//! is the unit the engine actually runs on. Hovering a row spells the period
//! out in game time and names what the rule does.
//!
//! A rule that never fires with many ticks to go is simply not due yet; one
//! that is due every tick and still does nothing is blocked, and the Rule Log
//! says by what. A rule outside the hours it keeps is asleep, and the countdown
//! would claim it is about to fire.
// ----------------------------------------------------------------------------
template <typename RuleContainer>
static void drawRules(RuleContainer const& rules,
                      uint32_t ticks,
                      uint32_t ticksPerMinute,
                      uint32_t hourOfDay)
{
    if (rules.empty())
    {
        ImGui::TextDisabled("no rule");
        return;
    }

    if (!ImGui::BeginTable("rules",
                           3,
                           ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                               ImGuiTableFlags_SizingStretchProp))
        return;

    ImGui::TableSetupColumn("rule", ImGuiTableColumnFlags_WidthStretch, 2.0f);
    ImGui::TableSetupColumn("every", ImGuiTableColumnFlags_WidthStretch, 1.0f);
    ImGui::TableSetupColumn("in", ImGuiTableColumnFlags_WidthStretch, 1.0f);
    ImGui::TableHeadersRow();

    for (auto* rule : rules)
    {
        if (rule == nullptr)
            continue;

        uint32_t const period = std::max(1u, rule->periodTicks(ticksPerMinute));
        uint32_t const remaining = ticksToGo(period, ticks);
        OpeningHours const hours = ruleHours(*rule);
        bool const asleep = hours.bounded() && !hours.isOpen(hourOfDay);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        // The name spans the row so that hovering anywhere on it explains the
        // whole line rather than the cell under the cursor.
        ImGui::Selectable(
            rule->type().c_str(), false, ImGuiSelectableFlags_SpanAllColumns);
        if (ImGui::BeginItemTooltip())
        {
            ImGui::TextUnformatted(rule->type().c_str());
            ImGui::Separator();
            ImGui::Text("Every %u tick%s (%s of game time)",
                        period,
                        (period > 1u) ? "s" : "",
                        gameTimeText(period, ticksPerMinute).c_str());
            ImGui::Text("Next attempt in %u tick%s",
                        remaining,
                        (remaining > 1u) ? "s" : "");
            if (hours.bounded())
            {
                uint32_t const next = hours.nextOpening(hourOfDay);
                if (asleep && (next != OpeningHours::NEVER))
                {
                    ImGui::TextColored(
                        ImGui::ColorConvertU32ToFloat4(theme::FAILURE),
                        "Inactive until %uh: attempts are skipped",
                        next);
                }
                else if (asleep)
                {
                    ImGui::TextColored(
                        ImGui::ColorConvertU32ToFloat4(theme::FAILURE),
                        "The hours it keeps are empty: it never fires");
                }
                else
                {
                    ImGui::TextColored(
                        ImGui::ColorConvertU32ToFloat4(theme::SUCCESS),
                        "Active within its hours");
                }
            }
            ImGui::Separator();
            std::string const commands = commandsText(*rule);
            if (commands.empty())
                ImGui::TextDisabled("does nothing");
            else
                ImGui::TextUnformatted(commands.c_str());
            ImGui::EndTooltip();
        }

        ImGui::TableNextColumn();
        ImGui::Text("%u", period);

        ImGui::TableNextColumn();
        if (asleep)
        {
            // The countdown is still running, but the calendar condition will
            // refuse the attempt, so showing "now" would be a lie.
            ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(theme::MUTED),
                               "asleep");
        }
        else if (remaining == 0u)
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
    ImGui::TextDisabled("Periods are in ticks. Hover a row for the game time,\n"
                        "the hours the rule keeps and what it does.");
}

// ----------------------------------------------------------------------------
//! \brief One line of an agent listing: what it is, what it carries and how
//! far it still has to go.
// ----------------------------------------------------------------------------
static void drawAgentLine(Agent const& agent)
{
    std::string carried;
    for (Resource const& resource : agent.resources().container())
    {
        if (resource.getAmount() == 0u)
            continue;
        if (!carried.empty())
            carried += ", ";
        carried +=
            std::to_string(resource.getAmount()) + " " + resource.type().str();
    }
    if (carried.empty())
        carried = "empty";

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TextColored(
        ImGui::ColorConvertU32ToFloat4(theme::fromScript(agent.color())),
        "%s #%u",
        agent.type().c_str(),
        agent.id());
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

    for (auto const& agent : city.agents())
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

    ImGui::Text("%zu agent(s) sent out, %zu on their way in",
                outbound.size(),
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

    auto const table = [&](char const* id,
                           char const* first,
                           std::vector<Agent const*> const& agents)
    {
        if (agents.empty())
            return;
        if (!ImGui::BeginTable(id, 4, flags))
            return;
        ImGui::TableSetupColumn(first);
        ImGui::TableSetupColumn("carries");
        ImGui::TableSetupColumn("looks for");
        ImGui::TableSetupColumn("left");
        ImGui::TableHeadersRow();
        for (Agent const* agent : agents)
            drawAgentLine(*agent);
        ImGui::EndTable();
    };

    table("outbound", "sent out", outbound);
    table("inbound", "coming in", inbound);
}

// ----------------------------------------------------------------------------
void InspectorPanel::draw(Simulation& simulation,
                          game::DebugState& state,
                          game::RuleTrace const& trace)
{
    if (!ImGui::Begin("Inspector"))
    {
        ImGui::End();
        return;
    }

    if (ImGui::BeginTabBar("inspector"))
    {
        if (ImGui::BeginTabItem("Selection"))
        {
            ImGui::Spacing();
            switch (state.selection.kind)
            {
                case game::Selection::Kind::None:
                    ImGui::TextDisabled(
                        "Click a building, an agent, a road, a node, a zone\n"
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
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Ruleset"))
        {
            ImGui::Spacing();
            drawRuleset(simulation);
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}

// ----------------------------------------------------------------------------
//! \brief Every rule of the open ruleset, broken down: which kind of entity
//! runs it, how often, and what it does. The selection only ever shows the
//! rules of what is under the cursor, and reading a ruleset one building at a
//! time tells nothing about the city.
// ----------------------------------------------------------------------------
void InspectorPanel::drawRuleset(Simulation& simulation)
{
    uint32_t const perMinute = simulation.clock().ticksPerMinute();

    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputTextWithHint("##filter",
                             "filter by rule name or by command",
                             m_filter,
                             IM_ARRAYSIZE(m_filter));
    ImGui::Spacing();

    struct Row
    {
        char const* runBy;
        IRule* rule;
    };

    std::vector<Row> rows;
    ScriptDefinitions const& definitions = simulation.script().definitions();
    for (auto const& it : definitions.ruleMaps())
        rows.push_back({ "map", it.second.get() });
    for (auto const& it : definitions.ruleUnits())
        rows.push_back({ "building", it.second.get() });
    for (auto const& it : definitions.ruleAreas())
        rows.push_back({ "zone", it.second.get() });

    if (rows.empty())
    {
        ImGui::TextDisabled("This ruleset defines no rule at all.");
        return;
    }

    // Case insensitive, because a filter that only matches the exact casing of
    // a rule name is a filter nobody uses twice.
    auto contains = [](std::string haystack, std::string needle)
    {
        std::transform(haystack.begin(),
                       haystack.end(),
                       haystack.begin(),
                       [](unsigned char c) { return char(std::tolower(c)); });
        std::transform(needle.begin(),
                       needle.end(),
                       needle.begin(),
                       [](unsigned char c) { return char(std::tolower(c)); });
        return haystack.find(needle) != std::string::npos;
    };

    ImGuiTableFlags const flags =
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
        ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchProp;

    if (!ImGui::BeginTable("ruleset", 4, flags))
        return;

    ImGui::TableSetupColumn("Rule", ImGuiTableColumnFlags_WidthStretch, 1.6f);
    ImGui::TableSetupColumn("Run by", ImGuiTableColumnFlags_WidthStretch, 0.8f);
    ImGui::TableSetupColumn("Every", ImGuiTableColumnFlags_WidthStretch, 0.6f);
    ImGui::TableSetupColumn("Does", ImGuiTableColumnFlags_WidthStretch, 3.0f);
    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableHeadersRow();

    std::string const filter(m_filter);
    for (Row const& row : rows)
    {
        if (row.rule == nullptr)
            continue;

        std::string const commands = commandsText(*row.rule);
        if (!filter.empty() && !contains(row.rule->type(), filter) &&
            !contains(commands, filter))
        {
            continue;
        }

        uint32_t const period = std::max(1u, row.rule->periodTicks(perMinute));

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        // Nothing forbids a map rule and a unit rule from sharing a name, and
        // two rows answering to the same identifier highlight together.
        ImGui::PushID(row.rule);
        ImGui::Selectable(row.rule->type().c_str(),
                          false,
                          ImGuiSelectableFlags_SpanAllColumns);
        ImGui::PopID();
        if (ImGui::BeginItemTooltip())
        {
            ImGui::TextUnformatted(row.rule->type().c_str());
            ImGui::Separator();
            ImGui::Text("run by every %s", row.runBy);
            ImGui::Text("every %u tick%s (%s of game time)",
                        period,
                        (period > 1u) ? "s" : "",
                        gameTimeText(period, perMinute).c_str());
            ImGui::Separator();
            if (commands.empty())
                ImGui::TextDisabled("does nothing");
            else
                ImGui::TextUnformatted(commands.c_str());
            ImGui::EndTooltip();
        }

        ImGui::TableNextColumn();
        ImGui::TextDisabled("%s", row.runBy);

        ImGui::TableNextColumn();
        ImGui::Text("%u", period);

        ImGui::TableNextColumn();
        if (commands.empty())
        {
            ImGui::TextDisabled("nothing");
        }
        else
        {
            // One command per line: a rule is a list of conditions and of
            // effects, and running them together loses which is which.
            ImGui::TextUnformatted(commands.c_str());
        }
    }

    ImGui::EndTable();
}

// ----------------------------------------------------------------------------
void InspectorPanel::drawUnit(Simulation& simulation,
                              game::DebugState& state,
                              game::RuleTrace const& trace)
{
    Unit* const unit = state.selection.unit;
    if (unit == nullptr)
    {
        state.selection.clear();
        return;
    }

    ImGui::TextColored(
        ImGui::ColorConvertU32ToFloat4(theme::fromScript(unit->color())),
        "Building %s #%u",
        unit->type().c_str(),
        unit->id());
    ImGui::Spacing();

    auto const cityIt = simulation.cities().find(state.selection.city);
    City* const city =
        (cityIt == simulation.cities().end()) ? nullptr : cityIt->second.get();

    // Which zone the building stands in decides which area rules may upgrade
    // or demolish it, so it belongs with the rest of its identity.
    std::string zones;
    if (city != nullptr)
    {
        for (auto const& area : city->areas())
        {
            if (!area->contains(unit->mapU(), unit->mapV()))
                continue;
            if (!zones.empty())
                zones += ", ";
            zones += area->type();
        }
    }

    if (beginFields("unit"))
    {
        uint32_t const hour = simulation.clock().hourOfDay();
        OpeningStatus const opening = openingStatus(*unit, hour);

        field("City", "%s", state.selection.city.c_str());
        field("Cell", "(%d, %d)", unit->mapU(), unit->mapV());

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Status");
        ImGui::TableNextColumn();
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(
                               opening.open ? theme::SUCCESS : theme::FAILURE),
                           "%s",
                           opening.text.c_str());
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip(
                "Derived from the 'hour between' conditions of its rules.\n"
                "It is %02uh00 in the city.",
                hour);
        }

        field("Map radius", "%u", unit->mapRadius());
        field("Zone", "%s", zones.empty() ? "outside any zone" : zones.c_str());
        if (unit->node() != nullptr)
        {
            field("Stands on", "node #%u", unit->node()->id());
        }
        else if (unit->way() != nullptr)
        {
            field("Stands on",
                  "%s #%u at %.0f%%",
                  unit->way()->type().c_str(),
                  unit->way()->id(),
                  100.0f * unit->wayOffset());
        }
        else
        {
            field("Stands on", "nothing");
        }
        endFields();
    }

    if (!unit->hasWays())
    {
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(theme::FAILURE),
                           "Not connected to any road: the agents it creates\n"
                           "cannot leave, and none can reach it.");
        ImGui::Spacing();
    }

    ImGui::Checkbox("Show the radius on the map", &state.showSelectionRadius);

    drawResources("Resources", unit->resources().container());

    ImGui::SeparatorText("Accepts");
    if (unit->targets().empty())
    {
        ImGui::TextDisabled("nothing: no agent can deliver here");
    }
    else
    {
        std::string targets;
        for (std::string const& target : unit->targets())
        {
            if (!targets.empty())
                targets += ", ";
            targets += target;
        }
        ImGui::TextWrapped("agents looking for %s", targets.c_str());
    }
    ImGui::Spacing();

    ImGui::SeparatorText("Traffic");
    if (city != nullptr)
    {
        drawUnitAgents(*city, *unit);
    }
    ImGui::Spacing();

    ImGui::SeparatorText("Rules");
    drawRules(unit->rules(),
              unit->ticks(),
              simulation.clock().ticksPerMinute(),
              simulation.clock().hourOfDay());
    ImGui::Spacing();

    // Outcome of the last attempts recorded for this very unit type.
    ImGui::SeparatorText("Last attempts");
    if (!trace.recording())
    {
        ImGui::TextDisabled("Enable the recording in the Rule Log panel\n"
                            "to see why a rule does or does not fire.");
        return;
    }

    std::string const entity = "Unit " + unit->type().str();
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
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(color),
                           "%s",
                           event.success ? "ok" : "blocked");
        ImGui::SameLine();
        if (event.success)
        {
            ImGui::Text("%s (tick %llu)",
                        event.rule.c_str(),
                        (unsigned long long)event.tick);
        }
        else
        {
            ImGui::Text("%s by %s (tick %llu)",
                        event.rule.c_str(),
                        event.blockedBy.c_str(),
                        (unsigned long long)event.tick);
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

    ImGui::TextColored(
        ImGui::ColorConvertU32ToFloat4(theme::fromScript(agent->color())),
        "Agent %s #%u",
        agent->type().c_str(),
        agent->id());
    ImGui::Spacing();

    Way const* const way = agent->currentWay();
    if (beginFields("agent"))
    {
        field("City", "%s", state.selection.city.c_str());
        field("Looking for",
              "a building accepting %s",
              agent->searchTarget().c_str());
        field("Speed", "%.2f world units per second", agent->speed());
        field("Position",
              "(%.1f, %.1f)",
              agent->position().x,
              agent->position().y);
        if (way == nullptr)
            field("Driving on", "nothing: waiting on a node for a route");
        else
            field("driving on",
                  "%s, %.0f%% travelled",
                  way->type().c_str(),
                  100.0f * agent->offset());
        endFields();
    }

    ImGui::SeparatorText("Itinerary");
    Route const& route = agent->route();
    if (beginFields("route"))
    {
        field("Time to go", "%.2f s", agent->remainingCost());
        if (!route.found)
        {
            field("Route", "none cached: it is looking for one");
        }
        else
        {
            field("Nodes left", "%zu", route.waypointCount());
            if (route.destination != nullptr)
            {
                field("Destination",
                      "%s #%u",
                      route.destination->type().c_str(),
                      route.destination->id());
            }
            if (route.approachWay != nullptr)
            {
                field("Arrives on",
                      "%s at %.0f%%",
                      route.approachWay->type().c_str(),
                      100.0f * route.approachOffset);
            }
        }
        endFields();
    }

    drawResources("Carried", agent->resources().container());
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
    ImGui::Spacing();

    if (beginFields("node"))
    {
        field("City", "%s", state.selection.city.c_str());
        field(
            "Position", "(%.1f, %.1f)", node->position().x, node->position().y);
        endFields();
    }

    ImGui::SeparatorText("Roads");
    if (node->ways().empty())
    {
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(theme::FAILURE),
                           "Orphan node: no agent can reach it");
    }
    for (Way const* way : node->ways())
    {
        ImGui::BulletText("%s, %.1f long, %.2f s, %.0f%% full",
                          way->type().c_str(),
                          way->magnitude(),
                          way->travelTime(),
                          100.0f * way->saturation());
    }
    ImGui::Spacing();

    ImGui::SeparatorText("Buildings");
    if (node->units().empty())
    {
        ImGui::TextDisabled("none");
    }
    for (Unit const* unit : node->units())
    {
        ImGui::BulletText("%s #%u", unit->type().c_str(), unit->id());
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

    ImGui::Text("Road %s #%u", way->type().c_str(), way->id());
    ImGui::Spacing();

    if (beginFields("way"))
    {
        field("City", "%s", state.selection.city.c_str());
        field("From", "node #%u", way->from().id());
        field("To", "node #%u", way->to().id());
        field("Length", "%.1f", way->magnitude());
        field("Free flow", "%.2f s", way->freeFlowTime());
        field("Now", "%.2f s", way->travelTime());
        endFields();
    }

    float const saturation = way->saturation();
    char overlay[64];
    std::snprintf(overlay,
                  sizeof(overlay),
                  "%.0f / %.0f agents",
                  way->flow(),
                  way->capacity());
    ImGui::TextUnformatted("saturation");
    ImGui::SameLine(LABEL_WIDTH);
    ImGui::PushStyleColor(
        ImGuiCol_PlotHistogram,
        ImGui::ColorConvertU32ToFloat4(theme::congestionColor(saturation)));
    ImGui::ProgressBar(
        std::min(1.0f, saturation), ImVec2(-1.0f, 0.0f), overlay);
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

    ImGui::TextColored(
        ImGui::ColorConvertU32ToFloat4(theme::fromScript(area->color())),
        "Zone %s #%u",
        area->type().c_str(),
        area->id());
    ImGui::Spacing();

    if (beginFields("area"))
    {
        field("City", "%s", state.selection.city.c_str());
        field("Size", "%u x %u cells", footprint.sizeU, footprint.sizeV);
        field("Top left", "(%d, %d)", footprint.u0, footprint.v0);
        endFields();
    }

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
        for (Unit const* unit : units)
            ++counts[unit->type()];

        for (auto const& it : counts)
        {
            ImGui::BulletText("%u x %s", it.second, it.first.c_str());
        }
        ImGui::TextDisabled("%zu building(s) on %llu cell(s)",
                            units.size(),
                            (unsigned long long)footprint.area());
    }
    ImGui::Spacing();

    ImGui::SeparatorText("Rules");
    drawRules(area->rules(),
              area->ticks(),
              simulation.clock().ticksPerMinute(),
              simulation.clock().hourOfDay());
    ImGui::Spacing();

    auto const it = simulation.cities().find(state.selection.city);
    if (it == simulation.cities().end())
        return;

    City& city = *it->second;
    char header[64];
    std::snprintf(header,
                  sizeof(header),
                  "Cell (%d, %d) under the zone",
                  state.selection.u,
                  state.selection.v);
    ImGui::SeparatorText(header);
    for (auto& mapIt : city.maps())
    {
        Map& map = *mapIt.second;
        ImGui::TextColored(
            ImGui::ColorConvertU32ToFloat4(theme::fromScript(map.color())),
            "%s",
            map.type().c_str());
        ImGui::SameLine(LABEL_WIDTH);
        ImGui::Text("%u / %u",
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
    ImGui::Text("Cell (%d, %d) of %s",
                state.selection.u,
                state.selection.v,
                city.name().c_str());
    ImGui::Spacing();

    ImGui::SeparatorText("Maps");
    for (auto& mapIt : city.maps())
    {
        Map& map = *mapIt.second;
        uint32_t const amount =
            map.getResource(state.selection.u, state.selection.v);
        uint32_t const capacity = std::max(1u, map.getCapacity());

        char overlay[64];
        std::snprintf(
            overlay, sizeof(overlay), "%u / %u", amount, map.getCapacity());

        ImGui::TextColored(
            ImGui::ColorConvertU32ToFloat4(theme::fromScript(map.color())),
            "%s",
            map.type().c_str());
        ImGui::SameLine(LABEL_WIDTH);
        ImGui::ProgressBar(
            float(amount) / float(capacity), ImVec2(-1.0f, 0.0f), overlay);
    }
    ImGui::Spacing();

    drawResources("City globals", city.globals().container());
}
} // namespace ui
} // namespace ogb
