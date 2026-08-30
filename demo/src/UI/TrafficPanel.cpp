//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "OpenGlassBox/Simulation.hpp"
#include "UI/Panels.hpp"
#include "UI/Theme.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <vector>

namespace ogb::ui
{
using namespace ogb::theme;

//! \brief Share of its capacity above which a segment is worth counting as
//! busy. Below it the colour of the road already says everything.
static constexpr float CROWDED = 0.8f;

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
//! \brief Open a two column block where the numbers line up on the right edge
//! of the panel, so that a column of figures can be read down rather than
//! hunted for at the end of a sentence.
// ----------------------------------------------------------------------------
static bool beginMetrics(char const* id)
{
    return ImGui::BeginTable(id, 2, ImGuiTableFlags_SizingStretchProp);
}

// ----------------------------------------------------------------------------
//! \brief One measurement: what it is on the left, what it is worth on the
//! right, and what it means under the cursor.
// ----------------------------------------------------------------------------
static void
metric(char const* label, std::string const& value, char const* help = nullptr)
{
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TextDisabled("%s", label);
    bool hovered = ImGui::IsItemHovered();

    ImGui::TableNextColumn();
    float const width = ImGui::CalcTextSize(value.c_str()).x;
    ImGui::SetCursorPosX(
        ImGui::GetCursorPosX() +
        std::max(0.0f, ImGui::GetContentRegionAvail().x - width));
    ImGui::TextUnformatted(value.c_str());
    hovered = hovered || ImGui::IsItemHovered();

    if (hovered && (help != nullptr))
    {
        ImGui::SetTooltip("%s", help);
    }
}

// ----------------------------------------------------------------------------
//! \brief A number with two decimals, since a stream is the only way to get
//! one into a std::string without a scratch buffer at every call site.
// ----------------------------------------------------------------------------
static std::string fixed(float value, int precision)
{
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(precision) << value;
    return stream.str();
}

// ----------------------------------------------------------------------------
//! \brief The scale the roads are painted with, with the two ends named.
// ----------------------------------------------------------------------------
static void drawCongestionScale()
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

// ----------------------------------------------------------------------------
float TrafficPanel::totalTravelTime(Simulation const& simulation)
{
    float total = 0.0f;

    for (auto const& [_, cityPtr] : simulation.getCities())
    {
        for (auto const& [_, path] : cityPtr->getPaths())
        {
            for (auto const& segment : path->getSegments())
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
            "lowering this is the quickest segment to make the simulation "
            "crawl.");
    }

    std::vector<SegmentRow> rows;
    float totalTime = 0.0f;
    float freeFlowTotal = 0.0f;
    float worst = 0.0f;
    uint32_t travelling = 0u;
    uint32_t crowded = 0u;

    for (auto const& [_, cityPtr] : simulation.getCities())
    {
        City const& city = *cityPtr;
        for (auto const& [_, pathPtr] : city.getPaths())
        {
            Path const& path = *pathPtr;
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
                worst = std::max(worst, row.saturation);
                if (row.saturation >= CROWDED)
                    ++crowded;

                rows.push_back(std::move(row));
            }
        }
    }

    ImGui::SeparatorText("Congestion");
    ImGui::Checkbox("Color roads", &state.showTraffic);
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Paint every road with how saturated it is rather\n"
                          "than with the colour of its type.");
    }
    drawCongestionScale();

    if (rows.empty())
    {
        ImGui::TextDisabled("No road in the simulation.");
        ImGui::End();
        return;
    }

    // How much the congestion costs compared to an empty network. Zero means
    // free flow, and a large value means the demand exceeds the capacity.
    float const excess = (freeFlowTotal > 0.0f)
                             ? (totalTime - freeFlowTotal) / freeFlowTotal
                             : 0.0f;

    if (beginMetrics("congestion"))
    {
        metric("Agents on the roads",
               std::to_string(travelling),
               "Agents standing on a segment right now. An agent waiting at a\n"
               "building it could not unload into is not counted.");
        metric("Busiest road",
               fixed(100.0f * worst, 0) + " %",
               "Saturation of the single most loaded segment. The ranking at\n"
               "the bottom of the panel names it.");
        metric(("Roads over " + fixed(100.0f * CROWDED, 0) + " %").c_str(),
               std::to_string(crowded) + " of " + std::to_string(rows.size()),
               "Segments carrying more than that share of what they can take.\n"
               "A handful is a rush hour; most of the network is a shortage\n"
               "of roads.");
        metric("Slowdown",
               fixed(100.0f * excess, 1) + " %",
               "How much longer the trips being made take than they would on\n"
               "an empty network. Zero is free flow.");
        metric("Link travel time",
               fixed(totalTime, 2) + " s",
               "Sum over every road segment of (smoothed flow x travel time).\n"
               "Measures the load on the network right now, in\n"
               "segment-seconds. Not the same as the total system travel time\n"
               "below, which counts only the remaining trips of the agents on\n"
               "the road.");
        ImGui::EndTable();
    }

    ImGui::SeparatorText("Assignment (sampled agents)");

    Simulation::TrafficMetrics const metrics = simulation.getTrafficMetrics();
    float const tstt = metrics.totalTravelTime;
    float const sptt = metrics.shortestPathTime;
    float const gap = metrics.relativeGap;
    RoutingQuality const quality = routingQuality(gap);

    if (beginMetrics("assignment"))
    {
        metric("Total system travel time",
               fixed(tstt, 2) + " s",
               "TSTT: sum of remaining itinerary costs for a sample of\n"
               "agents, counted from the next crossroads each agent is\n"
               "heading to.");
        metric("Shortest path travel time",
               fixed(sptt, 2) + " s",
               "SPTT: what the same sample would pay if every agent rerouted\n"
               "instantly from its next routing crossroads at current travel\n"
               "times.");
        metric("Relative gap",
               fixed(100.0f * gap, 2) + " %",
               "(TSTT - SPTT) / TSTT, clamped to 0 when SPTT exceeds TSTT.\n"
               "Near zero, agents are on cheapest itineraries.");
        metric("Routing quality", quality.label, quality.detail);
        ImGui::EndTable();
    }

    if (sptt > tstt + 1e-3f)
    {
        ImGui::TextDisabled(
            "SPTT > TSTT: fresh reroutes would cost more than finishing\n"
            "current trips. Expected only for agents whose destination\n"
            "filled up while they were driving to it, and whose next best\n"
            "one is farther. Gap shown as 0%%.");
    }

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
} // namespace ogb::ui
