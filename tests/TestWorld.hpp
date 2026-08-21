//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#ifndef OPEN_GLASSBOX_TESTS_TEST_WORLD_HPP
#  define OPEN_GLASSBOX_TESTS_TEST_WORLD_HPP

#  include "OpenGlassBox/World.hpp"

//==============================================================================
//! \brief A world holding a single city, so that a test needing just a city can
//! declare one local object. A City belongs to a World and cannot outlive it.
//==============================================================================
struct TestWorld
{
    explicit TestWorld(std::string const& name = "Paris", uint32_t sizeU = 32u,
                       uint32_t sizeV = 32u,
                       Vector3f const& position = Vector3f(0.0f, 0.0f, 0.0f),
                       SimulationConfig const& config = {})
        : world(config), city(world.addCity(name, position, sizeU, sizeV))
    {}

    World world;
    City& city;
};

#endif
