//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "UI/Panels.hpp"
#include "UI/Theme.hpp"
#include "OpenGlassBox/Simulation.hpp"

#include <algorithm>
#include <vector>

namespace ogb {
namespace ui {
using namespace ogb::theme;


// ----------------------------------------------------------------------------
//! \brief One row of the ranking of the busiest segments.
// ----------------------------------------------------------------------------
struct WayRow
{
    std::string city;
    std::string path;
    std::string type;
    float saturation = 0.0f;
    float travelTime = 0.0f;
    float freeFlowTime = 0.0f;
    uint32_t agents = 0u;
};

// ----------------------------------------------------------------------------
float TrafficPanel::totalTravelTime(Simulation& simulation)
{
    float total = 0.0f;

    for (auto& it: simulation.cities())
    {
        for (auto& path: it.second->paths())
        {
            for (auto& way: path.second->ways())
            {
                total += way->flow() * way->travelTime();
            }
        }
    }

    return total;
}

// ----------------------------------------------------------------------------
void TrafficPanel::draw(Simulation& simulation, game::DebugState& state)
{
    if (!ImGui::Begin("Traffic"))
    {
        ImGui::End();
        return;
    }

    std::vector<WayRow> rows;
    float totalTime = 0.0f;
    float freeFlowTotal = 0.0f;
    uint32_t travelling = 0u;

    for (auto& it: simulation.cities())
    {
        City& city = *it.second;
        for (auto& pathIt: city.paths())
        {
            Path& path = *pathIt.second;
            for (auto& way: path.ways())
            {
                WayRow row;
                row.city = city.name();
                row.path = path.type();
                row.type = way->type();
                row.saturation = way->saturation();
                row.travelTime = way->travelTime();
                row.freeFlowTime = way->freeFlowTime();
                row.agents = way->agentCount();

                totalTime += way->flow() * row.travelTime;
                freeFlowTotal += way->flow() * row.freeFlowTime;
                travelling += row.agents;

                rows.push_back(std::move(row));
            }
        }
    }

    if (rows.empty())
    {
        ImGui::TextDisabled("No road in the simulation.");
        ImGui::End();
        return;
    }

    ImGui::Text("%u agent%s on the roads", travelling,
                (travelling > 1u) ? "s" : "");
    ImGui::Text("total travel time: %.2f s", totalTime);
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip(
            "Sum over the roads of the flow times the travel time. This is\n"
            "the quantity a Wardrop equilibrium minimizes: once it stops\n"
            "drifting, the routing has settled.");
    }

    float const gap = simulation.relativeGap();
    ImGui::Text("relative gap: %.2f %%", 100.0f * gap);
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip(
            "How far the current assignment is from the shortest paths at\n"
            "the current travel times, (TSTT - SPTT) / TSTT. Inspired by\n"
            "the Relgap of the MSA of CiudadSim. Near zero, Agents are on\n"
            "cheapest itineraries and the network has settled.");
    }

    // How much the congestion costs compared to an empty network. Zero means
    // free flow, and a large value means the demand exceeds the capacity.
    float const excess = (freeFlowTotal > 0.0f)
                         ? (totalTime - freeFlowTotal) / freeFlowTotal
                         : 0.0f;
    ImGui::Text("congestion overhead: %.1f %%", 100.0f * excess);

    ImGui::SeparatorText("Busiest roads");

    std::sort(rows.begin(), rows.end(),
              [](WayRow const& a, WayRow const& b) {
                  return a.saturation > b.saturation;
              });

    ImGuiTableFlags const flags = ImGuiTableFlags_Borders |
                                  ImGuiTableFlags_RowBg |
                                  ImGuiTableFlags_ScrollY |
                                  ImGuiTableFlags_SizingStretchProp;

    // The panel is docked on a narrow side, so the identity of the segment is
    // kept short and the full breakdown is moved to a tooltip.
    if (ImGui::BeginTable("ways", 4, flags))
    {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("road");
        ImGui::TableSetupColumn("agents", ImGuiTableColumnFlags_WidthFixed, 46.0f);
        ImGui::TableSetupColumn("load");
        ImGui::TableSetupColumn("time", ImGuiTableColumnFlags_WidthFixed, 54.0f);
        ImGui::TableHeadersRow();

        size_t const shown = std::min<size_t>(rows.size(), 40u);
        for (size_t i = 0u; i < shown; ++i)
        {
            WayRow const& row = rows[i];

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(row.type.c_str());
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("%s / %s / %s\n%u agent%s\n"
                                  "travel time %.2f s, free flow %.2f s",
                                  row.city.c_str(), row.path.c_str(),
                                  row.type.c_str(), row.agents,
                                  (row.agents > 1u) ? "s" : "",
                                  row.travelTime, row.freeFlowTime);
            }

            ImGui::TableNextColumn();
            ImGui::Text("%u", row.agents);

            ImGui::TableNextColumn();
            ImGui::PushStyleColor(
                ImGuiCol_PlotHistogram,
                ImGui::ColorConvertU32ToFloat4(
                    theme::congestionColor(row.saturation)));
            char overlay[32];
            std::snprintf(overlay, sizeof(overlay), "%.0f %%",
                          100.0f * row.saturation);
            ImGui::ProgressBar(std::min(1.0f, row.saturation),
                               ImVec2(-1.0f, 0.0f), overlay);
            ImGui::PopStyleColor();

            ImGui::TableNextColumn();
            ImGui::Text("%.2fs", row.travelTime);
        }

        ImGui::EndTable();
    }

    ImGui::End();
}
} // namespace ui
} // namespace ogb
