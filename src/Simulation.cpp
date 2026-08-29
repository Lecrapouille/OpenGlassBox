//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "OpenGlassBox/Simulation.hpp"
#include "OpenGlassBox/Agent.hpp"

//------------------------------------------------------------------------------
namespace ogb
{

Simulation::Simulation(Config const& config)
    : m_clock(config.time.ticksPerMinute), m_world(config, m_clock)
{
    // A city that opens at midnight keeps the player waiting until the rules
    // that hold office hours wake up. Loading a save overwrites this, since it
    // restores the tick counter it was written with.
    m_clock.setTimeOfDay(0u, config.time.startHour, 0u);
}

// -----------------------------------------------------------------------------
void Simulation::setListener(Simulation::Listener& listener)
{
    m_world.setListener(listener);
}

// -----------------------------------------------------------------------------
bool Simulation::loadScriptFile(std::string const& filename)
{
    return m_ruleset.loadFile(filename);
}

// -----------------------------------------------------------------------------
bool Simulation::loadScriptString(std::string const& source,
                                  std::string const& name)
{
    return m_ruleset.loadString(source, name);
}

//------------------------------------------------------------------------------
void Simulation::stepOneTick()
{
    m_world.update(getConfig().time.tickDuration());
}

//------------------------------------------------------------------------------
void Simulation::update(float const deltaTime)
{
    if (m_paused)
        return;

    m_time += deltaTime * m_timeScale;

    float const tick = getConfig().time.tickDuration();
    uint32_t maxIterations = getConfig().time.maxTicksPerUpdate;
    while ((m_time >= tick) && (maxIterations-- > 0u))
    {
        m_time -= tick;
        stepOneTick();
    }
}

//------------------------------------------------------------------------------
void Simulation::updateTrafficMetrics() const
{
    // A panel reads these on every frame while the simulation advances twenty
    // times a second, and the answer cannot change between two ticks. Nothing
    // below runs again until the world has actually moved.
    if (m_trafficMetricsTick == m_clock.getTicks())
        return;

    float tstt = 0.0f;
    float sptt = 0.0f;

    uint32_t const budget = getConfig().traffic.relativeGapSamples;

    for (auto const& cityIt : m_world.getCities())
    {
        City const& city = *cityIt.second;
        auto const& agents = city.getAgents();

        // Walking one agent in every stride samples the whole population
        // instead of whichever end of the vector the loop starts at. Both
        // sums are scaled the same way, and the gap being their ratio, the
        // estimate needs no correction factor.
        size_t stride = 1u;
        if ((budget != 0u) && (agents.size() > size_t(budget)))
        {
            stride = agents.size() / size_t(budget);
        }

        IRouter& router = city.getRouter();

        for (size_t i = 0u; i < agents.size(); i += stride)
        {
            Agent& agent = *agents[i];

            // The two sums have to run over the same agents, or their ratio
            // measures the difference in populations rather than the distance
            // to equilibrium. An agent with no itinerary has no remaining cost
            // to put in TSTT, so it must not put an alternative in SPTT
            // either; one whose reroute is unreachable has no alternative, so
            // it must not put its remaining cost in TSTT.
            if (!agent.getRoute().isFound())
                continue;

            float const alternative = agent.computeRerouteCost(router);
            if (routingCostUnreachable(alternative))
                continue;

            tstt += agent.getRemainingCost();
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

    m_trafficMetrics.totalTravelTime = tstt;
    m_trafficMetrics.shortestPathTime = sptt;
    m_trafficMetrics.relativeGap = gap;
    m_trafficMetricsTick = m_clock.getTicks();
}

//------------------------------------------------------------------------------
Simulation::TrafficMetrics Simulation::getTrafficMetrics() const
{
    updateTrafficMetrics();
    return m_trafficMetrics;
}

//------------------------------------------------------------------------------
City& Simulation::addCity(std::string const& name, Vector3f const& position)
{
    return m_world.addCity(name, position);
}

//------------------------------------------------------------------------------
City& Simulation::addCity(std::string const& name,
                          Vector3f const& position,
                          uint32_t sizeU,
                          uint32_t sizeV)
{
    return m_world.addCity(name, position, sizeU, sizeV);
}

//------------------------------------------------------------------------------
bool Simulation::removeCity(std::string const& name)
{
    return m_world.removeCity(name);
}

//------------------------------------------------------------------------------
City& Simulation::getCity(std::string const& name)
{
    return m_world.getCity(name);
}

//------------------------------------------------------------------------------
City* Simulation::findCity(std::string const& name)
{
    return m_world.findCity(name);
}

//------------------------------------------------------------------------------
City* Simulation::findCityAt(Vector3f const& position)
{
    return m_world.findCityAt(position);
}

//------------------------------------------------------------------------------
Layer* Simulation::findLayer(std::string const& name)
{
    return m_world.findLayer(name);
}

//------------------------------------------------------------------------------
Cell Simulation::worldToCell(Vector3f const& position) const
{
    return m_world.worldToCell(position);
}

//------------------------------------------------------------------------------
Vector3f Simulation::cellToWorld(Cell const cell) const
{
    return m_world.cellToWorld(cell);
}

//------------------------------------------------------------------------------
bool Simulation::addRoad(City& owner,
                         std::string const& pathType,
                         SegmentType const& segmentType,
                         Vector3f const& from,
                         Vector3f const& to)
{
    return m_world.addRoad(owner, pathType, segmentType, from, to);
}

} // namespace ogb
