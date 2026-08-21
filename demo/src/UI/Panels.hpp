//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#ifndef OPEN_GLASSBOX_DEMO_PANELS_HPP
#  define OPEN_GLASSBOX_DEMO_PANELS_HPP

#  include "Application/OpenGL.hpp"
#  include "Core/DebugState.hpp"
#  include "Core/TimeSeries.hpp"

#  include <map>
#  include <memory>
#  include <string>
#  include <vector>

class Simulation;

namespace ogb {

class RuleTrace;
class CityViewer;

// ****************************************************************************
//! \brief Lists every Map of every City and lets one be designated as the main
//! heatmap while the others become overlays.
// ****************************************************************************
class LayersPanel
{
public:

    void draw(Simulation& simulation, DebugState& state);
};

// ****************************************************************************
//! \brief Details whatever is selected: resources as progress bars, rules with
//! their period and the number of ticks left before the next attempt.
// ****************************************************************************
class InspectorPanel
{
public:

    void draw(Simulation& simulation, DebugState& state, RuleTrace const& trace);

private:

    void drawUnit(Simulation& simulation, DebugState& state,
                  RuleTrace const& trace);
    void drawAgent(Simulation& simulation, DebugState& state);
    void drawNode(DebugState& state);
    void drawCell(Simulation& simulation, DebugState& state);
};

// ****************************************************************************
//! \brief The filterable log of the rule executions.
//!
//! The engine validates every command of a rule and silently gives up on the
//! first refusal, so without this panel there is no way to know why a
//! simulation stays still. Each line names the command that blocked.
// ****************************************************************************
class RuleLogPanel
{
public:

    void draw(Simulation& simulation, DebugState& state, RuleTrace& trace);

private:

    //! \brief Case insensitive substring the entity or the rule must contain.
    char m_filter[64] = "";
    bool m_show_success = true;
    bool m_show_failure = true;
    bool m_auto_scroll = true;
};

// ****************************************************************************
//! \brief Plots the history of the aggregates of the simulation with ImPlot.
// ****************************************************************************
class ChartsPanel
{
public:

    // ------------------------------------------------------------------------
    //! \brief Append one sample of every tracked quantity. Called once per
    //! simulation tick, not once per frame, so that the abscissa is the tick.
    // ------------------------------------------------------------------------
    void sample(Simulation& simulation);

    void draw(Simulation& simulation, DebugState& state);

    void clear();

private:

    TimeSeries& series(std::string const& group, std::string const& name);

private:

    //! \brief Histories by group ("Maps", "Agents", "Globals", "Traffic") then
    //! by quantity name.
    std::map<std::string, std::map<std::string, TimeSeries>> m_series;
    //! \brief Interval in ticks between two samples, to keep long runs cheap.
    int m_sample_period = 5;
    uint64_t m_last_sample_tick = 0u;
    bool m_first_sample = true;
};

// ****************************************************************************
//! \brief Pause, step and speed of the simulation, plus the traffic tuning.
// ****************************************************************************
class TimeControlPanel
{
public:

    void draw(Simulation& simulation, DebugState& state, RuleTrace& trace);

    // ------------------------------------------------------------------------
    //! \brief Number of ticks the user asked to run while paused. Consumed by
    //! the host, which is the only one allowed to advance the simulation.
    // ------------------------------------------------------------------------
    uint32_t takePendingSteps();

private:

    uint32_t m_pending_steps = 0u;
    int m_step_size = 1;
};

// ****************************************************************************
//! \brief Ranks the most saturated Ways and reports whether the network is
//! settling down.
// ****************************************************************************
class TrafficPanel
{
public:

    void draw(Simulation& simulation, DebugState& state);

    // ------------------------------------------------------------------------
    //! \brief Total travel time of the network, the sum over the Ways of the
    //! flow times the travel time. This is the quantity the Wardrop equilibrium
    //! minimizes, so watching it settle tells whether the routing converged.
    // ------------------------------------------------------------------------
    static float totalTravelTime(Simulation& simulation);
};

} // namespace ogb

#endif
