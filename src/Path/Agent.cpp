#include "Agent.hpp"
#include <iostream>

#define TICKS_PER_SECOND 10.0f

Agent::Agent(std::string id,
             Vector3f position,
             Unit* owner,
             Resources const& resources,
             std::string const& searchTarget)
{
    m_id = id;
    m_owner = owner;
    m_searchTarget = m_searchTarget;
    m_resources.addResources(resources);
    m_position = position;
    m_searchTarget = searchTarget;
}

void Agent::move()
{
    if (m_nextNode == nullptr)
    {
        if (unloadResources())
        {
            //TODO m_lastNode->path().box.RemoveAgent(this);
        }
        else
        {
            findNextNode();
        }
    }
    else
    {
        moveTowardsNextNode();
    }
  }

void Agent::moveTowardsNextNode()
{
    float direction;

    if (m_nextNode == &m_currentSegment->node2())
        direction = 1.0f; // moving from node1 to node2
    else
        direction = -1.0f; // moving from node2 to node1

    m_offset += direction * (m_speed / TICKS_PER_SECOND); //FIXME / m_currentSegment->length();

    // Reached node1
    if (m_offset < 0.0f)
    {
        m_offset = 0.0f;
        m_lastNode = &m_currentSegment->node1();
        m_nextNode = nullptr;
    }

    // Reached node2
    else if (m_offset > 1.0f)
    {
        m_offset = 1.0f;
        m_lastNode = &m_currentSegment->node2();
        m_nextNode = nullptr;
    }

    updatePosition();
}

void Agent::updatePosition()
{
    m_position = m_currentSegment->position1() +
                 (m_currentSegment->position1() - m_currentSegment->position2())
                 * m_offset;
}

void Agent::findNextNode()
{
    m_nextNode = nullptr; // TODO m_lastNode->path().findNextNode(m_lastNode, m_searchTarget, m_resources);

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
