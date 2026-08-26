//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file Config.hpp
//! \brief Default simulation constants and the runtime SimulationConfig
//! settings.

#ifndef OPEN_GLASSBOX_CONFIG_HPP
#define OPEN_GLASSBOX_CONFIG_HPP

#include <cstdint>

namespace ogb
{

//==============================================================================
//! \brief Default values for SimulationConfig.
//!
//! This header is installed with the library and therefore shall not depend on
//! any compilation macro defined by the build system.
//==============================================================================
namespace config
{
//! \brief Default number of simulation steps run per second of game time.
constexpr float DEFAULT_TICKS_PER_SECOND = 20.0f;

//! \brief Default upper bound on the number of ticks caught up during a
//! single call to Simulation::update(). Prevents the spiral of death when
//! the caller has been stalled.
constexpr uint32_t DEFAULT_MAX_TICKS_PER_UPDATE = 20u;

//! \brief Default length of the side of a Map cell, expressed in world
//! units. This is the conversion factor between world positions and grid
//! indices. It is not a number of pixels: how a world unit is rendered is
//! the sole responsibility of the renderer.
constexpr float DEFAULT_GRID_CELL_SIZE = 30.0f;
} // namespace config

//==============================================================================
//! \brief Runtime settings of a Simulation. Passed at construction and shared
//! with the Cities it holds and with their Agents.
//!
//! These used to be compilation constants, which made the simulation speed
//! impossible to change at runtime and forced every consumer of the library to
//! define the same macros as the build system.
//!
//! Nothing here belongs to a ruleset: a script says what a city does, this says
//! how finely and how fast it is simulated. Two settings do reach into the
//! rules, though, and both are conversions rather than behaviour:
//! \c ticksPerMinute turns \c rate \c 30 \c minutes into a number of ticks, and
//! \c gridCellSize turns a world position into the grid cell whose Maps a rule
//! reads.
//!
//! Example:
//! \code
//! ogb::SimulationConfig config;
//! config.randomSeed = 1234u;      // reproducible run
//! config.startHour = 8u;          // open the city as the workers leave
//! config.ticksPerMinute = 20u;    // one second of game time is one minute
//!
//! ogb::Simulation simulation(32u, 32u, config);
//! \endcode
//==============================================================================
struct SimulationConfig
{
    //! \brief Number of simulation steps run per second of game time. Change
    //! this to make the simulation run faster or slower.
    float ticksPerSecond = config::DEFAULT_TICKS_PER_SECOND;
    //! \brief Upper bound on the number of ticks caught up during a single
    //! call to Simulation::update().
    uint32_t maxTicksPerUpdate = config::DEFAULT_MAX_TICKS_PER_UPDATE;
    //! \brief Length of the side of a Map cell, in world units.
    float gridCellSize = config::DEFAULT_GRID_CELL_SIZE;
    //! \brief Seed of the random generators. Set to a fixed value to make a
    //! run reproducible.
    uint32_t randomSeed = 0u;
    //! \brief Step of the moving average smoothing the traffic flow of the
    //! Ways, in ]0..1]. Small values damp harder. Set it to one to route on the
    //! raw instantaneous count, which makes the population oscillate between
    //! itineraries. See Way::smoothFlow.
    float trafficSmoothing = 0.05f;
    //! \brief How many simulation ticks make one game minute. Twenty matches
    //! the default tick rate, so one second of game time is one game minute.
    uint32_t ticksPerMinute = 20u;
    //! \brief Hour of the day a fresh simulation opens at. Starting at midnight
    //! means waiting a third of a day before the rules that keep office hours
    //! do anything at all.
    uint32_t startHour = 8u;
    //! \brief How often an Agent recomputes its remaining itinerary, in ticks.
    //! Routing on a congested network is expensive, and doing it at every node
    //! makes the population swing from one road to the other.
    uint32_t pathRecalcTicks = 40u;
    //! \brief Relative increase of the remaining itinerary cost that forces an
    //! Agent to recompute immediately, in [0..1]. Zero means always recompute.
    float pathCostDeviation = 0.25f;
    //! \brief How often an Agent compares its itinerary against the current
    //! shortest one, in ticks. That comparison costs a whole graph search, so
    //! doing it on every tick for every Agent dwarfs everything else the
    //! simulation does. Zero and one both mean every tick.
    uint32_t pathCheckTicks = 10u;
    //! \brief How long an Agent wanders without finding anything that accepts
    //! its load before it gives up, in ticks. Two game hours by default. Zero
    //! lets it roam for ever, which piles up Agents nothing will ever remove.
    uint32_t agentGiveUpTicks = 2400u;
    //! \brief How many Agents Simulation::relativeGap() examines at most. That
    //! diagnostic costs a whole graph search per Agent, so a large city would
    //! spend more time measuring how settled it is than settling. The Agents
    //! are picked at a regular stride over the population, which samples the
    //! whole of it rather than the beginning. Zero means every Agent.
    uint32_t relativeGapSamples = 256u;

    //--------------------------------------------------------------------------
    //! \brief Duration of a single simulation tick, in seconds of game time.
    //--------------------------------------------------------------------------
    float tickDuration() const
    {
        return 1.0f / ticksPerSecond;
    }
};

} // namespace ogb

#endif
