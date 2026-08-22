//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "UI/Panels.hpp"
#include "UI/Theme.hpp"
#include "OpenGlassBox/Simulation.hpp"

#include <implot.h>

#include <map>

namespace ogb {
namespace ui {


// ----------------------------------------------------------------------------
core::TimeSeries& ChartsPanel::series(std::string const& group, std::string const& name)
{
    auto& groupSeries = m_series[group];
    auto const it = groupSeries.find(name);
    if (it != groupSeries.end())
        return it->second;

    return groupSeries.emplace(name, core::TimeSeries(name)).first->second;
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

    // Totals per map, summed over the cities so that the curve tracks the
    // resource of the whole world.
    std::map<std::string, float> mapTotals;
    std::map<std::string, float> agentCounts;
    std::map<std::string, float> globals;
    float agents = 0.0f;

    for (auto& it: simulation.cities())
    {
        City& city = *it.second;

        for (auto& map: city.maps())
        {
            mapTotals[map.second->type()] += float(map.second->totalResource());
        }

        for (auto& agent: city.agents())
        {
            agentCounts[agent->type()] += 1.0f;
            agents += 1.0f;
        }

        for (Resource const& resource: city.globals().container())
        {
            globals[resource.type()] += float(resource.getAmount());
        }
    }

    for (auto const& it: mapTotals)
    {
        series("Maps", it.first).push(tick, it.second);
    }
    for (auto const& it: agentCounts)
    {
        series("Agents", it.first).push(tick, it.second);
    }
    for (auto const& it: globals)
    {
        series("Globals", it.first).push(tick, it.second);
    }

    series("Agents", "total").push(tick, agents);
    series("Traffic", "total travel time")
        .push(tick, TrafficPanel::totalTravelTime(simulation));
}

// ----------------------------------------------------------------------------
void ChartsPanel::draw(Simulation& simulation, core::DebugState& state)
{
    if (!ImGui::Begin("Charts"))
    {
        ImGui::End();
        return;
    }

    ImGui::SetNextItemWidth(160.0f);
    ImGui::SliderInt("sample every", &m_sample_period, 1, 60, "%d ticks");
    ImGui::SameLine();
    if (ImGui::Button("Clear history"))
    {
        clear();
    }

    if (m_series.empty())
    {
        ImGui::TextDisabled("Run the simulation to collect samples.");
        ImGui::End();
        return;
    }

    for (auto& group: m_series)
    {
        if (!ImGui::CollapsingHeader(group.first.c_str(),
                                     ImGuiTreeNodeFlags_DefaultOpen))
            continue;

        ImGui::PushID(group.first.c_str());
        if (ImPlot::BeginPlot("##plot", ImVec2(-1.0f, 190.0f)))
        {
            ImPlot::SetupAxes("tick", nullptr,
                              ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);

            for (auto& entry: group.second)
            {
                core::TimeSeries const& serie = entry.second;
                if (serie.empty())
                    continue;

                ImPlot::PlotLine(serie.name().c_str(), serie.ticks(),
                                 serie.values(), int(serie.size()));
            }

            ImPlot::EndPlot();
        }
        ImGui::PopID();
    }

    ImGui::End();
}
} // namespace ui
} // namespace ogb
