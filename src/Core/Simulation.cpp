//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "Core/Simulation.hpp"

#define MAX_ITERATIONS_PER_UPDATE 20u
#define TICKS_PER_SECOND 200.0f

//------------------------------------------------------------------------------
Simulation::Simulation(uint32_t gridSizeU, uint32_t gridSizeV)
    : m_gridSizeU(gridSizeU),
      m_gridSizeV(gridSizeV)
{
    static Simulation::Listener listener;
    setListener(listener);
}

// -----------------------------------------------------------------------------
void Simulation::setListener(Simulation::Listener& listener)
{
    m_listener = &listener;
}

//------------------------------------------------------------------------------
void Simulation::update(float const deltaTime)
{
    m_time += deltaTime;

    // Rules are execute at TICKS_PER_SECOND intervals
    uint32_t maxIterations = MAX_ITERATIONS_PER_UPDATE;
    while ((m_time >= 1.0f / TICKS_PER_SECOND) && (maxIterations-- > 0u))
    {
        m_time -= 1.0f / TICKS_PER_SECOND;

        for (auto& it: m_cities) {
            it.second->update();
        }
    }
}

//------------------------------------------------------------------------------
City& Simulation::addCity(std::string const& name, Vector3f position)
{
    City& city = *(m_cities[name] = std::make_unique<City>(name, position, m_gridSizeU, m_gridSizeV));
    m_listener->onCityAdded(city);
    return city;
}

//------------------------------------------------------------------------------
City& Simulation::getCity(std::string const& name)
{
    return *m_cities.at(name);
}

//------------------------------------------------------------------------------
City const& Simulation::getCity(std::string const& name) const
{
    return *m_cities.at(name);
}
