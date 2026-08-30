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

namespace ogb::ui
{

//! \brief Heights, in pixels, the two halves of the panel are shared by.
//! Docked in a short slot, the editor keeps a few lines to type in and the
//! breakdown a few rows to scroll; docked in a shorter one still, the window
//! scrolls rather than either of them collapsing.
static constexpr float MIN_EDITOR_HEIGHT = 120.0f;
static constexpr float MIN_RULES_HEIGHT = 90.0f;
static constexpr float SQUEEZED_EDITOR_HEIGHT = 60.0f;

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
//! \brief The fingerprint of what is being typed, beside the button that makes
//! it the fingerprint of the file.
//!
//! Sixty-four hexadecimal characters do not fit next to a button and nobody
//! reads them anyway: what they are good for is being compared with the header
//! of a save, and being pasted. The first digits do the first job, the
//! clipboard does the second.
// ----------------------------------------------------------------------------
static void drawEditorHash(std::string const& hash)
{
    if (hash.empty())
    {
        ImGui::TextDisabled("no ruleset");
        return;
    }

    ImGui::TextDisabled("%s", hash.substr(0u, 12u).c_str());
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("SHA-256 of the text in this editor:\n%s\n\n"
                          "A save records the fingerprint of the ruleset it\n"
                          "was written against and refuses to open against a\n"
                          "different one. Click to copy.",
                          hash.c_str());
    }
    if (ImGui::IsItemClicked())
    {
        ImGui::SetClipboardText(hash.c_str());
    }
}

// ----------------------------------------------------------------------------
//! \brief The rulesets sitting beside the open one, one line each, the open
//! one ticked. Picking another opens it.
// ----------------------------------------------------------------------------
static void drawRulesetList(ScriptPanel::Files const& files,
                            ScriptPanel::Actions& actions)
{
    std::string const current = fileName(files.current);
    char const* preview = current.empty() ? "none" : current.c_str();

    ImGui::SetNextItemWidth(200.0f);
    bool const unfolded = ImGui::BeginCombo("##rulesets", preview);
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("The rulesets sitting in the same directory.\n"
                          "Opening another one starts a new city.");
    }
    if (!unfolded)
        return;

    for (std::string const& path : files.rulesets)
    {
        std::string const name = fileName(path);
        bool const open = (path == files.current);
        if (ImGui::Selectable(name.c_str(), open) && !open)
        {
            actions.openRuleset = path;
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("%s", path.c_str());
        }
        if (open)
        {
            ImGui::SetItemDefaultFocus();
        }
    }

    ImGui::EndCombo();
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
        ImGui::SetTooltip(
            "Record the fingerprint of the ruleset as it is now\n"
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
void ScriptPanel::drawRuleset(Simulation const& simulation)
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
    for (auto const& [_, rule] : ruleset.getRuleLayers())
    {
        rows.push_back({ "layer", rule.get() });
    }
    for (auto const& [_, rule] : ruleset.getRuleBuildings())
    {
        rows.push_back({ "building", rule.get() });
    }
    for (auto const& [_, rule] : ruleset.getRuleZones())
    {
        rows.push_back({ "zone", rule.get() });
    }

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
void ScriptPanel::draw(Simulation const& simulation,
                       std::string& text,
                       std::string const& status,
                       Checksum const& checksum,
                       Files const& files,
                       Options& options,
                       Actions& actions)
{
    if (!ImGui::Begin("Script"))
    {
        ImGui::End();
        return;
    }

    drawRulesetList(files, actions);

    ImGui::SameLine();
    if (ImGui::Button("Apply"))
        actions.apply = true;
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Write this text to the ruleset and parse it again.\n"
                          "The city is kept as long as every type still\n"
                          "standing in it is still defined.");
    }

    ImGui::SameLine();
    drawEditorHash(checksum.edited);

    if (!status.empty())
    {
        ImGui::SameLine();
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(theme::ACCENT),
                           "%s",
                           status.c_str());
    }

    ImGui::Checkbox("Reload when the file changes", &options.autoReload);
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Watch the ruleset on disk and apply it again as\n"
                          "soon as it is written, so that a script edited in\n"
                          "another editor is seen here without a click.");
    }
    ImGui::SameLine();
    ImGui::Checkbox("Open stale saves", &options.ignoreMismatch);
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Waive the fingerprint check for a save that was\n"
                          "not stamped. The geometry is still read, and a\n"
                          "type the script dropped is still refused by name.");
    }

    if (checksum.known)
        drawChecksumStatus(checksum, actions);

    // The script and what it parses into share the panel: the breakdown of the
    // rules answers questions about the text right above it, and reading one
    // while the other is on another tab is reading them one at a time. The
    // text takes the larger share, and gives way down to a few lines rather
    // than leaving the rules nothing at all.
    float const available = ImGui::GetContentRegionAvail().y;
    float const editor = std::min(
        std::max(0.55f * available, MIN_EDITOR_HEIGHT),
        std::max(SQUEEZED_EDITOR_HEIGHT, available - MIN_RULES_HEIGHT));

    ImGui::InputTextMultiline("##script",
                              &text,
                              ImVec2(-1.0f, editor),
                              ImGuiInputTextFlags_AllowTabInput);

    ImGui::SeparatorText("Rules of this ruleset");
    drawRuleset(simulation);

    ImGui::End();
}
} // namespace ogb::ui
