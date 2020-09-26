#ifndef AGENT_HPP
#define AGENT_HPP

#include "Core/Vector.hpp"
#include "Core/Unit.hpp"
#include "Core/Path.hpp"
#include <string>

class City;

//==============================================================================
//! \brief Type of Agents (people, worker ...).
//! Class constructed during the parsing of simulation scripts. This class is
//! shared as read only by several Agent and therefore used as const reference.
//!
//! Examples:
//!  - agent People color 0xFFFF00 speed 10.5
//!  - agent Worker color 0xFFFFFF speed 10 radius 3
//==============================================================================
class AgentType
{
public:

    //--------------------------------------------------------------------------
    //! \brief Copy constructor.
    //--------------------------------------------------------------------------
    AgentType(AgentType const&) = default;

    //--------------------------------------------------------------------------
    //! \brief Empty constructor with default parameters.
    //--------------------------------------------------------------------------
    AgentType(std::string const& name_)
        : name(name_), speed(1.0f), radius(1u), color(0xFFFF00)
    {}

    //--------------------------------------------------------------------------
    //! \brief Constructor with the full values to set.
    //--------------------------------------------------------------------------
    AgentType(std::string const& name_, float speed_, uint32_t radius_, uint32_t color_)
        : name(name_), speed(speed_), radius(radius_), color(color_)
    {}

    //! \brief Name of the Agent (ie. People, Worker ...)
    std::string name;
    //! \brief Max velocity on Segments of Path.
    float       speed;
    //! \brief Radius action on Maps.
    uint32_t    radius;
    //! \brief For Rendering agents.
    uint32_t    color;
};

//==============================================================================
//! \brief Created by UnitRules. Each Agent has a set of resources and carry
//! these resources from one Unit to another. Agents do not run rules because
//! there are 10,000s of agents by simulation)
//! Each agent is given a destination (Home, Work, Fire, Sickness).
//==============================================================================
class Agent
{
public:

    //--------------------------------------------------------------------------
    //! \brief Initialized the state of the Agent.
    //! \param[in] id: unique identifier.
    //! \param[in] type: const reference of a given type of Agent also refered
    //! internally. The refered instance shall not be deleted before this Agent
    //! instance is destroyed.
    //! \param[in] owner: Unit owning this agent. This param is used to place
    //! the Agent on the Node affected by the Unit. FIXME Unit split Segment
    //! into two sub-segment.
    //! \param[in] resources: Resources that the Agent is carrying.
    //! \param[in] searchTarget: the Unit target (type of destination).
    //--------------------------------------------------------------------------
    Agent(uint32_t id, AgentType const& type, Unit& owner, Resources const& resources,
          std::string const& searchTarget);

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

    uint32_t           m_id;
    AgentType const&   m_type;
    std::string        m_searchTarget;
    Resources          m_resources;
    Vector3f           m_position;
    float              m_offset = 0.0f;
    Segment           *m_currentSegment = nullptr;
    Node              *m_lastNode = nullptr;
    Node              *m_nextNode = nullptr;
};

using Agents = std::vector<std::unique_ptr<Agent>>;

#endif
