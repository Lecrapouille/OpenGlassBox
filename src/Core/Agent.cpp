//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "Core/Agent.hpp"
#include "Core/Dijkstra.hpp"
#include "Core/Unit.hpp"
#include "Core/Config.hpp"
#include <iostream>

//------------------------------------------------------------------------------
Agent::Agent(uint32_t id, AgentType const& type, Unit& owner,
             Resources const& resources, std::string const& searchTarget)
    : m_id(id),
      m_type(type),
      m_searchTarget(searchTarget),
      m_resources(resources),
      m_lastNode(&(owner.node()))
{
    // Unit's Node should be linked at least one arc (Way) of the graph (Path)
    // else Agents cannot move towards a Path Way.
}

//------------------------------------------------------------------------------
void Agent::translate(Vector3f const direction)
{
    m_position += direction;
}

//------------------------------------------------------------------------------
bool Agent::update(Dijkstra& dijkstra)
{
    // Reached the destination node ?
    if (m_nextNode == nullptr)
    {
        // Yes ! Transfer resources to the Unit.
        // Has Agent no more resource ?
        if (unloadResources())
        {
            // Yes ! Return true to remove it !
            return true;
        }
        else
        {
            // No ! Keep finding another destination to unload resources.
            findNextNode(dijkstra);
        }
    }
    else
    {
        // Move the Agent towards the next Node.
        moveTowardsNextNode();
    }

    return false;
}

//------------------------------------------------------------------------------
Unit* Agent::searchUnit()
{
    if (m_lastNode == nullptr)
        return nullptr;

    std::vector<Unit*>& units = m_lastNode->units();
    size_t i = units.size();
    while (i--)
    {
        if (units[i]->accepts(m_searchTarget, m_resources))
        {
            return units[i];
        }
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
void Agent::findNextNode(Dijkstra& dijkstra)
{
    if (m_lastNode == nullptr)
        return ;

    m_nextNode = dijkstra.findNextPoint(*m_lastNode, m_searchTarget, m_resources);
    if (m_nextNode != nullptr)
    {
        m_currentWay = m_lastNode->getWayToNode(*m_nextNode);
        if (m_currentWay != nullptr)
        {
            if (m_lastNode == &m_currentWay->from())
                m_offset = 0.0f;
            else
                m_offset = 1.0f;
        }
        else
        {
            std::cout << "failure: getWayToNode" << std::endl;
        }
    }
}

//------------------------------------------------------------------------------
void Agent::moveTowardsNextNode()
{
    float direction;

#if !defined(NDEBUG)
    // Unit's Node should be linked at least one arc (Way) of the graph (Path)
    // else Agents cannot move towards a Path Way.
    if (m_currentWay == nullptr)
    {
        std::cerr << "Ill-formed Node: should have Ways to "
                  << "make Agents move towards them" << std::endl;
        m_position = m_lastNode->position();
        return ;
    }
#endif

    if (m_nextNode == &(m_currentWay->to()))
    {
        // Moving from origin node to destination node
        direction = 1.0f;
    }
    else
    {
        // Moving from destination node to origin node
        direction = -1.0f;
    }

    // FIXME use dt()
    m_offset += direction
                * (m_type.speed / config::TICKS_PER_SECOND)
                / m_currentWay->magnitude();

    // Reached node1 ?
    if (m_offset < 0.0f)
    {
        m_offset = 0.0f;
        m_lastNode = &m_currentWay->from();
        m_nextNode = nullptr;
    }

    // Reached node2 ?
    else if (m_offset > 1.0f)
    {
        m_offset = 1.0f;
        m_lastNode = &m_currentWay->to();
        m_nextNode = nullptr;
    }

    // Update the world position of the Agent along the way.
    // TODO: Faster m_position += m_deltaSlope where m_deltaSlope
    // is the vector of delta deplacement along the slope
    m_position = m_currentWay->position1() +
                 (m_currentWay->position2() - m_currentWay->position1())
                 * m_offset;
}
