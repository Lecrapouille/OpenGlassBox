//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#ifndef OPEN_GLASSBOX_SIMULATION_HPP
#define OPEN_GLASSBOX_SIMULATION_HPP

#  include "Core/City.hpp"

//==============================================================================
//! \brief Entry point class managing (add, get, remove) a collection of Cities
//! and running simulation on them.
//! In this current phase of developement Cities are not connected between them.
//==============================================================================
class Simulation
{
public:

    Simulation(uint32_t gridSizeU = 32u, uint32_t gridSizeV = 32u);
    void load(std::string const& file);
    void update(float const deltaTime);
    City& addCity(std::string const& id, Vector3f position);
    City& getCity(std::string const& id);
    City const& getCity(std::string const& id) const;

    Cities const& cities() const { return m_cities; }

private:

    uint32_t      m_gridSizeU;
    uint32_t      m_gridSizeV;
    float         m_time = 0.0f;
    Cities        m_cities;
};

#endif
