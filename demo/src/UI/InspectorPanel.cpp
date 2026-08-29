//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "Game/RuleTrace.hpp"
#include "OpenGlassBox/Simulation.hpp"
#include "UI/Panels.hpp"
#include "UI/Theme.hpp"

#include <imgui_stdlib.h>

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <map>
#include <sstream>
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
static std::string fixed(double value, int precision)
{
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision) << value;
    return oss.str();
}

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
static void field(char const* label, std::string const& value)
{
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TextDisabled("%s", label);
    ImGui::TableNextColumn();
    ImGui::TextUnformatted(value.c_str());
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

    std::string const overlay = std::to_string(resource.getAmount()) + " / " +
                                std::to_string(resource.getCapacity());

    ImGui::TextUnformatted(resource.getTypeName().c_str());
    ImGui::SameLine(LABEL_WIDTH);
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::ProgressBar(ratio, ImVec2(-1.0f, 0.0f), overlay.c_str());
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
//! is depends on TimeConfig::ticksPerMinute.
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
static std::string commandsText(IRule const& rule)
{
    std::string out;
    for (IRuleCommand* command : rule.getCommands())
    {
        if (command == nullptr)
            continue;
        if (!out.empty())
            out += "\n";
        out += command->getDescription();
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
OpeningStatus openingStatus(Building const& building, uint32_t hourOfDay)
{
    OpeningStatus status;

    if (building.getRules().empty())
    {
        status.known = true;
        status.open = false;
        status.text = "Inactive (no rules)";
        return status;
    }

    OpeningHours const hours = building.getOpeningHours();
    status.known = true;

    // A building is open as soon as one of its rules may run, and a rule with
    // no "hour between" may run at any hour. That is why a factory shipping
    // goods around the clock reads as always active while a house, all of
    // whose rules keep hours, shuts for the night: the difference is in the
    // ruleset, not in the reading of it. Since that answer surprises whoever
    // expects a shop to close, the rules that do keep hours are counted, so
    // that "always" is not read as "nothing here ever waits for the clock".
    if (!hours.isRestricted())
    {
        size_t timed = 0u;
        size_t asleep = 0u;
        for (IRule const* rule : building.getRules())
        {
            if (rule == nullptr)
                continue;
            OpeningHours const own = ruleHours(*rule);
            if (!own.isRestricted())
                continue;
            ++timed;
            if (!own.isOpen(hourOfDay))
                ++asleep;
        }

        status.open = true;
        if (timed == 0u)
        {
            status.text = "Active (no rule keeps hours)";
            return status;
        }

        status.text = "Active (" + std::to_string(timed) + " of " +
                      std::to_string(building.getRules().size()) +
                      " rules keep hours)";
        status.detail =
            (asleep == 0u)
                ? "Every rule may run at this hour."
                : (std::to_string(asleep) +
                   " of them sleep at this hour, the others run around the "
                   "clock, so the building never shuts.");
        return status;
    }

    status.open = hours.isOpen(hourOfDay);
    if (status.open)
    {
        uint32_t const last = hours.getClosingHour(hourOfDay);
        status.text = (last == OpeningHours::NEVER)
                          ? "Active"
                          : "Active until " +
                                std::to_string((last + 1u) %
                                               OpeningHours::HOURS_PER_DAY) +
                                "h";
        status.detail = "Every rule of this building keeps hours, so it has "
                        "nothing to do outside them.";
        return status;
    }

    uint32_t const next = hours.getNextOpeningHour(hourOfDay);
    status.text = (next == OpeningHours::NEVER)
                      ? "Inactive (never opens)"
                      : "Inactive until " + std::to_string(next) + "h";
    status.detail = "Every rule of this building keeps hours, so it has "
                    "nothing to do outside them.";
    return status;
}

// ----------------------------------------------------------------------------
//! \brief List the rules of the selected entity: how often each one is
//! attempted and how long until the next attempt, both counted in ticks, which
//! is the building the engine actually runs on. Hovering a row spells the period
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

        uint32_t const period = std::max(1u, rule->getPeriodTicks(ticksPerMinute));
        uint32_t const remaining = ticksToGo(period, ticks);
        OpeningHours const hours = ruleHours(*rule);
        bool const asleep = hours.isRestricted() && !hours.isOpen(hourOfDay);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        // The name spans the row so that hovering anywhere on it explains the
        // whole line rather than the cell under the cursor.
        ImGui::Selectable(
            rule->getName().c_str(), false, ImGuiSelectableFlags_SpanAllColumns);
        if (ImGui::BeginItemTooltip())
        {
            ImGui::TextUnformatted(rule->getName().c_str());
            ImGui::Separator();
            ImGui::Text("Every %u tick%s (%s of game time)",
                        period,
                        (period > 1u) ? "s" : "",
                        gameTimeText(period, ticksPerMinute).c_str());
            ImGui::Text("Next attempt in %u tick%s",
                        remaining,
                        (remaining > 1u) ? "s" : "");
            if (hours.isRestricted())
            {
                uint32_t const next = hours.getNextOpeningHour(hourOfDay);
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
    for (Resource const& resource : agent.getResources().getAll())
    {
        if (resource.getAmount() == 0u)
            continue;
        if (!carried.empty())
            carried += ", ";
        carried +=
            std::to_string(resource.getAmount()) + " " + resource.getTypeName().str();
    }
    if (carried.empty())
        carried = "empty";

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TextColored(
        ImGui::ColorConvertU32ToFloat4(theme::fromScript(agent.getColor())),
        "%s #%u",
        agent.getTypeName().c_str(),
        agent.getId());
    ImGui::TableNextColumn();
    ImGui::TextUnformatted(carried.c_str());
    ImGui::TableNextColumn();
    ImGui::TextUnformatted(agent.getTarget().c_str());
    ImGui::TableNextColumn();
    if (agent.getSegment() == nullptr)
        ImGui::TextDisabled("waiting");
    else
        ImGui::Text("%.1f s", agent.getRemainingCost());
}

// ----------------------------------------------------------------------------
//! \brief The agents this Building sent out and the agents on their way to it.
//!
//! The engine deletes an Agent the moment it unloads, so there is no such
//! thing as a population sitting inside a building. What a building does have
//! is traffic: what left it and what is coming.
// ----------------------------------------------------------------------------
static void drawBuildingAgents(City& city, Building const& building)
{
    std::vector<Agent const*> outbound;
    std::vector<Agent const*> inbound;

    for (auto const& agent : city.getAgents())
    {
        if (agent->getOwner() == &building)
        {
            outbound.push_back(agent.get());
            continue;
        }
        if (agent->getRoute().getDestination() == &building)
        {
            inbound.push_back(agent.get());
        }
    }

    ImGui::Text("%zu agent(s) sent out, %zu on their segment in",
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
                          game::RuleTrace const& trace) const
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
                        "or a cell on the layer to inspect it.");
                    break;
                case game::Selection::Kind::Building:
                    drawBuilding(simulation, state, trace);
                    break;
                case game::Selection::Kind::Agent:
                    drawAgent(simulation, state);
                    break;
                case game::Selection::Kind::Node:
                    drawNode(state);
                    break;
                case game::Selection::Kind::Segment:
                    drawSegment(state);
                    break;
                case game::Selection::Kind::Zone:
                    drawZone(simulation, state);
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
void InspectorPanel::drawRuleset(Simulation& simulation) const
{
    uint32_t const perMinute = simulation.getClock().getTicksPerMinute();

    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputTextWithHint(
        "##filter", "filter by rule name or by command", &m_filter);
    ImGui::Spacing();

    struct Row
    {
        char const* runBy;
        IRule* rule;
    };

    std::vector<Row> rows;
    Ruleset const& ruleset = simulation.getRuleset();
    for (auto const& it : ruleset.getRuleLayers())
        rows.push_back({ "layer", it.second.get() });
    for (auto const& it : ruleset.getRuleBuildings())
        rows.push_back({ "building", it.second.get() });
    for (auto const& it : ruleset.getRuleZones())
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

    std::string const& filter = m_filter;
    for (Row const& row : rows)
    {
        if (row.rule == nullptr)
            continue;

        std::string const commands = commandsText(*row.rule);
        if (!filter.empty() && !contains(row.rule->getName(), filter) &&
            !contains(commands, filter))
        {
            continue;
        }

        uint32_t const period = std::max(1u, row.rule->getPeriodTicks(perMinute));

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        // Nothing forbids a layer rule and a building rule from sharing a name, and
        // two rows answering to the same identifier highlight together.
        ImGui::PushID(row.rule);
        ImGui::Selectable(row.rule->getName().c_str(),
                          false,
                          ImGuiSelectableFlags_SpanAllColumns);
        ImGui::PopID();
        if (ImGui::BeginItemTooltip())
        {
            ImGui::TextUnformatted(row.rule->getName().c_str());
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
void InspectorPanel::drawBuilding(Simulation& simulation,
                              game::DebugState& state,
                              game::RuleTrace const& trace) const
{
    Building* const building = state.selection.building;
    if (building == nullptr)
    {
        state.selection.clear();
        return;
    }

    ImGui::TextColored(
        ImGui::ColorConvertU32ToFloat4(theme::fromScript(building->getColor())),
        "Building %s #%u",
        building->getTypeName().c_str(),
        building->getId());
    ImGui::Spacing();

    auto const cityIt = simulation.getCities().find(state.selection.city);
    City* const city =
        (cityIt == simulation.getCities().end()) ? nullptr : cityIt->second.get();

    // Which zone the building stands in decides which zone rules may upgrade
    // or demolish it, so it belongs with the rest of its identity.
    std::string zones;
    if (city != nullptr)
    {
        for (auto const& zone : city->getZones())
        {
            if (!zone->contains(building->getCell()))
                continue;
            if (!zones.empty())
                zones += ", ";
            zones += zone->getTypeName().str();
        }
    }

    if (beginFields("building"))
    {
        uint32_t const hour = simulation.getClock().getHourOfDay();
        OpeningStatus const opening = openingStatus(*building, hour);

        field("City", state.selection.city);
        field("Cell",
              "(" + std::to_string(building->getCell().u) + ", " +
                  std::to_string(building->getCell().v) + ")");

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Status");
        ImGui::TableNextColumn();
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(
                               opening.open ? theme::SUCCESS : theme::FAILURE),
                           "%s",
                           opening.text.c_str());
        if (ImGui::BeginItemTooltip())
        {
            ImGui::Text("Derived from the 'hour between' conditions of its "
                        "rules.\nIt is %02uh00 in the city.",
                        hour);
            if (!opening.detail.empty())
            {
                ImGui::Separator();
                ImGui::TextUnformatted(opening.detail.c_str());
            }
            ImGui::EndTooltip();
        }

        field("Layer radius", std::to_string(building->getLayerRadius()));
        field("Zone", zones.empty() ? "outside any zone" : zones);
        if (building->getNode() != nullptr)
        {
            field("Stands on", "node #" + std::to_string(building->getNode()->getId()));
        }
        else if (building->getSegment() != nullptr)
        {
            field("Stands on",
                  building->getSegment()->getTypeName().str() + " #" +
                      std::to_string(building->getSegment()->getId()) + " at " +
                      fixed(100.0 * building->getSegmentOffset(), 0) + "%");
        }
        else
        {
            field("Stands on", "nothing");
        }
        endFields();
    }

    if (!building->hasSegments())
    {
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(theme::FAILURE),
                           "Not connected to any road: the agents it creates\n"
                           "cannot leave, and none can reach it.");
        ImGui::Spacing();
    }

    ImGui::Checkbox("Show the radius on the layer", &state.showSelectionRadius);

    drawResources("Resources", building->getResources().getAll());

    ImGui::SeparatorText("Accepts");
    if (building->getTargets().empty())
    {
        ImGui::TextDisabled("nothing: no agent can deliver here");
    }
    else
    {
        std::string targets;
        for (Name const& target : building->getTargets())
        {
            if (!targets.empty())
                targets += ", ";
            targets += target.str();
        }
        ImGui::TextWrapped("agents looking for %s", targets.c_str());
    }
    ImGui::Spacing();

    ImGui::SeparatorText("Traffic");
    if (city != nullptr)
    {
        drawBuildingAgents(*city, *building);
    }
    ImGui::Spacing();

    ImGui::SeparatorText("Rules");
    drawRules(building->getRules(),
              building->getTicks(),
              simulation.getClock().getTicksPerMinute(),
              simulation.getClock().getHourOfDay());
    ImGui::Spacing();

    // Outcome of the last attempts recorded for this very building type.
    ImGui::SeparatorText("Last attempts");
    if (!trace.recording())
    {
        ImGui::TextDisabled("Enable the recording in the Rule Log panel\n"
                            "to see why a rule does or does not fire.");
        return;
    }

    std::string const entity = "Building " + building->getTypeName().str();
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
void InspectorPanel::drawAgent(Simulation& simulation,
                               game::DebugState const& state) const
{
    Agent* const agent = state.selection.resolveAgent(simulation);
    if (agent == nullptr)
    {
        ImGui::TextDisabled("The agent has delivered its load and is gone.");
        return;
    }

    ImGui::TextColored(
        ImGui::ColorConvertU32ToFloat4(theme::fromScript(agent->getColor())),
        "Agent %s #%u",
        agent->getTypeName().c_str(),
        agent->getId());
    ImGui::Spacing();

    Segment const* const segment = agent->getSegment();
    if (beginFields("agent"))
    {
        field("City", state.selection.city);
        field("Looking for",
              "a building accepting " + agent->getTarget().str());
        field("Speed",
              fixed(agent->getSpeed(), 2) + " world units per second");
        field("Position",
              "(" + fixed(agent->getPosition().x, 1) + ", " +
                  fixed(agent->getPosition().y, 1) + ")");
        if (segment == nullptr)
            field("Driving on", "nothing: waiting on a node for a route");
        else
            field("driving on",
                  segment->getTypeName().str() + ", " +
                      fixed(100.0 * agent->getOffset(), 0) + "% travelled");
        endFields();
    }

    ImGui::SeparatorText("Itinerary");
    Route const& route = agent->getRoute();
    if (beginFields("route"))
    {
        field("Time to go", fixed(agent->getRemainingCost(), 2) + " s");
        if (!route.isFound())
        {
            field("Route", "none cached: it is looking for one");
        }
        else
        {
            field("Nodes left", std::to_string(route.getWaypointCount()));
            if (route.getDestination() != nullptr)
            {
                field("Destination",
                      std::string(route.getDestination()->getTypeName()) + " #" +
                          std::to_string(route.getDestination()->getId()));
            }
            if (route.getApproachSegment() != nullptr)
            {
                field("Arrives on",
                      route.getApproachSegment()->getTypeName().str() + " at " +
                          fixed(100.0 * route.getApproachOffset(), 0) + "%");
            }
        }
        endFields();
    }

    drawResources("Carried", agent->getResources().getAll());
}

// ----------------------------------------------------------------------------
void InspectorPanel::drawNode(game::DebugState& state) const
{
    Node* const node = state.selection.node;
    if (node == nullptr)
    {
        state.selection.clear();
        return;
    }

    ImGui::Text("Node #%u", node->getId());
    ImGui::Spacing();

    if (beginFields("node"))
    {
        field("City", state.selection.city);
        field("Position",
              "(" + fixed(node->getPosition().x, 1) + ", " +
                  fixed(node->getPosition().y, 1) + ")");
        endFields();
    }

    ImGui::SeparatorText("Roads");
    if (node->getSegments().empty())
    {
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(theme::FAILURE),
                           "Orphan node: no agent can reach it");
    }
    for (Segment const* segment : node->getSegments())
    {
        ImGui::BulletText("%s, %.1f long, %.2f s, %.0f%% full",
                          segment->getTypeName().c_str(),
                          segment->getLength(),
                          segment->getTravelTime(),
                          100.0f * segment->getSaturation());
    }
    ImGui::Spacing();

    ImGui::SeparatorText("Buildings");
    if (node->getBuildings().empty())
    {
        ImGui::TextDisabled("none");
    }
    for (Building const* building : node->getBuildings())
    {
        ImGui::BulletText("%s #%u", building->getTypeName().c_str(), building->getId());
    }
}

// ----------------------------------------------------------------------------
void InspectorPanel::drawSegment(game::DebugState& state) const
{
    Segment* const segment = state.selection.segment;
    if (segment == nullptr)
    {
        state.selection.clear();
        return;
    }

    ImGui::Text("Road %s #%u", segment->getTypeName().c_str(), segment->getId());
    ImGui::Spacing();

    if (beginFields("segment"))
    {
        field("City", state.selection.city);
        field("From", "node #" + std::to_string(segment->getFrom().getId()));
        field("To", "node #" + std::to_string(segment->getTo().getId()));
        field("Length", fixed(segment->getLength(), 1));
        field("Free flow", fixed(segment->getFreeFlowTime(), 2) + " s");
        field("Now", fixed(segment->getTravelTime(), 2) + " s");
        endFields();
    }

    float const saturation = segment->getSaturation();
    std::string const overlay = fixed(segment->getFlow(), 0) + " / " +
                                fixed(segment->getCapacity(), 0) + " agents";
    ImGui::TextUnformatted("saturation");
    ImGui::SameLine(LABEL_WIDTH);
    ImGui::PushStyleColor(
        ImGuiCol_PlotHistogram,
        ImGui::ColorConvertU32ToFloat4(theme::congestionColor(saturation)));
    ImGui::ProgressBar(
        std::min(1.0f, saturation), ImVec2(-1.0f, 0.0f), overlay.c_str());
    ImGui::PopStyleColor();
}

// ----------------------------------------------------------------------------
void InspectorPanel::drawZone(Simulation& simulation,
                              game::DebugState& state) const
{
    Zone const* zone = state.selection.zone;
    if (zone == nullptr)
    {
        state.selection.clear();
        return;
    }

    CellRegion const& footprint = zone->getRegion();

    ImGui::TextColored(
        ImGui::ColorConvertU32ToFloat4(theme::fromScript(zone->getColor())),
        "Zone %s #%u",
        zone->getTypeName().c_str(),
        zone->getId());
    ImGui::Spacing();

    if (beginFields("zone"))
    {
        field("City", state.selection.city);
        field("Size",
              std::to_string(footprint.sizeU) + " x " +
                  std::to_string(footprint.sizeV) + " cells");
        field("Top left",
              "(" + std::to_string(footprint.u0) + ", " +
                  std::to_string(footprint.v0) + ")");
        endFields();
    }

    std::vector<Building*> const buildings = zone->getBuildingsInside();

    ImGui::SeparatorText("Buildings inside");
    if (buildings.empty())
    {
        ImGui::TextDisabled("Empty. An zone rule has to spawn something, and\n"
                            "its conditions are listed below.");
    }
    else
    {
        // Grouped by type: what matters about a zone is its mix, not the
        // identity of each building, which the buildings themselves report.
        std::map<std::string, uint32_t, std::less<>> counts;
        for (Building const* building : buildings)
            ++counts[building->getTypeName().str()];

        for (auto const& it : counts)
        {
            ImGui::BulletText("%u x %s", it.second, it.first.c_str());
        }
        ImGui::TextDisabled("%zu building(s) on %llu cell(s)",
                            buildings.size(),
                            (unsigned long long)footprint.getCellCount());
    }
    ImGui::Spacing();

    ImGui::SeparatorText("Rules");
    drawRules(zone->getRules(),
              zone->getTicks(),
              simulation.getClock().getTicksPerMinute(),
              simulation.getClock().getHourOfDay());
    ImGui::Spacing();

    auto const it = simulation.getCities().find(state.selection.city);
    if (it == simulation.getCities().end())
        return;

    City& city = *it->second;
    std::string const header =
        "Cell (" + std::to_string(state.selection.u) + ", " +
        std::to_string(state.selection.v) + ") under the zone";
    ImGui::SeparatorText(header.c_str());
    for (auto const& layerIt : city.getLayers())
    {
        Layer const& layer = *layerIt.second;
        ImGui::TextColored(
            ImGui::ColorConvertU32ToFloat4(theme::fromScript(layer.getColor())),
            "%s",
            layer.getTypeName().c_str());
        ImGui::SameLine(LABEL_WIDTH);
        ImGui::Text("%u / %u",
                    layer.getResource({ state.selection.u, state.selection.v }),
                    layer.getCellCapacity());
    }
}

// ----------------------------------------------------------------------------
void InspectorPanel::drawCell(Simulation& simulation,
                              game::DebugState& state) const
{
    auto const it = simulation.getCities().find(state.selection.city);
    if (it == simulation.getCities().end())
    {
        state.selection.clear();
        return;
    }

    City& city = *it->second;
    ImGui::Text("Cell (%d, %d) of %s",
                state.selection.u,
                state.selection.v,
                city.getName().c_str());
    ImGui::Spacing();

    ImGui::SeparatorText("Layers");
    for (auto const& layerIt : city.getLayers())
    {
        Layer const& layer = *layerIt.second;
        uint32_t const amount =
            layer.getResource({ state.selection.u, state.selection.v });
        uint32_t const capacity = std::max(1u, layer.getCellCapacity());

        std::string const overlay =
            std::to_string(amount) + " / " + std::to_string(layer.getCellCapacity());

        ImGui::TextColored(
            ImGui::ColorConvertU32ToFloat4(theme::fromScript(layer.getColor())),
            "%s",
            layer.getTypeName().c_str());
        ImGui::SameLine(LABEL_WIDTH);
        ImGui::ProgressBar(float(amount) / float(capacity),
                           ImVec2(-1.0f, 0.0f),
                           overlay.c_str());
    }
    ImGui::Spacing();

    drawResources("City globals", city.getGlobals().getAll());
}
} // namespace ui
} // namespace ogb
