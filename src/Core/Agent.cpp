#include "Core/Agent.hpp"
#include "Core/City.hpp"
#include "Core/Config.hpp"
#include <iostream>

//------------------------------------------------------------------------------
Agent::Agent(uint32_t id, AgentType const& type, Unit& owner,
             Resources const& resources, std::string const& searchTarget)
    : AgentType(type),
      m_id(id),
      m_owner(owner),
      m_resources(resources),
      m_searchTarget(searchTarget),
      m_lastNode(&(owner.node())) // FIXME should be linked at least one segment
{
    findNextNode();
    updatePosition();
    if (m_currentSegment == nullptr)
    {
        std::cerr << "Ill formed Agent " << id << ": does not belong to any segment"
                  << std::endl;
    }

}

//------------------------------------------------------------------------------
void Agent::move(City& city)
{
    // Reached the destination node ?
    if (m_nextNode == nullptr)
    {
        // Yes ! Transfer resources to the Unit.
        // Has Agent no more resource ?
        if (unloadResources())
        {
            // Yes ! Remove it !
            city.removeAgent(*this);
        }
        else
        {
            // No ! Keep finding another destination to unload resources.
            findNextNode();
        }
    }
    else
    {
        // Driving on the Segment
        moveTowardsNextNode();
    }
}

//------------------------------------------------------------------------------
void Agent::moveTowardsNextNode()
{
    float direction;

    if (m_currentSegment != nullptr)
    {
        if (m_nextNode == &(m_currentSegment->node2()))
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
                    * (m_speed / config::TICKS_PER_SECOND)
                    / m_currentSegment->length();

        // Reached node1 ?
        if (m_offset < 0.0f)
        {
            m_offset = 0.0f;
            m_lastNode = &m_currentSegment->node1();
            m_nextNode = nullptr;
        }

        // Reached node2 ?
        else if (m_offset > 1.0f)
        {
            m_offset = 1.0f;
            m_lastNode = &m_currentSegment->node2();
            m_nextNode = nullptr;
        }
    }

    updatePosition();
}

//------------------------------------------------------------------------------
void Agent::updatePosition()
{
    if (m_currentSegment != nullptr)
    {
        m_position = m_currentSegment->position1() +
                     (m_currentSegment->position2() - m_currentSegment->position1())
                     * m_offset;
    }
    else
    {
        m_position = m_lastNode->position();
    }
}

//------------------------------------------------------------------------------
void Agent::findNextNode()
{
    // Ill formed agent: abord
    if ((m_lastNode == nullptr) || (m_lastNode->segments().size() == 0u))
        return ;

    m_nextNode = &(m_lastNode->segments()[0]->node2());
    //nullptr; // TODO m_lastNode->path().findNextNode(m_lastNode, m_searchTarget, m_resources);

    if (m_nextNode != nullptr)
    {
        m_currentSegment = m_lastNode->getSegmentToNode(*m_nextNode);
        if (m_currentSegment != nullptr)
        {
            if (m_lastNode == &m_currentSegment->node1())
                m_offset = 0.0f;
            else
                m_offset = 1.0f;
        }
        else
        {
            std::cout << "failure: getSegmentToNode" << std::endl;
        }
    }
}

//------------------------------------------------------------------------------
bool Agent::unloadResources()
{
    std::vector<Unit*>& units = m_lastNode->units();
    size_t i = units.size();
    while (i--)
    {
        if (units[i]->accepts(m_searchTarget, m_resources))
        {
            m_resources.transferResourcesTo(units[i]->resources());
        }
    }

    return m_resources.isEmpty();
}

//------------------------------------------------------------------------------
// TODO
/*Unit* Agent::searchUnit()
{
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
*/
