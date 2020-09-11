#ifndef AGENT_HPP
#define AGENT_HPP

#include "Vector.hpp"
#include "Unit.hpp"
#include "Path.hpp"
#include <string>

class City;

struct AgentConfig
{
    float     speed = 1.0f;
    float     radius = 1.0f;
    uint32_t  color = 0xFFFFFF;
};

//==============================================================================
//! \brief Carry resources from one Unit to another.
//!
//! Created by Unit rules. Each Agent has a set of resources.
//! Each agent is given a destination (Home, Work, Fire, Sickness).
//==============================================================================
class Agent
{
public:

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    Agent(uint32_t id,
          Node& node,
          Unit& owner,
          Resources const& resources,
          std::string const& searchTarget);

    //------------------------------------------------------------------------------
    //! \brief Configurate the agent. Method called from the Rule
    //--------------------------------------------------------------------------
    void configure(AgentConfig const& config);

    //--------------------------------------------------------------------------
    //! \brief Driving on Segments, transporting resources and unloading them on
    //! destination.
    //--------------------------------------------------------------------------
    void move(City& city);

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    bool unloadResources();

private:

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    void moveTowardsNextNode();

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    void findNextNode();

    //--------------------------------------------------------------------------
    //! \brief Update the position of the Agent along the segment.
    //--------------------------------------------------------------------------
    void updatePosition();

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    //Unit* searchUnit();

private:

    uint32_t    m_id;
    Node       &m_node;
    Unit       &m_owner;
    Resources   m_resources;
    std::string m_searchTarget;
    Vector3f    m_position;
    float       m_offset = 0.0f;
    Segment    *m_currentSegment = nullptr;
    Node       *m_lastNode = nullptr;
    Node       *m_nextNode = nullptr;

    float       m_speed = 1.0f;
    float       m_radius = 1.0f;
    uint32_t    m_color = 0xFFFFFF;
};

#endif
