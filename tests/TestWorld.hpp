//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file TestWorld.hpp
//! \brief Minimal test fixture holding one world and one city for unit tests.

#ifndef OPEN_GLASSBOX_TESTS_TEST_WORLD_HPP
#define OPEN_GLASSBOX_TESTS_TEST_WORLD_HPP

#include "OpenGlassBox/DijkstraRouter.hpp"
#include "OpenGlassBox/World.hpp"

#include <memory>
#include <utility>
#include <vector>

using namespace ogb;

//------------------------------------------------------------------------------
//! \brief Build a script-defined type and keep it alive until the test binary
//! exits.
//!
//! Buildings, Agents, Segments, Paths, Zones and Layers hold their type by reference: one
//! recipe is shared by every entity of that kind, and in a running simulation
//! ScriptDefinitions owns it and outlives every City. A test writing
//! \c city.addBuilding(BuildingType("Home"), node) hands over a temporary that dies at
//! the end of the statement, and the building is left reading freed memory. It
//! worked for years by luck, until a change of layout in Building made one such
//! test read a name that was no longer there.
//!
//! \param[in] args forwarded to the constructor of TYPE.
//! \return a reference that stays valid, as ScriptDefinitions would give.
//------------------------------------------------------------------------------
template <class TYPE, class... ARGS>
static TYPE const& keep(ARGS&&... args)
{
    // Deliberately never emptied: this is the arena of the test binary.
    static std::vector<std::unique_ptr<TYPE>> kept;
    kept.push_back(std::make_unique<TYPE>(std::forward<ARGS>(args)...));
    return *kept.back();
}

//==============================================================================
//! \brief A world holding a single city, so that a test needing just a city can
//! declare one local object. A City belongs to a World and cannot outlive it.
//==============================================================================
struct TestWorld
{
    explicit TestWorld(std::string const& name = "Paris",
                       uint32_t sizeU = 32u,
                       uint32_t sizeV = 32u,
                       Vector3f const& position = {},
                       Config const& config = {})
        : clock(config.time.ticksPerMinute),
          world(config, clock),
          city(world.addCity(name, position, sizeU, sizeV))
    {
        clock.setTimeOfDay(0u, config.time.startHour, 0u);
        installDijkstraRouter(city, world.getConfig());
    }

    SimulationClock clock;
    World world;
    City& city;
};

#endif
