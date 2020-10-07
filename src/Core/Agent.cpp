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
      m_lastNode(&(owner.node())) // FIXME Unit's node should be linked at least one segment
{
    #warning "findNextNode();"
    //findNextNode();
    updatePosition();
    if (m_currentWay == nullptr)
    {
        std::cerr << "Ill formed Agent " << id << ": does not belong to any segment"
                  << std::endl;
    }
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
        // Driving on the Way
        moveTowardsNextNode();
    }

    return false;
}

//------------------------------------------------------------------------------
void Agent::moveTowardsNextNode()
{
    float direction;

    if (m_currentWay != nullptr)
    {
        if (m_nextNode == &(m_currentWay->to()))
        {
            // Moving from node1 to node2
            direction = 1.0f;
        }
        else
        {
            // Moving from node2 to node1
            direction = -1.0f;
        }

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
    }

    updatePosition();
}

//------------------------------------------------------------------------------
void Agent::updatePosition()
{
    if (m_currentWay != nullptr)
    {
        m_position = m_currentWay->position1() +
                     (m_currentWay->position2() - m_currentWay->position1())
                     * m_offset;
    }
    else
    {
        m_position = m_lastNode->position();
    }
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
bool Agent::unloadResources()
{
    Unit* unit = searchUnit();
    if (unit != nullptr)
        m_resources.transferResourcesTo(unit->resources());
    return m_resources.isEmpty();
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
