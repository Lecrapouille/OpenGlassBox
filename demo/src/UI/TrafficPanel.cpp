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
struct SegmentRow
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

    for (auto& it : simulation.getCities())
    {
        for (auto const& path : it.second->getPaths())
        {
            for (auto& segment : path.second->getSegments())
            {
                total += segment->getFlow() * segment->getTravelTime();
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

    // The settings are read from the simulation and written back as a whole:
    // Simulation::getConfig() hands out a const reference on purpose.
    Simulation::Config config = simulation.getConfig();

    float smoothing = config.traffic.smoothing;
    ImGui::SetNextItemWidth(-140.0f);
    if (ImGui::SliderFloat("flow smoothing",
                           &smoothing,
                           0.005f,
                           1.0f,
                           "%.3f",
                           ImGuiSliderFlags_Logarithmic))
    {
        config.traffic.smoothing = smoothing;
        simulation.setConfig(config);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip(
            "Step of the moving average applied to the traffic flow.\n"
            "Set it to one to route on the raw instantaneous count.");
    }

    auto recalc = int(config.routing.pathRecalcTicks);
    ImGui::SetNextItemWidth(-140.0f);
    if (ImGui::SliderInt("path recalc", &recalc, 1, 400))
    {
        config.routing.pathRecalcTicks = uint32_t(recalc);
        simulation.setConfig(config);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("How often, in ticks, an Agent recomputes its "
                          "remaining itinerary.");
    }

    float deviation = config.routing.pathCostDeviation;
    ImGui::SetNextItemWidth(-140.0f);
    if (ImGui::SliderFloat("cost deviation", &deviation, 0.0f, 1.0f, "%.2f"))
    {
        config.routing.pathCostDeviation = deviation;
        simulation.setConfig(config);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip(
            "Relative increase of the remaining itinerary cost that forces an\n"
            "Agent to recompute immediately. Zero means always recompute.");
    }

    auto check = int(config.routing.pathCheckTicks);
    ImGui::SetNextItemWidth(-140.0f);
    if (ImGui::SliderInt("deviation check", &check, 1, 100))
    {
        config.routing.pathCheckTicks = uint32_t(check);
        simulation.setConfig(config);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip(
            "How often, in ticks, an Agent compares its itinerary against the\n"
            "shortest one. That comparison costs a whole graph search, so\n"
            "lowering this is the quickest segment to make the simulation crawl.");
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

    std::vector<SegmentRow> rows;
    float totalTime = 0.0f;
    float freeFlowTotal = 0.0f;
    uint32_t travelling = 0u;

    for (auto& it : simulation.getCities())
    {
        City& city = *it.second;
        for (auto const& pathIt : city.getPaths())
        {
            Path const& path = *pathIt.second;
            for (auto const& segment : path.getSegments())
            {
                SegmentRow row;
                row.city = city.getName();
                row.path = path.getTypeName().str();
                row.type = segment->getTypeName().str();
                row.saturation = segment->getSaturation();
                row.travelTime = segment->getTravelTime();
                row.freeFlowTime = segment->getFreeFlowTime();
                row.agents = segment->getAgentCount();

                totalTime += segment->getFlow() * row.travelTime;
                freeFlowTotal += segment->getFlow() * row.freeFlowTime;
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

    Simulation::TrafficMetrics const metrics = simulation.getTrafficMetrics();
    float const tstt = metrics.totalTravelTime;
    float const sptt = metrics.shortestPathTime;
    float const gap = metrics.relativeGap;
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
            "current trips. Expected only for agents whose destination\n"
            "filled up while they were driving to it, and whose next best\n"
            "one is farther. Gap shown as 0%%.");
    }

    ImGui::Text("Routing quality: %s", quality.label);
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
              [](SegmentRow const& a, SegmentRow const& b)
              { return a.saturation > b.saturation; });

    ImGuiTableFlags const flags =
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
        ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchProp;

    // The panel is docked on a narrow side, so the identity of the segment is
    // kept short and the full breakdown is moved to a tooltip.
    if (ImGui::BeginTable("segments", 4, flags))
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
            SegmentRow const& row = rows[i];

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
