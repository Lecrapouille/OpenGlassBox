//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "UI/Panels.hpp"
#include "UI/Theme.hpp"

#include <imgui_stdlib.h>

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

namespace ogb {
namespace ui {

//! \brief Heights, in pixels, the two halves of the panel are never squeezed
//! below. Whichever is docked in a short slot, the editor keeps a few lines to
//! type in and the breakdown keeps a few rows to scroll.
static constexpr float MIN_EDITOR_HEIGHT = 120.0f;
static constexpr float MIN_RULES_HEIGHT = 90.0f;


// ----------------------------------------------------------------------------
//! \brief Trailing part of a path, enough to tell two saves apart without
//! spelling out a directory nobody needs to read.
// ----------------------------------------------------------------------------
static std::string fileName(std::string const& path)
{
    size_t const slash = path.find_last_of("/\\");
    return (slash == std::string::npos) ? path : path.substr(slash + 1u);
}

// ----------------------------------------------------------------------------
//! \brief One fingerprint on a line, with the button that puts it in the
//! clipboard. Sixty-four hexadecimal characters do not fit in a narrow panel
//! and nobody reads them: what they are good for is being pasted.
// ----------------------------------------------------------------------------
static void drawHash(char const* label, std::string const& hash)
{
    ImGui::TextDisabled("%s", label);
    ImGui::SameLine(110.0f);

    if (hash.empty())
    {
        ImGui::TextDisabled("none");
        return;
    }

    ImGui::TextUnformatted(hash.substr(0u, 16u).c_str());
    ImGui::SameLine();
    ImGui::PushID(label);
    if (ImGui::SmallButton("copy"))
        ImGui::SetClipboardText(hash.c_str());
    ImGui::PopID();
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", hash.c_str());
}

// ----------------------------------------------------------------------------
//! \brief Where the ruleset and the saves beside it stand, in one line.
//!
//! The panel used to print three fingerprints and leave the reader to compare
//! them. What the answer is used for is knowing whether a save still opens, so
//! that is what is said, and the digits moved out of the way.
// ----------------------------------------------------------------------------
static void drawChecksumStatus(ScriptPanel::Checksum const& checksum,
                               ScriptPanel::Actions& actions)
{
    if (checksum.edited != checksum.onDisk)
    {
        ImGui::TextDisabled("Unapplied changes in this editor.");
        return;
    }

    if (checksum.staleSaves.empty())
    {
        ImGui::TextDisabled("Saves match this ruleset.");
        return;
    }

    std::string names;
    for (std::string const& save : checksum.staleSaves)
    {
        if (!names.empty())
            names += ", ";
        names += fileName(save);
    }

    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(theme::FAILURE),
                       "Written against another version: %s",
                       names.c_str());

    if (ImGui::Button("Update them"))
        actions.restampSaves = true;
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Record the fingerprint of the ruleset as it is now\n"
                          "into those saves, so that they open again. Only the\n"
                          "header changes; a type the script dropped is still\n"
                          "refused by name when the save is read.");
    }
}

// ----------------------------------------------------------------------------
//! \brief Every rule of the open ruleset, broken down: which kind of entity
//! runs it, how often, and what it does.
//!
//! It reads the script the panel above it holds, one row per rule instead of
//! one paragraph per rule, which is the form the question "what does this
//! ruleset actually do" is asked in.
// ----------------------------------------------------------------------------
void ScriptPanel::drawRuleset(Simulation& simulation)
{
    uint32_t const perMinute = simulation.getClock().getTicksPerMinute();

    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputTextWithHint(
        "##filter", "filter by rule name or by command", &m_filter);

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

    for (Row const& row : rows)
    {
        if (row.rule == nullptr)
            continue;

        std::string const commands = commandsText(*row.rule);
        if (!m_filter.empty() && !contains(row.rule->getName(), m_filter) &&
            !contains(commands, m_filter))
        {
            continue;
        }

        uint32_t const period =
            std::max(1u, row.rule->getPeriodTicks(perMinute));

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        // Nothing forbids a layer rule and a building rule from sharing a
        // name, and two rows answering to the same identifier highlight
        // together.
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
void ScriptPanel::draw(Simulation& simulation, std::string& text,
                       std::string const& status,
                       Checksum const& checksum, bool& ignoreMismatch,
                       Actions& actions)
{
    if (!ImGui::Begin("Script"))
    {
        ImGui::End();
        return;
    }

    ImGui::TextDisabled("Ruleset (.ogs). Apply reparses and keeps the city if "
                        "every type still placed is still defined.");
    if (ImGui::Button("Apply"))
        actions.apply = true;
    if (!status.empty())
    {
        ImGui::SameLine();
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(theme::ACCENT),
                           "%s", status.c_str());
    }

    if (checksum.known)
        drawChecksumStatus(checksum, actions);

    if (ImGui::CollapsingHeader("Checksum details"))
    {
        ImGui::TextDisabled(
            "A save records the SHA-256 of the ruleset it was written against\n"
            "and refuses to open against one that changed since. Apply keeps\n"
            "the saves beside the ruleset stamped with the current one.");

        drawHash("ruleset", checksum.onDisk);
        drawHash("this editor", checksum.edited);
        drawHash("open save", checksum.save);

        ImGui::Checkbox("Open saves with a stale checksum", &ignoreMismatch);
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Waive the check for a save that was not\n"
                              "stamped. The geometry is still read, and a type\n"
                              "the script dropped is still refused by name.");
        }
        ImGui::Spacing();
    }

    // The script and what it parses into share the panel: the breakdown of the
    // rules answers questions about the text right above it, and reading one
    // while the other is on another tab is reading them one at a time.
    float const available = ImGui::GetContentRegionAvail().y;
    float const rules =
        std::min(std::max(0.45f * available, MIN_RULES_HEIGHT),
                 std::max(0.0f, available - MIN_EDITOR_HEIGHT));

    ImGui::InputTextMultiline("##script", &text, ImVec2(-1.0f, -rules),
                              ImGuiInputTextFlags_AllowTabInput);

    ImGui::SeparatorText("Rules of this ruleset");
    drawRuleset(simulation);

    ImGui::End();
}
} // namespace ui
} // namespace ogb
