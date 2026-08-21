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

// ----------------------------------------------------------------------------
static char const* modeName(LayerMode mode)
{
    switch (mode)
    {
    case LayerMode::Heatmap: return "Heatmap";
    case LayerMode::Contour: return "Contour";
    case LayerMode::Value: return "Value";
    }
    return "?";
}

// ----------------------------------------------------------------------------
void LayersPanel::draw(Simulation& simulation, DebugState& state)
{
    if (!ImGui::Begin("Layers"))
    {
        ImGui::End();
        return;
    }

    // A map name may exist in several cities; the settings are shared, which is
    // what one wants when comparing two cities side by side.
    std::set<std::string> names;
    for (auto& it: simulation.cities())
    {
        for (auto& map: it.second->maps())
        {
            names.insert(map.second->type());
        }
    }

    if (names.empty())
    {
        ImGui::TextDisabled("No map. Load a simulation script.");
        ImGui::End();
        return;
    }

    ImGui::TextDisabled("The main layer fills the cells, the others are drawn "
                        "inset so that they stay readable when superimposed.");
    ImGui::Separator();

    if (!state.soloLayer.empty())
    {
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(theme::ACCENT),
                           "Solo: %s", state.soloLayer.c_str());
        ImGui::SameLine();
        if (ImGui::SmallButton("Exit solo"))
        {
            state.soloLayer.clear();
        }
        ImGui::Separator();
    }

    for (std::string const& name: names)
    {
        ImGui::PushID(name.c_str());

        LayerSettings& settings = state.layer(name);

        // Color swatch, so that the legend of the canvas is right here.
        uint32_t color = 0xFFFFFF;
        for (auto& it: simulation.cities())
        {
            auto const map = it.second->maps().find(name);
            if (map != it.second->maps().end())
            {
                color = map->second->color();
                break;
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

        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 44.0f);
        bool const isSolo = (state.soloLayer == name);
        if (ImGui::SmallButton(isSolo ? "unsolo" : "solo"))
        {
            state.soloLayer = isSolo ? std::string() : name;
        }

        ImGui::Indent();
        ImGui::SetNextItemWidth(-90.0f);
        ImGui::SliderFloat("opacity", &settings.opacity, 0.05f, 1.0f, "%.2f");

        ImGui::SetNextItemWidth(-90.0f);
        if (ImGui::BeginCombo("mode", modeName(settings.mode)))
        {
            LayerMode const modes[] = { LayerMode::Heatmap, LayerMode::Contour,
                                        LayerMode::Value };
            for (LayerMode mode: modes)
            {
                if (ImGui::Selectable(modeName(mode), settings.mode == mode))
                {
                    settings.mode = mode;
                }
            }
            ImGui::EndCombo();
        }

        // Total held by this map, all cities together: a quick way to see a
        // resource being exhausted.
        uint64_t total = 0u;
        for (auto& it: simulation.cities())
        {
            auto const map = it.second->maps().find(name);
            if (map != it.second->maps().end())
            {
                total += map->second->totalResource();
            }
        }
        ImGui::TextDisabled("total: %llu", (unsigned long long)total);

        ImGui::Unindent();
        ImGui::Separator();
        ImGui::PopID();
    }

    ImGui::End();
}

} // namespace ogb
