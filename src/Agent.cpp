//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "OpenGlassBox/Agent.hpp"
#include "OpenGlassBox/Config.hpp"
#include "OpenGlassBox/Unit.hpp"
#include <cmath>
#include <iostream>
#include <limits>

namespace ogb {

namespace
{
    static const float MIN_WAY_MAGNITUDE = 1e-6f;
}

//------------------------------------------------------------------------------
Agent::Agent(uint32_t id, AgentType const& type, Unit& owner,
             Resources const& resources, std::string const& searchTarget)
    : m_id(id),
      m_type(type),
      m_searchTarget(searchTarget),
      m_resources(resources),
      m_position(owner.position())
{
    if (owner.way() != nullptr && owner.node() == nullptr)
    {
        // Sit on the Way at the Unit offset and walk to the closer
        // intersection before looking for a route.
        m_currentWay = owner.way();
        m_currentWay->addAgent();
        m_offset = owner.wayOffset();
        m_lastNode = owner.accessNode();
        m_nextNode = m_lastNode;
    }
    else
    {
        m_lastNode = owner.accessNode();
    }
}

//------------------------------------------------------------------------------
Agent::~Agent()
{
    setCurrentWay(nullptr);
}

//------------------------------------------------------------------------------
void Agent::setCurrentWay(Way* way)
{
    if (m_currentWay == way)
        return;

    if (m_currentWay != nullptr)
        m_currentWay->removeAgent();

    m_currentWay = way;

    if (m_currentWay != nullptr)
        m_currentWay->addAgent();
}

//------------------------------------------------------------------------------
void Agent::translate(Vector3f const direction)
{
    m_position += direction;
}

//------------------------------------------------------------------------------
bool Agent::uses(Way const& way) const
{
    if (m_currentWay == &way)
        return true;
    if (m_route.approachWay == &way)
        return true;
    return false;
}

//------------------------------------------------------------------------------
bool Agent::uses(Node const& node) const
{
    if ((m_lastNode == &node) || (m_nextNode == &node))
        return true;
    for (Node* n: m_route.nodes)
    {
        if (n == &node)
            return true;
    }
    return false;
}

//------------------------------------------------------------------------------
Node* Agent::routingNode() const
{
    return (m_nextNode != nullptr) ? m_nextNode : m_lastNode;
}

//------------------------------------------------------------------------------
float Agent::remainingRouteCost() const
{
    if (!m_route.found)
        return 0.0f;

    float cost = 0.0f;
    Node* current = routingNode();
    for (Node* next: m_route.nodes)
    {
        if (current == nullptr || next == nullptr)
            break;
        Way* way = current->getWayToNode(*next);
        if (way != nullptr)
            cost += way->travelTime();
        current = next;
    }
    if ((m_route.approachWay != nullptr) && (current != nullptr))
    {
        float const fraction = (&m_route.approachWay->from() == current)
                               ? m_route.approachOffset
                               : (1.0f - m_route.approachOffset);
        cost += m_route.approachWay->travelTime() * fraction;
    }
    return cost;
}

//------------------------------------------------------------------------------
bool Agent::shouldRecomputeRoute(Dijkstra& dijkstra,
                                 SimulationConfig const& config) const
{
    if (!m_route.found)
        return true;
    if (m_ticksOnRoute >= config.pathRecalcTicks)
        return true;
    if (config.pathCostDeviation <= 0.0f)
        return true;

    Node* from = routingNode();
    if (from == nullptr)
        return true;

    float const remaining = remainingRouteCost();
    if (remaining <= 1e-4f)
        return false;

    float const shortest = dijkstra.shortestPathCost(*from, m_searchTarget,
                                                     m_resources);
    if (!std::isfinite(shortest))
        return false;

    return shortest + remaining * config.pathCostDeviation < remaining;
}

//------------------------------------------------------------------------------
void Agent::computeRoute(Dijkstra& dijkstra)
{
    Node* from = routingNode();
    if (from == nullptr)
    {
        m_route = Route();
        return;
    }

    m_route = dijkstra.findRoute(*from, m_searchTarget, m_resources);
    m_ticksOnRoute = 0u;
}

//------------------------------------------------------------------------------
void Agent::followRoute(Dijkstra& dijkstra, SimulationConfig const& config)
{
    ++m_ticksOnRoute;

    if (shouldRecomputeRoute(dijkstra, config))
        computeRoute(dijkstra);

    if (!m_route.found)
    {
        if (m_lastNode == nullptr)
            return;

        m_nextNode = dijkstra.randomNeighbor(*m_lastNode);
        if (m_nextNode == nullptr)
            return;

        setCurrentWay(m_lastNode->getWayToNode(*m_nextNode));
        if (m_currentWay != nullptr)
            m_offset = (m_lastNode == &m_currentWay->from()) ? 0.0f : 1.0f;
        else
            m_nextNode = nullptr;
        return;
    }

    if (!m_route.nodes.empty())
    {
        m_nextNode = m_route.nodes.front();
        m_route.nodes.erase(m_route.nodes.begin());
        if (m_lastNode != nullptr)
        {
            setCurrentWay(m_lastNode->getWayToNode(*m_nextNode));
            if (m_currentWay != nullptr)
            {
                m_offset = (m_lastNode == &m_currentWay->from()) ? 0.0f : 1.0f;
            }
            else
            {
                m_nextNode = nullptr;
            }
        }
        return;
    }

    if (m_route.approachWay != nullptr)
    {
        setCurrentWay(m_route.approachWay);
        if (m_lastNode == &m_currentWay->from())
        {
            m_offset = 0.0f;
            m_nextNode = &m_currentWay->to();
        }
        else
        {
            m_offset = 1.0f;
            m_nextNode = &m_currentWay->from();
        }
        return;
    }

    m_nextNode = nullptr;
}

//------------------------------------------------------------------------------
bool Agent::update(Dijkstra& dijkstra, float dt)
{
    SimulationConfig const& config = (m_simConfig != nullptr)
                                     ? *m_simConfig
                                     : SimulationConfig{};
    return update(dijkstra, config, dt);
}

//------------------------------------------------------------------------------
bool Agent::update(Dijkstra& dijkstra, SimulationConfig const& config, float dt)
{
    if (m_nextNode == nullptr)
    {
        if (unloadResources())
            return true;

        if (m_lastNode == nullptr)
            return false;

        followRoute(dijkstra, config);
    }
    else
    {
        moveTowardsNextNode(dt);
    }

    return false;
}

//------------------------------------------------------------------------------
Unit* Agent::searchUnit()
{
    if ((m_currentWay != nullptr) && (m_route.approachWay == m_currentWay))
    {
        float const arrived = std::fabs(m_offset - m_route.approachOffset);
        if (arrived <= 0.05f || m_nextNode == nullptr)
        {
            for (Unit* unit: m_currentWay->units())
            {
                if (unit->accepts(m_searchTarget, m_resources))
                    return unit;
            }
        }
    }

    if (m_lastNode == nullptr)
        return nullptr;

    std::vector<Unit*>& units = m_lastNode->units();
    size_t i = units.size();
    while (i--)
    {
        if (units[i]->accepts(m_searchTarget, m_resources))
            return units[i];
    }

    return nullptr;
}

//------------------------------------------------------------------------------
bool Agent::unloadResources()
{
    Unit* unit = searchUnit();
    if (unit != nullptr)
        m_resources.transferResourcesTo(unit->resources());
    return m_resources.isEmpty();
}

//------------------------------------------------------------------------------
void Agent::moveTowardsNextNode(float dt)
{
    if (m_currentWay == nullptr)
        return;

    // Destination is a point on the current Way rather than a Node.
    if ((m_route.approachWay == m_currentWay) && m_route.nodes.empty())
    {
        float const target = m_route.approachOffset;
        float const wayLength = m_currentWay->magnitude();
        if (wayLength <= MIN_WAY_MAGNITUDE)
        {
            m_offset = target;
            m_position = m_currentWay->positionAt(m_offset);
            m_nextNode = nullptr;
            m_lastNode = (m_offset <= 0.5f) ? &m_currentWay->from()
                                            : &m_currentWay->to();
            return;
        }

        float const direction = (target >= m_offset) ? 1.0f : -1.0f;
        m_offset += direction * m_type.speed * dt / wayLength;

        bool const arrived = (direction > 0.0f) ? (m_offset >= target)
                                                : (m_offset <= target);
        if (arrived)
        {
            m_offset = target;
            m_nextNode = nullptr;
            m_lastNode = (m_offset <= 0.5f) ? &m_currentWay->from()
                                            : &m_currentWay->to();
        }

        m_position = m_currentWay->positionAt(m_offset);
        return;
    }

    float direction = (m_nextNode == &(m_currentWay->to())) ? 1.0f : -1.0f;

    float const wayLength = m_currentWay->magnitude();
    if (wayLength <= MIN_WAY_MAGNITUDE)
    {
        m_lastNode = m_nextNode;
        m_nextNode = nullptr;
        m_offset = 0.0f;
        if (m_lastNode != nullptr)
            m_position = m_lastNode->position();
        return;
    }

    m_offset += direction * m_type.speed * dt / wayLength;

    if (m_offset < 0.0f)
    {
        m_offset = 0.0f;
        m_lastNode = &m_currentWay->from();
        m_nextNode = nullptr;
    }
    else if (m_offset > 1.0f)
    {
        m_offset = 1.0f;
        m_lastNode = &m_currentWay->to();
        m_nextNode = nullptr;
    }

    m_position = m_currentWay->positionAt(m_offset);
}

} // namespace ogb
