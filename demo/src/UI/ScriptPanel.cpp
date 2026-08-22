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
void ScriptPanel::draw(std::string& text, bool& applyRequested,
                       std::string const& status)
{
    if (!ImGui::Begin("Script"))
    {
        ImGui::End();
        return;
    }

    ImGui::TextDisabled("Ruleset (.ogs). Apply reparses and keeps the city if "
                        "every type still placed is still defined.");
    if (ImGui::Button("Apply"))
        applyRequested = true;
    if (!status.empty())
    {
        ImGui::SameLine();
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(theme::ACCENT),
                           "%s", status.c_str());
    }

    ImGui::InputTextMultiline("##script", &text, ImVec2(-1.0f, -1.0f),
                              ImGuiInputTextFlags_AllowTabInput);
    ImGui::End();
}
} // namespace ui
} // namespace ogb
