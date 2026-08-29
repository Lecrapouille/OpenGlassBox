//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "UI/Panels.hpp"
#include "UI/Theme.hpp"

#include <imgui_stdlib.h>

namespace ogb {
namespace ui {


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
void ScriptPanel::draw(std::string& text, std::string const& status,
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

    ImGui::InputTextMultiline("##script", &text, ImVec2(-1.0f, -1.0f),
                              ImGuiInputTextFlags_AllowTabInput);
    ImGui::End();
}
} // namespace ui
} // namespace ogb
