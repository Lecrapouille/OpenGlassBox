#ifndef AGENT_HPP
#define AGENT_HPP

#include "Core/Vector.hpp"
#include "Core/Unit.hpp"
#include "Core/Path.hpp"
#include <string>

class City;

//==============================================================================
//! \brief Created by UnitRules. Each Agent has a set of resources and carry
//! these resources from one Unit to another. Agents do not run rules because
//! there are 10,000s of agents by simulation)
//! Each agent is given a destination (Home, Work, Fire, Sickness).
//==============================================================================
class Agent
{
public:

    //==========================================================================
    //! \brief Type of Agents (people, worker ...).
    //! Class constructed during the parsing of simulation scripts.
    //! Examples:
    //!  - agent People color 0xFFFF00 speed 10.5
    //!  - agent Worker color 0xFFFFFF speed 10 radius 3
    //==========================================================================
    struct Type
    {
        Type(float s = 1.0f, uint32_t r = 1u, uint32_t c = 0xFFFFFF)
            : speed(s), radius(r), color(c)
        {}

        float     speed;
        uint32_t  radius;
        uint32_t  color;
    };

public:

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    Agent(uint32_t id, Agent::Type const& type, Unit& owner,
          Resources const& resources, std::string const& searchTarget);

    //--------------------------------------------------------------------------
    //! \brief Configure the Agent from information get on simulation scripts
    //--------------------------------------------------------------------------
    void configure(Agent::Type const& type);

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

    // Should be an int refering a global table
    float       m_speed = 1.0f;
    uint32_t    m_radius = 1u;
    uint32_t    m_color = 0xFFFFFF;
};

using Agents = std::vector<std::unique_ptr<Agent>>;

#endif
