//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "UI/Panels.hpp"
#include "UI/Theme.hpp"
#include "OpenGlassBox/Simulation.hpp"

#include <set>

namespace ogb {
namespace ui {
using namespace ogb::theme;


// ----------------------------------------------------------------------------
static char const* modeName(game::LayerMode mode)
{
    switch (mode)
    {
    case game::LayerMode::Heatmap: return "Heatmap";
    case game::LayerMode::Contour: return "Contour";
    case game::LayerMode::Value: return "Value";
    }
    return "?";
}

// ----------------------------------------------------------------------------
void LayersPanel::draw(Simulation& simulation, game::DebugState& state)
{
    if (!ImGui::Begin("Layers"))
    {
        ImGui::End();
        return;
    }

    ImGui::SeparatorText("Display");
    ImGui::Checkbox("Grid", &state.showGrid);
    ImGui::Checkbox("Paths", &state.showPaths);
    ImGui::Checkbox("Units", &state.showUnits);
    ImGui::Checkbox("Areas", &state.showAreas);
    ImGui::Checkbox("Agents", &state.showAgents);
    ImGui::Checkbox("Traffic", &state.showTraffic);
    ImGui::Checkbox("Labels", &state.showLabels);

    std::set<std::string> names;
    for (auto& it: simulation.cities())
    {
        for (auto& map: it.second->maps())
        {
            names.insert(map.second->type());
        }
    }

    ImGui::SeparatorText("Maps");
    if (names.empty())
    {
        ImGui::TextDisabled("No map. Load a ruleset.");
        ImGui::End();
        return;
    }

    ImGui::TextDisabled("Visible + which map is the main heatmap.");
    ImGui::Separator();

    for (std::string const& name: names)
    {
        ImGui::PushID(name.c_str());

        game::LayerSettings& settings = state.layer(name);

        uint32_t color = 0xFFFFFF;
        uint64_t total = 0u;
        for (auto& it: simulation.cities())
        {
            auto const map = it.second->maps().find(name);
            if (map != it.second->maps().end())
            {
                color = map->second->color();
                total += map->second->totalResource();
            }
        }

        ImGui::ColorButton("##color",
                           ImGui::ColorConvertU32ToFloat4(theme::fromScript(color)),
                           ImGuiColorEditFlags_NoTooltip |
                           ImGuiColorEditFlags_NoDragDrop,
                           ImVec2(14.0f, 14.0f));
        ImGui::SameLine();

        bool const isPrimary = (state.primaryLayer == name);
        if (ImGui::RadioButton("##primary", isPrimary))
        {
            state.primaryLayer = isPrimary ? std::string() : name;
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Show %s as the main heatmap", name.c_str());
        }
        ImGui::SameLine();

        ImGui::Checkbox(name.c_str(), &settings.visible);

        if (ImGui::TreeNode("…"))
        {
            ImGui::SetNextItemWidth(-90.0f);
            ImGui::SliderFloat("opacity", &settings.opacity, 0.05f, 1.0f, "%.2f");

            ImGui::SetNextItemWidth(-90.0f);
            if (ImGui::BeginCombo("mode", modeName(settings.mode)))
            {
                game::LayerMode const modes[] = {
                    game::LayerMode::Heatmap, game::LayerMode::Contour,
                    game::LayerMode::Value
                };
                for (game::LayerMode mode: modes)
                {
                    if (ImGui::Selectable(modeName(mode), settings.mode == mode))
                    {
                        settings.mode = mode;
                    }
                }
                ImGui::EndCombo();
            }

            ImGui::TextDisabled("total: %llu", (unsigned long long)total);
            ImGui::TreePop();
        }

        ImGui::PopID();
    }

    ImGui::End();
}
} // namespace ui
} // namespace ogb
