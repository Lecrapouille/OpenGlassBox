//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "UI/Panels.hpp"
#include "UI/Theme.hpp"
#include "OpenGlassBox/Simulation.hpp"

namespace ogb {
namespace ui {
using namespace ogb::theme;


//! \brief Speeds offered as one-click buttons.
static float const SPEEDS[] = { 0.25f, 0.5f, 1.0f, 2.0f, 4.0f, 8.0f, 16.0f };
static char const* const SPEED_LABELS[] = { "x0.25", "x0.5", "x1", "x2",
                                            "x4", "x8", "x16" };

// ----------------------------------------------------------------------------
uint32_t TimeControlPanel::takePendingSteps()
{
    uint32_t const steps = m_pending_steps;
    m_pending_steps = 0u;
    return steps;
}

// ----------------------------------------------------------------------------
void TimeControlPanel::draw(Simulation& simulation)
{
    if (!ImGui::Begin("Simulation clock"))
    {
        ImGui::End();
        return;
    }

    SimulationClock const& clock = simulation.clock();
    ImGui::Text("%02u:%02u", clock.hourOfDay(), clock.minuteOfHour());
    ImGui::SameLine();
    ImGui::TextDisabled("Jour %u  tick %llu",
                        clock.day(),
                        (unsigned long long)simulation.totalTicks());

    bool const paused = simulation.paused();
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    ImGui::TextWrapped("%s. Play and Pause sit at the top of the map toolbar.",
                       paused ? "Paused" : "Running");
    ImGui::PopStyleColor();

    ImGui::BeginDisabled(!paused);
    if (ImGui::Button("Step"))
    {
        m_pending_steps += uint32_t(m_step_size);
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(110.0f);
    ImGui::SliderInt("##stepsize", &m_step_size, 1, 100, "%d tick(s)");
    ImGui::EndDisabled();
    if (!paused && ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Pause the simulation to step through it.");
    }

    ImGui::SeparatorText("Speed");

    float const scale = simulation.timeScale();
    float const right = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
    for (size_t i = 0u; i < IM_ARRAYSIZE(SPEEDS); ++i)
    {
        if (i != 0u)
        {
            float const width = ImGui::CalcTextSize(SPEED_LABELS[i]).x +
                                2.0f * ImGui::GetStyle().FramePadding.x;
            float const next = ImGui::GetItemRectMax().x +
                               ImGui::GetStyle().ItemSpacing.x + width;
            if (next < right)
            {
                ImGui::SameLine();
            }
        }

        bool const active = (scale == SPEEDS[i]);
        if (active)
        {
            ImGui::PushStyleColor(ImGuiCol_Button,
                                  ImGui::ColorConvertU32ToFloat4(theme::ACCENT));
        }
        if (ImGui::Button(SPEED_LABELS[i]))
        {
            simulation.setTimeScale(SPEEDS[i]);
        }
        if (active)
        {
            ImGui::PopStyleColor();
        }
    }

    ImGui::SeparatorText("Tick rate");

    float ticksPerSecond = simulation.config().ticksPerSecond;
    ImGui::SetNextItemWidth(-140.0f);
    if (ImGui::SliderFloat("ticks per second", &ticksPerSecond, 1.0f, 120.0f,
                           "%.0f"))
    {
        simulation.config().ticksPerSecond = ticksPerSecond;
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip(
            "Rate the rules are run at. Combined with the speed above, this\n"
            "is how many ticks a second of wall clock time is worth.");
    }

    int maxTicks = int(simulation.config().maxTicksPerUpdate);
    ImGui::SetNextItemWidth(-140.0f);
    if (ImGui::SliderInt("max catch-up", &maxTicks, 1, 200))
    {
        simulation.config().maxTicksPerUpdate = uint32_t(maxTicks);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip(
            "Upper bound on the ticks run in a single frame. Without it, a\n"
            "stall would be followed by an unbounded catch-up.");
    }

    ImGui::End();
}
} // namespace ui
} // namespace ogb
