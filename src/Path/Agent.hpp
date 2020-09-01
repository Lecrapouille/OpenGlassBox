#ifndef AGENT_HPP
#define AGENT_HPP

#include "Vector.hpp"
#include "Unit.hpp"
#include "Path.hpp"
#include <string>

class Agent
{
public:

    Agent(std::string id,
          Vector3f position,
          Unit* owner,
          ResourceBin const& resources,
          std::string const& searchTarget);
    void move();
    bool unloadResources();

private:

    void moveTowardsNextNode();
    void findNextNode();
    void updatePosition();
    //Unit* searchUnit();

private:

    std::string m_id;
    float       m_speed;
    float       m_radius;
    uint32_t    m_color;
    Vector3f    m_position;
    ResourceBin m_resources;
    Unit*       m_owner;
    std::string m_searchTarget;
    float       m_offset = 0.0f;
    Segment    *m_currentSegment = nullptr;
    Node       *m_lastNode = nullptr;
    Node       *m_nextNode = nullptr;
};

#endif
