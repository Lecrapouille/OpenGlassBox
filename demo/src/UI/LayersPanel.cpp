//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "UI/Panels.hpp"
#include "UI/Theme.hpp"
#include "OpenGlassBox/Simulation.hpp"

#include <algorithm>
#include <set>

namespace ogb {
namespace ui {
using namespace ogb::theme;


// ----------------------------------------------------------------------------
static char const* modeName(game::LayerMode mode)
{
    switch (mode)
    {
    case game::LayerMode::Heatmap: return "Heat";
    case game::LayerMode::Contour: return "Line";
    case game::LayerMode::Value: return "Val";
    }
    return "?";
}

// ----------------------------------------------------------------------------
static void collectLayerNames(Simulation& simulation, std::set<std::string>& names)
{
    for (auto& it: simulation.getCities())
    {
        for (auto& layer: it.second->getLayers())
            names.insert(layer.second->getTypeName().str());
    }
}

// ----------------------------------------------------------------------------
//! \brief One layer: visibility, colour, name, opacity and drawing mode. Each
//! control sits in its own table column so that they line up from row to row.
// ----------------------------------------------------------------------------
static void drawLayerRow(Simulation& simulation, game::DebugState& state,
                         std::string const& name)
{
    ImGui::PushID(name.c_str());

    game::LayerSettings& settings = state.layer(name);

    uint32_t color = 0xFFFFFF;
    uint64_t total = 0u;
    for (auto& it: simulation.getCities())
    {
        auto const layer = it.second->getLayers().find(name);
        if (layer != it.second->getLayers().end())
        {
            color = layer->second->getColor();
            total += layer->second->getTotalResource();
        }
    }

    float const rowHeight = ImGui::GetFrameHeight();

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::Checkbox("##visible", &settings.visible);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Show or hide this layer.");

    ImGui::TableNextColumn();
    ImGui::ColorButton("##color",
                       ImGui::ColorConvertU32ToFloat4(theme::fromScript(color)),
                       ImGuiColorEditFlags_NoTooltip |
                           ImGuiColorEditFlags_NoDragDrop,
                       ImVec2(rowHeight, rowHeight));

    ImGui::TableNextColumn();
    bool const isPrimary = (state.primaryLayer == name);
    if (isPrimary)
    {
        ImGui::PushStyleColor(ImGuiCol_Button,
                              ImGui::ColorConvertU32ToFloat4(theme::ACCENT));
    }
    if (ImGui::Button(name.c_str(), ImVec2(-1.0f, 0.0f)))
    {
        if (ImGui::GetIO().KeyAlt)
        {
            state.soloLayer =
                (state.soloLayer == name) ? std::string() : name;
        }
        else
        {
            state.primaryLayer = name;
            settings.visible = true;
            if (state.soloLayer == name)
                state.soloLayer.clear();
        }
    }
    if (isPrimary)
        ImGui::PopStyleColor();
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip(
            "Show %s as the main heatmap.\n"
            "Alt+click to show only this layer.\n"
            "total: %llu",
            name.c_str(), (unsigned long long)total);
    }

    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::SliderFloat("##opacity", &settings.opacity, 0.05f, 1.0f, "%.1f");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Layer opacity.");

    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::BeginCombo("##mode", modeName(settings.mode)))
    {
        game::LayerMode const modes[] = {
            game::LayerMode::Heatmap, game::LayerMode::Contour,
            game::LayerMode::Value
        };
        for (game::LayerMode mode: modes)
        {
            if (ImGui::Selectable(modeName(mode), settings.mode == mode))
                settings.mode = mode;
        }
        ImGui::EndCombo();
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip(
            "Heat = filled cells, Line = contours, Val = numeric labels "
            "(main layer only).");
    }

    ImGui::PopID();
}

// ----------------------------------------------------------------------------
void LayersPanel::drawColumn(Simulation& simulation, game::DebugState& state,
                             float width)
{
    std::set<std::string> names;
    collectLayerNames(simulation, names);
    if (names.empty())
    {
        ImGui::TextDisabled("No layer in this ruleset.");
        return;
    }

    // Beyond a handful of layers the column would eat the canvas, so it scrolls
    // instead of growing.
    float const rowHeight = ImGui::GetFrameHeightWithSpacing();
    size_t const visibleRows = std::min<size_t>(names.size(), 5u);
    float const height = rowHeight * float(visibleRows + 1u);

    ImGuiTableFlags const flags = ImGuiTableFlags_SizingFixedFit |
                                  ImGuiTableFlags_RowBg |
                                  ImGuiTableFlags_ScrollY;

    // A width of zero lets the table take all the room the caller has left.
    float const outerWidth = (width > 0.0f) ? width : 0.0f;
    if (!ImGui::BeginTable("layers", 5, flags, ImVec2(outerWidth, height)))
        return;

    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableSetupColumn("##visible", ImGuiTableColumnFlags_WidthFixed,
                            ImGui::GetFrameHeight());
    ImGui::TableSetupColumn("##color", ImGuiTableColumnFlags_WidthFixed,
                            ImGui::GetFrameHeight());
    ImGui::TableSetupColumn("layer", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("opacity", ImGuiTableColumnFlags_WidthFixed, 72.0f);
    ImGui::TableSetupColumn("mode", ImGuiTableColumnFlags_WidthFixed, 62.0f);
    ImGui::TableHeadersRow();

    for (std::string const& name: names)
        drawLayerRow(simulation, state, name);

    ImGui::EndTable();
}
} // namespace ui
} // namespace ogb
