//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "OpenGlassBox/Simulation.hpp"
#include "UI/Panels.hpp"
#include "UI/Theme.hpp"

#include <implot.h>

#include <algorithm>
#include <map>

namespace ogb::ui
{

// ----------------------------------------------------------------------------
game::TimeSeries& ChartsPanel::series(std::string const& group,
                                      std::string const& name)
{
    auto& groupSeries = m_series[group];
    auto const it = groupSeries.find(name);
    if (it != groupSeries.end())
        return it->second;

    return groupSeries.emplace(name, game::TimeSeries(name)).first->second;
}

// ----------------------------------------------------------------------------
void ChartsPanel::clear()
{
    m_series.clear();
    m_first_sample = true;
}

// ----------------------------------------------------------------------------
void ChartsPanel::sample(Simulation const& simulation)
{
    uint64_t const tick = simulation.getClock().getTicks();

    if (!m_first_sample &&
        (tick < m_last_sample_tick + uint64_t(m_sample_period)))
        return;

    m_first_sample = false;
    m_last_sample_tick = tick;

    float const ticksPerMinute =
        float(std::max(1u, simulation.getClock().getTicksPerMinute()));
    float const hours = float(tick) / (ticksPerMinute * 60.0f);

    // Totals per layer, summed over the cities so that the curve tracks the
    // resource of the whole world.
    std::map<std::string, float> layerTotals;
    std::map<std::string, float> agentCounts;
    std::map<std::string, float> globals;
    float agents = 0.0f;

    for (auto const& [_, cityPtr] : simulation.getCities())
    {
        City const& city = *cityPtr;

        for (auto const& [_, layer] : city.getLayers())
        {
            layerTotals[layer->getTypeName().str()] +=
                float(layer->getTotalResource());
        }

        for (auto const& agent : city.getAgents())
        {
            agentCounts[agent->getTypeName().str()] += 1.0f;
            agents += 1.0f;
        }

        for (Resource const& resource : city.getGlobals().getAll())
        {
            globals[resource.getTypeName().str()] +=
                float(resource.getAmount());
        }
    }

    for (auto const& [name, total] : layerTotals)
    {
        series("Layers", name).pushHours(hours, total);
    }
    for (auto const& [name, count] : agentCounts)
    {
        series("Agents", name).pushHours(hours, count);
    }
    for (auto const& [name, amount] : globals)
    {
        series("Globals", name).pushHours(hours, amount);
    }
    series("Agents", "Total").pushHours(hours, agents);

    series("Traffic", "Link travel time")
        .pushHours(hours, TrafficPanel::totalTravelTime(simulation));
    Simulation::TrafficMetrics const traffic = simulation.getTrafficMetrics();
    series("Traffic", "TSTT").pushHours(hours, traffic.totalTravelTime);
    series("Traffic", "SPTT").pushHours(hours, traffic.shortestPathTime);
    series("Traffic quality", "relative gap")
        .pushHours(hours, 100.0f * traffic.relativeGap);
}

// ----------------------------------------------------------------------------
void ChartsPanel::draw(Simulation& simulation, game::DebugState& state)
{
    if (!ImGui::Begin("Charts"))
    {
        ImGui::End();
        return;
    }

    ImGui::SetNextItemWidth(160.0f);
    ImGui::SliderInt("Sample every", &m_sample_period, 1, 60, "%d ticks");
    ImGui::SameLine();
    ImGui::Checkbox("Raw", &m_show_raw);
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Draw the sampled values as recorded.\n"
                          "Step-shaped when rules fire in bursts.");
    }
    ImGui::SameLine();
    ImGui::Checkbox("Trend", &m_show_smoothed);
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip(
            "Draw the exponential moving average of each series.\n"
            "Reading aid only: the simulation never sees it.");
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear history"))
    {
        clear();
    }

    if (!m_show_raw && !m_show_smoothed)
    {
        ImGui::TextDisabled("Enable Raw or Trend to display curves.");
    }

    if (m_series.empty())
    {
        ImGui::TextDisabled("Run the simulation to collect samples.");
        ImGui::End();
        return;
    }

    for (auto const& [groupName, groupSeries] : m_series)
    {
        if (!ImGui::CollapsingHeader(groupName.c_str(),
                                     ImGuiTreeNodeFlags_DefaultOpen))
            continue;

        ImGui::PushID(groupName.c_str());
        if (ImPlot::BeginPlot("##plot", ImVec2(-1.0f, 190.0f)))
        {
            char const* yLabel =
                (groupName == "Traffic quality") ? "percent" : nullptr;
            ImPlot::SetupAxes("hours",
                              yLabel,
                              ImPlotAxisFlags_AutoFit,
                              ImPlotAxisFlags_AutoFit);

            for (auto const& [_, serie] : groupSeries)
            {
                game::TimeSeries const& series = serie;
                if (series.empty())
                    continue;

                if (!m_show_raw && !m_show_smoothed)
                    continue;

                if (m_show_raw)
                {
                    ImPlot::PlotLine(series.name().c_str(),
                                     series.hours(),
                                     series.values(),
                                     int(series.size()));
                }

                if (!m_show_smoothed)
                    continue;

                // Same colour as the raw curve when both are shown, so that the
                // two read as one quantity rather than as two.
                ImPlotSpec spec;
                if (m_show_raw)
                {
                    spec.LineColor = ImPlot::GetLastItemColor();
                }
                spec.LineWeight = 2.0f;

                std::string const trend = series.name() + " (trend)";
                ImPlot::PlotLine(trend.c_str(),
                                 series.hours(),
                                 series.smoothed(),
                                 int(series.size()),
                                 spec);
            }

            ImPlot::EndPlot();
        }
        ImGui::PopID();
    }

    ImGui::End();
}
} // namespace ogb::ui
