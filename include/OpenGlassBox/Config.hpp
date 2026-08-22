//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#ifndef OPEN_GLASSBOX_CONFIG_HPP
#  define OPEN_GLASSBOX_CONFIG_HPP

#include <cstdint>

namespace ogb {

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
}

//==============================================================================
//! \brief Runtime settings of a Simulation. Passed at construction and shared
//! with the Cities it holds.
//!
//! These used to be compilation constants, which made the simulation speed
//! impossible to change at runtime and forced every consumer of the library to
//! define the same macros as the build system.
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

    //! \brief How often an Agent recomputes its remaining itinerary, in ticks.
    //! Routing on a congested network is expensive, and doing it at every node
    //! makes the population swing from one road to the other.
    uint32_t pathRecalcTicks = 40u;

    //! \brief Relative increase of the remaining itinerary cost that forces an
    //! Agent to recompute immediately, in [0..1]. Zero means always recompute.
    float pathCostDeviation = 0.25f;

    //--------------------------------------------------------------------------
    //! \brief Duration of a single simulation tick, in seconds of game time.
    //--------------------------------------------------------------------------
    float tickDuration() const { return 1.0f / ticksPerSecond; }
};

} // namespace ogb

#endif
