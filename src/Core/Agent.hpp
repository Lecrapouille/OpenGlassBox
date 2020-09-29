#ifndef AGENT_HPP
#define AGENT_HPP

#include "Core/Path.hpp"

class City;
class Unit;

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
    //! the Agent on the Node affected by the Unit. FIXME Unit split Way
    //! into two sub-segment.
    //! \param[in] resources: Resources that the Agent is carrying.
    //! \param[in] searchTarget: the Unit target (type of destination).
    //--------------------------------------------------------------------------
    Agent(uint32_t id, AgentType const& type, Unit& owner, Resources const& resources,
          std::string const& searchTarget);

    //--------------------------------------------------------------------------
    //! \brief Driving on Ways, transporting resources and unloading them on
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

    // -------------------------------------------------------------------------
    //! \brief Getter: return the type of Agent.
    // -------------------------------------------------------------------------
    std::string const& type() const { return m_type.name; }


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
    Way           *m_currentWay = nullptr;
    Node              *m_lastNode = nullptr;
    Node              *m_nextNode = nullptr;
};

using Agents = std::vector<std::unique_ptr<Agent>>;

#endif
