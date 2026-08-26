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

    if (ImGui::CollapsingHeader("Checksum"))
    {
        if (ImGui::Button("Compute"))
            actions.computeChecksum = true;
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("SHA-256 of the ruleset. A save records it and\n"
                              "refuses to open against a ruleset that changed\n"
                              "since it was written.");
        }

        ImGui::SameLine();
        ImGui::Checkbox("Open saves with a stale checksum", &ignoreMismatch);
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("While a ruleset is being written its checksum\n"
                              "changes at every save. The geometry is still\n"
                              "read, and a type the new script dropped is\n"
                              "still refused by name.");
        }

        if (!checksum.known)
        {
            ImGui::TextDisabled("Not computed yet.");
        }
        else
        {
            drawHash("ruleset", checksum.onDisk);
            drawHash("this editor", checksum.edited);
            drawHash("open save", checksum.save);

            if (checksum.edited != checksum.onDisk)
            {
                ImGui::TextDisabled("The editor holds changes that Apply has "
                                    "not written yet.");
            }
            if (!checksum.save.empty() && (checksum.save != checksum.onDisk))
            {
                ImGui::TextColored(
                    ImGui::ColorConvertU32ToFloat4(theme::FAILURE),
                    "The open save was written against another version.");
                if (ImGui::Button("Re-stamp the open save"))
                    actions.restampSave = true;
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Write the city back with the checksum\n"
                                      "of the ruleset as it is now.");
                }
            }
        }
        ImGui::Spacing();
    }

    ImGui::InputTextMultiline("##script", &text, ImVec2(-1.0f, -1.0f),
                              ImGuiInputTextFlags_AllowTabInput);
    ImGui::End();
}
} // namespace ui
} // namespace ogb
