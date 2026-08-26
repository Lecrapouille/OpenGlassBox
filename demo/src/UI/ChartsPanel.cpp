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

namespace ogb
{
namespace ui
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
void ChartsPanel::sample(Simulation& simulation)
{
    uint64_t const tick = simulation.totalTicks();

    if (!m_first_sample &&
        (tick < m_last_sample_tick + uint64_t(m_sample_period)))
        return;

    m_first_sample = false;
    m_last_sample_tick = tick;

    float const ticksPerMinute =
        float(std::max(1u, simulation.clock().ticksPerMinute()));
    float const hours = float(tick) / (ticksPerMinute * 60.0f);

    // Totals per map, summed over the cities so that the curve tracks the
    // resource of the whole world.
    std::map<std::string, float> mapTotals;
    std::map<std::string, float> agentCounts;
    std::map<std::string, float> globals;
    float agents = 0.0f;

    for (auto& it : simulation.cities())
    {
        City& city = *it.second;

        for (auto& map : city.maps())
        {
            mapTotals[map.second->type()] += float(map.second->totalResource());
        }

        for (auto& agent : city.agents())
        {
            agentCounts[agent->type()] += 1.0f;
            agents += 1.0f;
        }

        for (Resource const& resource : city.globals().container())
        {
            globals[resource.type()] += float(resource.getAmount());
        }
    }

    for (auto const& it : mapTotals)
    {
        series("Maps", it.first).pushHours(hours, it.second);
    }
    for (auto const& it : agentCounts)
    {
        series("Agents", it.first).pushHours(hours, it.second);
    }
    for (auto const& it : globals)
    {
        series("Globals", it.first).pushHours(hours, it.second);
    }
    series("Agents", "Total").pushHours(hours, agents);

    series("Traffic", "Link travel time")
        .pushHours(hours, TrafficPanel::totalTravelTime(simulation));
    series("Traffic", "TSTT")
        .pushHours(hours, simulation.totalSystemTravelTime());
    series("Traffic", "SPTT")
        .pushHours(hours, simulation.shortestPathTravelTime());
    series("Traffic quality", "relative gap")
        .pushHours(hours, 100.0f * simulation.relativeGap());
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

    for (auto& group : m_series)
    {
        if (!ImGui::CollapsingHeader(group.first.c_str(),
                                     ImGuiTreeNodeFlags_DefaultOpen))
            continue;

        ImGui::PushID(group.first.c_str());
        if (ImPlot::BeginPlot("##plot", ImVec2(-1.0f, 190.0f)))
        {
            char const* yLabel =
                (group.first == "Traffic quality") ? "percent" : nullptr;
            ImPlot::SetupAxes("hours",
                              yLabel,
                              ImPlotAxisFlags_AutoFit,
                              ImPlotAxisFlags_AutoFit);

            for (auto& entry : group.second)
            {
                game::TimeSeries const& serie = entry.second;
                if (serie.empty())
                    continue;

                if (!m_show_raw && !m_show_smoothed)
                    continue;

                if (m_show_raw)
                {
                    ImPlot::PlotLine(serie.name().c_str(),
                                     serie.hours(),
                                     serie.values(),
                                     int(serie.size()));
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

                std::string const trend = serie.name() + " (trend)";
                ImPlot::PlotLine(trend.c_str(),
                                 serie.hours(),
                                 serie.smoothed(),
                                 int(serie.size()),
                                 spec);
            }

            ImPlot::EndPlot();
        }
        ImGui::PopID();
    }

    ImGui::End();
}
} // namespace ui
} // namespace ogb
