//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file Config.hpp
//! \brief Runtime settings of a Simulation, grouped by topic.

#ifndef OPEN_GLASSBOX_CONFIG_HPP
#define OPEN_GLASSBOX_CONFIG_HPP

#include <cstdint>
#include <limits>

namespace ogb
{

//==============================================================================
//! \brief Default values of the settings.
//!
//! This header is installed with the library. It shall not depend on any macro
//! defined by the build system.
//==============================================================================
namespace defaults
{
//! \brief Simulation steps run per second of game time.
constexpr float TICKS_PER_SECOND = 20.0f;

//! \brief Largest number of ticks caught up by one call to
//! Simulation::update(). Without this bound, a caller that stalled for a long
//! time asks for so many ticks that the next call stalls too.
constexpr uint32_t MAX_TICKS_PER_UPDATE = 20u;

//! \brief Side of a grid cell, in world units. This is how a world position
//! becomes a cell. It is not a number of pixels: the renderer decides how a
//! world unit is drawn.
constexpr float GRID_CELL_SIZE = 30.0f;
} // namespace defaults

//! \brief Cost of a route that does not exist. Uses max() rather than
//! infinity() so the value stays defined when the project is built with
//! -ffast-math (-Wnan-infinity-disabled).
constexpr float ROUTING_INFINITY = std::numeric_limits<float>::max();

//! \brief Whether a routing cost means "no route", including ROUTING_INFINITY.
inline bool routingCostUnreachable(float cost) noexcept
{
    return cost >= ROUTING_INFINITY * 0.5f;
}

//==============================================================================
//! \brief How fast the simulation runs, and how a tick maps to game time.
//==============================================================================
struct TimeConfig
{
    //! \brief Simulation steps run per second of game time. Raise it to make
    //! the simulation run faster.
    float ticksPerSecond = defaults::TICKS_PER_SECOND;
    //! \brief Largest number of ticks caught up by one call to
    //! Simulation::update().
    uint32_t maxTicksPerUpdate = defaults::MAX_TICKS_PER_UPDATE;
    //! \brief Ticks in one game minute. Twenty matches the default tick rate,
    //! so one second of game time lasts one game minute.
    uint32_t ticksPerMinute = 20u;
    //! \brief Hour of the day a new simulation starts at. Starting at midnight
    //! means waiting a third of a day before the rules that keep office hours
    //! do anything.
    uint32_t startHour = 8u;

    //--------------------------------------------------------------------------
    //! \brief \return how long one tick lasts, in seconds of game time.
    //--------------------------------------------------------------------------
    float tickDuration() const
    {
        return 1.0f / ticksPerSecond;
    }
};

//==============================================================================
//! \brief Size of the grid cells, and size of a city that does not give one.
//==============================================================================
struct GridConfig
{
    //! \brief Side of a grid cell, in world units.
    float cellSize = defaults::GRID_CELL_SIZE;
    //! \brief Cells a city spans along U when Simulation::addCity() is called
    //! without a size.
    uint32_t defaultCitySizeU = 32u;
    //! \brief Cells a city spans along V when Simulation::addCity() is called
    //! without a size.
    uint32_t defaultCitySizeV = 32u;
};

//==============================================================================
//! \brief How the traffic on the segments is measured.
//==============================================================================
struct TrafficConfig
{
    //! \brief Step of the moving average that smooths the traffic flow, in
    //! ]0..1]. Small values damp harder. One routes on the raw count, which
    //! makes the population swing between itineraries. See Segment::smoothFlow.
    float smoothing = 0.05f;
    //! \brief Largest number of agents Simulation::getTrafficMetrics() looks
    //! at. That measure costs one graph search per agent, so a large city
    //! would spend more time measuring how settled it is than settling. The
    //! agents are picked at a regular stride over the whole population. Zero
    //! means every agent.
    uint32_t relativeGapSamples = 256u;
};

//==============================================================================
//! \brief When an agent recomputes its itinerary, and when it gives up.
//==============================================================================
struct RoutingConfig
{
    //! \brief Ticks between two full recomputations of the remaining
    //! itinerary. Routing a congested network is expensive, and doing it at
    //! every node makes the population swing from one road to the other.
    uint32_t pathRecalcTicks = 40u;
    //! \brief Ticks between two comparisons of the current itinerary against
    //! the shortest one. That comparison costs a whole graph search, so doing
    //! it on every tick for every agent dwarfs everything else. Zero and one
    //! both mean every tick.
    uint32_t pathCheckTicks = 10u;
    //! \brief How much more expensive the current itinerary has to be, as a
    //! fraction in [0..1], before the agent recomputes at once. Zero means
    //! always recompute.
    float pathCostDeviation = 0.25f;
    //! \brief How long an agent looks for something that accepts its load
    //! before it gives up, in ticks. Two game hours by default. Zero lets it
    //! roam for ever, which piles up agents nothing will ever remove.
    uint32_t agentGiveUpTicks = 2400u;
};

//==============================================================================
//! \brief Runtime settings of a Simulation, shared with its cities and agents.
//!
//! These used to be compilation constants, which made the speed of the
//! simulation impossible to change while it runs, and forced every user of the
//! library to define the same macros as the build system.
//!
//! Nothing here belongs to a ruleset: a script says what a city does, this says
//! how finely and how fast it is simulated. Two settings do reach the rules,
//! and both are conversions rather than behaviour: \c time.ticksPerMinute turns
//! \c rate \c 30 \c minutes into a number of ticks, and \c grid.cellSize turns
//! a world position into the cell whose layers a rule reads.
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
    //! \brief Size of the grid cells and of a new city.
    GridConfig grid;
    //! \brief How the traffic is measured.
    TrafficConfig traffic;
    //! \brief When an agent recomputes its itinerary.
    RoutingConfig routing;
    //! \brief Seed of the random generators. Set it to a fixed value to make a
    //! run reproducible.
    uint32_t randomSeed = 0u;
};

} // namespace ogb

#endif
