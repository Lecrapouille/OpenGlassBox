//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "OpenGlassBox/Simulation.hpp"
#include "UI/Panels.hpp"
#include "UI/Theme.hpp"

#include <algorithm>
#include <vector>

namespace ogb
{
namespace ui
{
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
//! \brief Label for the relative gap bands described in doc/traffic.md.
// ----------------------------------------------------------------------------
struct RoutingQuality
{
    char const* label;
    char const* detail;
};

RoutingQuality routingQuality(float const gap)
{
    float const percent = 100.0f * gap;
    if (percent <= 1.0f) // 1%
    {
        return { "Equilibrium",
                 "Every sampled agent is on the cheapest route available "
                 "right now." };
    }
    if (percent < 5.0f) // 5%
    {
        return { "Near equilibrium",
                 "Routing has largely settled. Small differences come from "
                 "agents between deviation checks or sampling noise." };
    }
    if (percent < 15.0f) // 15%
    {
        return { "Normal churn",
                 "Typical for a busy city. Some agents still follow "
                 "itineraries chosen before traffic shifted." };
    }
    if (percent < 30.0f) // 30%
    {
        return { "Clearly off",
                 "Many agents pay substantially more than today's shortest "
                 "path: fresh congestion or deviation checks set too high." };
    }
    return { "Strong mismatch",
             "Often right after a shock. Expect visible jams until agents "
             "re-check their routes." };
}

// ----------------------------------------------------------------------------
float TrafficPanel::totalTravelTime(Simulation& simulation)
{
    float total = 0.0f;

    for (auto& it : simulation.cities())
    {
        for (auto const& path : it.second->paths())
        {
            for (auto& way : path.second->ways())
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

    ImGui::SeparatorText("Routing");
    float smoothing = simulation.config().trafficSmoothing;
    ImGui::SetNextItemWidth(-140.0f);
    if (ImGui::SliderFloat("flow smoothing",
                           &smoothing,
                           0.005f,
                           1.0f,
                           "%.3f",
                           ImGuiSliderFlags_Logarithmic))
    {
        simulation.config().trafficSmoothing = smoothing;
        for (auto& it : simulation.cities())
            it.second->config().trafficSmoothing = smoothing;
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip(
            "Step of the moving average applied to the traffic flow.\n"
            "Set it to one to route on the raw instantaneous count.");
    }

    auto recalc = int(simulation.config().pathRecalcTicks);
    ImGui::SetNextItemWidth(-140.0f);
    if (ImGui::SliderInt("path recalc", &recalc, 1, 400))
    {
        simulation.config().pathRecalcTicks = uint32_t(recalc);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("How often, in ticks, an Agent recomputes its "
                          "remaining itinerary.");
    }

    float deviation = simulation.config().pathCostDeviation;
    ImGui::SetNextItemWidth(-140.0f);
    if (ImGui::SliderFloat("cost deviation", &deviation, 0.0f, 1.0f, "%.2f"))
    {
        simulation.config().pathCostDeviation = deviation;
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip(
            "Relative increase of the remaining itinerary cost that forces an\n"
            "Agent to recompute immediately. Zero means always recompute.");
    }

    auto check = int(simulation.config().pathCheckTicks);
    ImGui::SetNextItemWidth(-140.0f);
    if (ImGui::SliderInt("deviation check", &check, 1, 100))
    {
        simulation.config().pathCheckTicks = uint32_t(check);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip(
            "How often, in ticks, an Agent compares its itinerary against the\n"
            "shortest one. That comparison costs a whole graph search, so\n"
            "lowering this is the quickest way to make the simulation crawl.");
    }

    ImGui::SeparatorText("Congestion");
    ImGui::Checkbox("Color roads", &state.showTraffic);
    {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 const origin = ImGui::GetCursorScreenPos();
        float const width = ImGui::GetContentRegionAvail().x;
        float const height = 10.0f;
        for (int i = 0; i < 32; ++i)
        {
            float const t0 = float(i) / 32.0f;
            float const t1 = float(i + 1) / 32.0f;
            drawList->AddRectFilled(
                ImVec2(origin.x + t0 * width, origin.y),
                ImVec2(origin.x + t1 * width, origin.y + height),
                theme::congestionColor(t0));
        }
        ImGui::Dummy(ImVec2(width, height));
        ImGui::TextDisabled("free");
        ImGui::SameLine();
        float const jammedWidth = ImGui::CalcTextSize("jammed").x;
        ImGui::SetCursorPosX(
            ImGui::GetCursorPosX() +
            std::max(0.0f, ImGui::GetContentRegionAvail().x - jammedWidth));
        ImGui::TextDisabled("jammed");
    }

    std::vector<WayRow> rows;
    float totalTime = 0.0f;
    float freeFlowTotal = 0.0f;
    uint32_t travelling = 0u;

    for (auto& it : simulation.cities())
    {
        City& city = *it.second;
        for (auto const& pathIt : city.paths())
        {
            Path const& path = *pathIt.second;
            for (auto const& way : path.ways())
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

    ImGui::Text(
        "%u agent%s on the roads", travelling, (travelling > 1u) ? "s" : "");
    ImGui::Text("Link travel time: %.2f s", totalTime);
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip(
            "Sum over every road segment of (smoothed flow x travel time).\n"
            "Measures congestion load on the network right now, in\n"
            "segment-seconds. Not the same as TSTT below, which counts only\n"
            "the remaining trips of agents on the road.");
    }

    ImGui::SeparatorText("Assignment (sampled agents)");

    float const tstt = simulation.totalSystemTravelTime();
    float const sptt = simulation.shortestPathTravelTime();
    float const gap = simulation.relativeGap();
    RoutingQuality const quality = routingQuality(gap);

    ImGui::Text("Total system travel time: %.2f s", tstt);
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip(
            "TSTT: sum of remaining itinerary costs for a sample of agents,\n"
            "counted from the next crossroads each agent is heading to.");
    }

    ImGui::Text("Shortest path travel time: %.2f s", sptt);
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip(
            "SPTT: what the same sample would pay if every agent rerouted\n"
            "instantly from its next routing crossroads at current travel\n"
            "times.");
    }

    ImGui::Text("Relative gap: %.2f %%", 100.0f * gap);
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip(
            "(TSTT - SPTT) / TSTT, clamped to 0 when SPTT exceeds TSTT.\n"
            "Near zero, agents are on cheapest itineraries.");
    }
    if (sptt > tstt + 1e-3f)
    {
        ImGui::TextDisabled(
            "SPTT > TSTT: fresh reroutes would cost more than finishing\n"
            "current trips (traffic changed since departure, or different\n"
            "destination picked on reroute). Gap shown as 0%%.");
    }

    ImGui::Text("routing quality: %s", quality.label);
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("%s", quality.detail);
    }

