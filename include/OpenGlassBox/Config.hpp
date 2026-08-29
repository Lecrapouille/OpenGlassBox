//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file Config.hpp
//! \brief Runtime settings for a Simulation, grouped by topic.

#ifndef OPEN_GLASSBOX_CONFIG_HPP
#define OPEN_GLASSBOX_CONFIG_HPP

#include <cstdint>
#include <limits>

namespace ogb
{

//==============================================================================
//! \brief Default setting values.
//!
//! This header is installed with the library. It must not depend on build macros.
//==============================================================================
namespace defaults
{
//! \brief Simulation ticks per second of game time.
constexpr float TICKS_PER_SECOND = 20.0f;

//! \brief Max ticks processed in one Simulation::update() call.
//! Without this limit, a long pause would trigger too many ticks at once.
constexpr uint32_t MAX_TICKS_PER_UPDATE = 20u;

//! \brief Side of one grid cell, in world units.
//! World position divided by this value gives a Cell.
//! This is not pixels: the renderer chooses how to draw world units.
constexpr float GRID_CELL_SIZE = 30.0f;
} // namespace defaults

//! \brief Cost for a route that does not exist.
//! Uses max() instead of infinity() so it stays valid with -ffast-math
//! (-Wnan-infinity-disabled).
constexpr float ROUTING_INFINITY = std::numeric_limits<float>::max();

//! \brief Return true if a routing cost means "no route", including ROUTING_INFINITY.
inline bool routingCostUnreachable(float cost) noexcept
{
    return cost >= ROUTING_INFINITY * 0.5f;
}

//==============================================================================
//! \brief Simulation speed and the mapping from ticks to game time.
//==============================================================================
struct TimeConfig
{
    //! \brief Ticks per second of game time. Higher values run the simulation faster.
    float ticksPerSecond = defaults::TICKS_PER_SECOND;
    //! \brief Max ticks processed in one Simulation::update() call.
    uint32_t maxTicksPerUpdate = defaults::MAX_TICKS_PER_UPDATE;
    //! \brief Ticks in one game minute. Default 20 matches the default tick rate:
    //! one second of game time equals one game minute.
    uint32_t ticksPerMinute = 20u;
    //! \brief Hour when a new simulation starts.
    //! Midnight would delay Rules that depend on office hours.
    uint32_t startHour = 8u;

    //--------------------------------------------------------------------------
    //! \return the duration of one tick, in seconds of game time.
    //--------------------------------------------------------------------------
    float tickDuration() const
    {
        return 1.0f / ticksPerSecond;
    }
};

//==============================================================================
//! \brief Grid cell size and default city size.
//==============================================================================
struct GridConfig
{
    //! \brief Side of a grid cell, in world units.
    float cellSize = defaults::GRID_CELL_SIZE;
    //! \brief Default city width in cells when Simulation::addCity() gets no size.
    uint32_t defaultCitySizeU = 32u;
    //! \brief Default city height in cells when Simulation::addCity() gets no size.
    uint32_t defaultCitySizeV = 32u;
};

//==============================================================================
//! \brief How traffic on Segments is measured.
//==============================================================================
struct TrafficConfig
{
    //! \brief Smoothing factor for traffic flow, in ]0..1].
    //! Lower values smooth more. Routing uses the raw count.
    //! See Segment::smoothFlow.
    float smoothing = 0.05f;
    //! \brief Max Agents sampled by Simulation::getTrafficMetrics().
    //! Each sample needs one graph search. Zero means all Agents.
    uint32_t relativeGapSamples = 256u;
};

//==============================================================================
//! \brief When an Agent recomputes its path and when it gives up.
//==============================================================================
struct RoutingConfig
{
    //! \brief Ticks between full path recomputations.
    //! Routing a busy network is expensive. Too often causes route switching.
    uint32_t pathRecalcTicks = 40u;
    //! \brief Ticks between checks of current path vs shortest path.
    //! Each check costs one graph search. Zero and one mean every tick.
    uint32_t pathCheckTicks = 10u;
    //! \brief Extra cost fraction in [0..1] before the Agent recomputes at once.
    //! Zero means always recompute.
    float pathCostDeviation = 0.25f;
    //! \brief Ticks before an Agent gives up finding a destination.
    //! Default is two game hours. Zero means never give up.
    uint32_t agentGiveUpTicks = 2400u;
};

//==============================================================================
//! \brief Runtime settings for a Simulation, shared with its cities and Agents.
//!
//! These used to be compile-time constants. Now you can change speed at runtime
//! without rebuilding or defining the same macros as the build system.
//!
//! Nothing here belongs to a ruleset. The script defines city behaviour.
//! Config defines how fast and how finely the simulation runs.
//! Two settings reach Rules as conversions only: \c time.ticksPerMinute turns
//! \c rate \c 30 \c minutes into ticks, and \c grid.cellSize turns a world
//! position into the Cell whose Layers a Rule reads.
//!
//! Example:
//! \code
//! ogb::Config config;
//! config.randomSeed = 1234u;         // reproducible run
//! config.time.startHour = 8u;        // start as the workers leave
//! config.time.ticksPerMinute = 20u;  // one second of game time is one minute
//!
//! ogb::Simulation simulation(config);
//! \endcode
//==============================================================================
struct Config
{
    //! \brief How fast the simulation runs.
    TimeConfig time;
    //! \brief Grid cell size and default city size.
    GridConfig grid;
    //! \brief How traffic is measured.
    TrafficConfig traffic;
    //! \brief When an Agent recomputes its path.
    RoutingConfig routing;
    //! \brief Random seed. Set a fixed value for a reproducible run.
    uint32_t randomSeed = 0u;
};

} // namespace ogb

#endif
