//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "OpenGlassBox/Simulation.hpp"
#include "OpenGlassBox/Agent.hpp"
#include "OpenGlassBox/Dijkstra.hpp"

#include <cmath>

//------------------------------------------------------------------------------
Simulation::Simulation(uint32_t gridSizeU, uint32_t gridSizeV,
                       SimulationConfig const& config)
    : m_world(config),
      m_gridSizeU(gridSizeU),
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
void Simulation::stepOneTick()
{
    m_world.update(config().tickDuration());
    ++m_totalTicks;
}

//------------------------------------------------------------------------------
void Simulation::update(float const deltaTime)
{
    if (m_paused)
        return;

    m_time += deltaTime * m_timeScale;

    float const tick = config().tickDuration();
    uint32_t maxIterations = config().maxTicksPerUpdate;
    while ((m_time >= tick) && (maxIterations-- > 0u))
    {
        m_time -= tick;
        stepOneTick();
    }
}

//------------------------------------------------------------------------------
float Simulation::relativeGap() const
{
    float tstt = 0.0f;
    float sptt = 0.0f;

    for (auto const& cityIt: m_world.cities())
    {
        City const& city = *cityIt.second;
        Dijkstra router;
        for (auto const& agent: city.agents())
        {
            float const remaining = agent->remainingCost();
            tstt += remaining;

            Node* from = agent->lastNode();
            if (from == nullptr)
                continue;

            float const shortest = router.shortestPathCost(
                *from, agent->searchTarget(), agent->carried());
            if (std::isfinite(shortest))
                sptt += shortest;
        }
    }

    if (tstt <= 1e-6f)
        return 0.0f;
    float const gap = (tstt - sptt) / tstt;
    return (gap < 0.0f) ? 0.0f : gap;
}

//------------------------------------------------------------------------------
City& Simulation::addCity(std::string const& name, Vector3f position)
{
    return addCity(name, position, m_gridSizeU, m_gridSizeV);
}

//------------------------------------------------------------------------------
City& Simulation::addCity(std::string const& name, Vector3f position,
                          uint32_t sizeU, uint32_t sizeV)
{
    City& city = m_world.addCity(name, position, sizeU, sizeV);
    m_listener->onCityAdded(city);
    return city;
}

//------------------------------------------------------------------------------
City& Simulation::getCity(std::string const& name)
{
    return m_world.getCity(name);
}

//------------------------------------------------------------------------------
City const& Simulation::getCity(std::string const& name) const
{
    return m_world.getCity(name);
}
