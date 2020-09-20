#ifndef AGENT_HPP
#define AGENT_HPP

#include "Core/Vector.hpp"
#include "Core/Unit.hpp"
#include "Core/Path.hpp"
#include <string>

class City;

//==========================================================================
//! \brief Type of Agents (people, worker ...).
//! Class constructed during the parsing of simulation scripts.
//! Examples:
//!  - agent People color 0xFFFF00 speed 10.5
//!  - agent Worker color 0xFFFFFF speed 10 radius 3
//==========================================================================
class AgentType
{
public:

    AgentType(AgentType const&) = default;

    AgentType(/*std::string const& name,*/ float speed /*= 1.0f*/, uint32_t radius /*= 1u*/, uint32_t color /*= 0xFFFFFF*/)
        : /*m_name(name),*/ m_speed(speed), m_radius(radius), m_color(color)
    {}

    //std::string m_name; // FIXME nooo!
    float       m_speed;
    uint32_t    m_radius;
    uint32_t    m_color;
};

//==============================================================================
//! \brief Created by UnitRules. Each Agent has a set of resources and carry
//! these resources from one Unit to another. Agents do not run rules because
//! there are 10,000s of agents by simulation)
//! Each agent is given a destination (Home, Work, Fire, Sickness).
//==============================================================================
class Agent: private AgentType
{
public:

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    Agent(uint32_t id, AgentType const& type, Unit& owner,
          Resources const& resources, std::string const& searchTarget);

    //--------------------------------------------------------------------------
    //! \brief Driving on Segments, transporting resources and unloading them on
    //! destination.
    //--------------------------------------------------------------------------
    void move(City& city);

    //--------------------------------------------------------------------------
    //! \brief Transfert the amount of resource to the target Unit.
    //! \return true if after the transfert the Agent has no more resource to
    //! carry.
    //--------------------------------------------------------------------------
    bool unloadResources();

    //--------------------------------------------------------------------------
    //! \brief Return the world position
    //--------------------------------------------------------------------------
    Vector3f const& position() const { return m_position; }

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
    Unit       &m_owner;
    Resources   m_resources;
    std::string m_searchTarget;
    Vector3f    m_position;
    float       m_offset = 0.0f;
    Segment    *m_currentSegment = nullptr;
    Node       *m_lastNode = nullptr;
    Node       *m_nextNode = nullptr;
};

using Agents = std::vector<std::unique_ptr<Agent>>;

#endif
