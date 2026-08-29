//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "OpenGlassBox/Simulation.hpp"
#include "UI/Panels.hpp"
#include "UI/Theme.hpp"
#include <algorithm>

namespace ogb
{
namespace ui
{
using namespace ogb::theme;

//! \brief Speeds offered as one-click buttons.
static float const SPEEDS[] = { 0.25f, 0.5f, 1.0f,  2.0f,
                                4.0f,  8.0f, 16.0f, 32.0f };
static char const* const SPEED_LABELS[] = { "x0.25", "x0.5", "x1",  "x2",
                                            "x4",    "x8",   "x16", "x32" };

// ----------------------------------------------------------------------------
uint32_t TimeControlPanel::takePendingSteps()
{
    uint32_t const steps = m_pending_steps;
    m_pending_steps = 0u;
    return steps;
}

// ----------------------------------------------------------------------------
//! \brief Move the calendar where the player wants it. A save carries the tick
//! counter it was written with, and a rule such as "hour between 8 18" does
//! nothing at all before eight, so without this the only way to see the city
//! commute was to wait a third of a game day.
// ----------------------------------------------------------------------------
void TimeControlPanel::drawTimeOfDay(Simulation& simulation)
{
    SimulationClock const& clock = simulation.getClock();

    // The two fields follow the running clock until the player touches one of
    // them, and then stay where they were left until the jump is asked for or
    // dropped. Following the clock again as soon as the drag ends is what used
    // to make the button set the time it already was, one frame later.
    if (!m_editing_time)
    {
        m_hour = int(clock.getHourOfDay());
        m_minute = int(clock.getMinuteOfHour());
    }

    ImGui::SetNextItemWidth(60.0f);
    if (ImGui::DragInt("##hour", &m_hour, 0.1f, 0, 23, "%02dh"))
        m_editing_time = true;
    ImGui::SameLine();
    ImGui::SetNextItemWidth(60.0f);
    if (ImGui::DragInt("##minute", &m_minute, 0.5f, 0, 59, "%02dm"))
        m_editing_time = true;

    ImGui::SameLine();
    if (m_editing_time)
    {
        ImGui::PushStyleColor(ImGuiCol_Button,
                              ImGui::ColorConvertU32ToFloat4(theme::ACCENT));
    }
    if (ImGui::Button("Set time"))
    {
        simulation.setTimeOfDay(
            clock.getDay(), uint32_t(m_hour), uint32_t(m_minute));
        m_editing_time = false;
    }
    if (m_editing_time)
    {
        ImGui::PopStyleColor();
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Jump the calendar to that time of day, keeping the\n"
                          "current day. Rules that keep office hours only run\n"
                          "inside their window.\n"
                          "Drag the two fields, then press this.");
    }

    ImGui::SameLine();
    ImGui::BeginDisabled(!m_editing_time);
    if (ImGui::Button("Cancel"))
    {
        m_editing_time = false;
    }
    ImGui::EndDisabled();
    if (m_editing_time && ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Drop what was typed and follow the clock again.");
    }
}

// ----------------------------------------------------------------------------
void TimeControlPanel::draw(Simulation& simulation)
{
    if (!ImGui::Begin("Simulation clock"))
    {
        ImGui::End();
        return;
    }

    SimulationClock const& clock = simulation.getClock();
    bool const paused = simulation.isPaused();

    ImGui::SeparatorText("Day");

    ImGui::Text("Day %u", clock.getDay());
    ImGui::SameLine();
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(theme::ACCENT),
                       "%02u:%02u",
                       clock.getHourOfDay(),
                       clock.getMinuteOfHour());
    ImGui::SameLine();
    ImGui::TextDisabled("tick %llu", (unsigned long long)clock.getTicks());
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Ticks since the city was founded. A save carries\n"
                          "this counter, so an opened city resumes its day.");
    }

    drawTimeOfDay(simulation);

    ImGui::SeparatorText("Step");

    uint32_t const perMinute =
        std::max(1u, simulation.getConfig().time.ticksPerMinute);

    if (paused)
    {
        ImGui::TextDisabled("Paused. Run a fixed number of ticks:");
    }
    else
    {
        ImGui::TextDisabled("Running. Pause to step through the ticks.");
    }

    ImGui::BeginDisabled(!paused);
    if (ImGui::Button("Step"))
    {
        m_pending_steps += uint32_t(m_step_size);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Run the ticks one after the other, so a rule that\n"
                          "fires every three ticks and one that fires every\n"
                          "seven both land on their own tick.");
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(130.0f);
    ImGui::SliderInt("##stepsize", &m_step_size, 1, 100, "run %d tick(s)");
    ImGui::SameLine();
    if (ImGui::Button("1 min"))
    {
        m_pending_steps += perMinute;
    }
    ImGui::SameLine();
    if (ImGui::Button("1 h"))
    {
        m_pending_steps += perMinute * 60u;
    }
    ImGui::EndDisabled();

    ImGui::SeparatorText("Speed");

    if (!paused)
    {
        ImGui::PushStyleColor(ImGuiCol_Button,
                              ImGui::ColorConvertU32ToFloat4(theme::SUCCESS));
    }
    if (ImGui::Button(paused ? "Play" : "Pause"))
    {
        simulation.setPaused(!paused);
    }
    if (!paused)
    {
        ImGui::PopStyleColor();
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Pause or resume the simulation. Shortcut: space.\n"
                          "The same button sits on the toolbar of the city.");
    }

    float const scale = simulation.getTimeScale();
    float const right =
        ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
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
            ImGui::PushStyleColor(
                ImGuiCol_Button, ImGui::ColorConvertU32ToFloat4(theme::ACCENT));
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

    Simulation::Config config = simulation.getConfig();

    float ticksPerSecond = config.time.ticksPerSecond;
    ImGui::SetNextItemWidth(-140.0f);
    if (ImGui::SliderFloat(
            "ticks per second", &ticksPerSecond, 1.0f, 120.0f, "%.0f"))
    {
        config.time.ticksPerSecond = ticksPerSecond;
        simulation.setConfig(config);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip(
            "Rate the rules are run at. Combined with the speed above, this\n"
            "is how many ticks a second of wall clock time is worth.");
    }

    int maxTicks = int(config.time.maxTicksPerUpdate);
    ImGui::SetNextItemWidth(-140.0f);
    if (ImGui::SliderInt("max catch-up", &maxTicks, 1, 200))
    {
        config.time.maxTicksPerUpdate = uint32_t(maxTicks);
        simulation.setConfig(config);
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
