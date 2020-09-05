#ifndef AGENT_HPP
#define AGENT_HPP

#include "Vector.hpp"
#include "Unit.hpp"
#include "Path.hpp"
#include <string>

class Agent
{
public:

    Agent(uint32_t id,
          Node& node,//Vector3f position,
          Unit& owner,
          Resources const& resources,
          std::string const& searchTarget);
    void move();
    bool unloadResources();

private:

    void moveTowardsNextNode();
    void findNextNode();
    void updatePosition();
    //Unit* searchUnit();

private:

    uint32_t m_id;
    float       m_speed;
    float       m_radius;
    uint32_t    m_color;
    Node& m_node;
    Vector3f    m_position;
    Resources m_resources;
    Unit&       m_owner;
    std::string m_searchTarget;
    float       m_offset = 0.0f;
    Segment    *m_currentSegment = nullptr;
    Node       *m_lastNode = nullptr;
    Node       *m_nextNode = nullptr;
};

#endif
