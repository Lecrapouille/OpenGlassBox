//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "OpenGlassBox/Simulation.hpp"
#include "OpenGlassBox/Agent.hpp"

#include <cmath>
#include <limits>

//------------------------------------------------------------------------------
namespace ogb
{

Simulation::Simulation(uint32_t gridSizeU,
                       uint32_t gridSizeV,
                       SimulationConfig const& config)
    : m_world(config), m_gridSizeU(gridSizeU), m_gridSizeV(gridSizeV)
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
void Simulation::updateAssignmentMetrics() const
{
    // A panel reads these on every frame while the simulation advances twenty
    // times a second, and the answer cannot change between two ticks. Nothing
    // below runs again until the world has actually moved.
    if (m_assignmentMetricsTick == m_totalTicks)
        return;

    float tstt = 0.0f;
    float sptt = 0.0f;

    uint32_t const budget = config().relativeGapSamples;

    for (auto const& cityIt : m_world.cities())
    {
        City const& city = *cityIt.second;
        auto const& agents = city.agents();

        // Walking one Agent in every stride samples the whole population
        // instead of whichever end of the vector the loop starts at. Both
        // sums are scaled the same way, and the gap being their ratio, the
        // estimate needs no correction factor.
        size_t stride = 1u;
        if ((budget != 0u) && (agents.size() > size_t(budget)))
        {
            stride = agents.size() / size_t(budget);
        }

        IRouter& router = const_cast<City&>(city).router();

        for (size_t i = 0u; i < agents.size(); i += stride)
        {
            Agent& agent = *agents[i];

            // The two sums have to run over the same Agents, or their ratio
            // measures the difference in populations rather than the distance
            // to equilibrium. An Agent with no itinerary has no remaining cost
            // to put in TSTT, so it must not put an alternative in SPTT
            // either; one whose reroute is unreachable has no alternative, so
            // it must not put its remaining cost in TSTT.
            if (!agent.route().found)
                continue;

            float const alternative = agent.rerouteCost(router);
            if (routingCostUnreachable(alternative))
                continue;

            tstt += agent.remainingCost();
            sptt += alternative;
        }
    }

    float gap = 0.0f;
    if (tstt > 1e-6f)
    {
        gap = (tstt - sptt) / tstt;
        if (gap < 0.0f)
            gap = 0.0f;
    }

    m_tstt = tstt;
    m_sptt = sptt;
    m_relativeGap = gap;
    m_assignmentMetricsTick = m_totalTicks;
}

//------------------------------------------------------------------------------
float Simulation::relativeGap() const
{
    updateAssignmentMetrics();
    return m_relativeGap;
}

//------------------------------------------------------------------------------
float Simulation::totalSystemTravelTime() const
{
    updateAssignmentMetrics();
    return m_tstt;
}

//------------------------------------------------------------------------------
float Simulation::shortestPathTravelTime() const
{
    updateAssignmentMetrics();
    return m_sptt;
}

//------------------------------------------------------------------------------
City& Simulation::addCity(std::string const& name, Vector3f position)
{
    return addCity(name, position, m_gridSizeU, m_gridSizeV);
}

//------------------------------------------------------------------------------
City& Simulation::addCity(std::string const& name,
                          Vector3f position,
                          uint32_t sizeU,
                          uint32_t sizeV)
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

} // namespace ogb