    // How much the congestion costs compared to an empty network. Zero means
    // free flow, and a large value means the demand exceeds the capacity.
    float const excess = (freeFlowTotal > 0.0f)
                             ? (totalTime - freeFlowTotal) / freeFlowTotal
                             : 0.0f;
    ImGui::Text("Congestion overhead: %.1f %%", 100.0f * excess);

    ImGui::SeparatorText("Busiest roads");

    std::sort(rows.begin(),
              rows.end(),
              [](WayRow const& a, WayRow const& b)
              { return a.saturation > b.saturation; });

    ImGuiTableFlags const flags =
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
        ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchProp;

    // The panel is docked on a narrow side, so the identity of the segment is
    // kept short and the full breakdown is moved to a tooltip.
    if (ImGui::BeginTable("ways", 4, flags))
    {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("Road");
        ImGui::TableSetupColumn(
            "Agents", ImGuiTableColumnFlags_WidthFixed, 46.0f);
        ImGui::TableSetupColumn("Load");
        ImGui::TableSetupColumn(
            "Time", ImGuiTableColumnFlags_WidthFixed, 54.0f);
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
                                  row.city.c_str(),
                                  row.path.c_str(),
                                  row.type.c_str(),
                                  row.agents,
                                  (row.agents > 1u) ? "s" : "",
                                  row.travelTime,
                                  row.freeFlowTime);
            }

            ImGui::TableNextColumn();
            ImGui::Text("%u", row.agents);

            ImGui::TableNextColumn();
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram,
                                  ImGui::ColorConvertU32ToFloat4(
                                      theme::congestionColor(row.saturation)));
            char overlay[32];
            std::snprintf(
                overlay, sizeof(overlay), "%.0f %%", 100.0f * row.saturation);
            ImGui::ProgressBar(
                std::min(1.0f, row.saturation), ImVec2(-1.0f, 0.0f), overlay);
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
