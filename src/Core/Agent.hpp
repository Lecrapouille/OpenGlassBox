//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#ifndef OPEN_GLASSBOX_AGENT_HPP
#  define OPEN_GLASSBOX_AGENT_HPP

#  include "Core/Path.hpp"

class Dijkstra;
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
    VIRTUAL ~Agent() = default;

    //--------------------------------------------------------------------------
    //! \brief Make the Agent transporting resources on the current Way and
    //! make unloading resource when the Agent has reached its destination.
    //! \return true when the Agent has reached its destination.
    //--------------------------------------------------------------------------
    VIRTUAL bool update(Dijkstra& dijkstra);

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

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    uint32_t color() const { return m_type.color; }

    // -------------------------------------------------------------------------
    //! \brief Change the position of the Agent in the world.
    //! This also change the position of Path, Unit, Agent ... hold by the City.
    // -------------------------------------------------------------------------
    void translate(Vector3f const direction);

private:

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    void moveTowardsNextNode();

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    void findNextNode(Dijkstra& dijkstra);

    //--------------------------------------------------------------------------
    //! \brief Update the position of the Agent along the segment.
    //--------------------------------------------------------------------------
    void updatePosition();

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    Unit* searchUnit();

private:

    uint32_t           m_id;
    AgentType const&   m_type;
    std::string        m_searchTarget;
    Resources          m_resources;
    Vector3f           m_position; // idee Vector3f& pointe sur un tableau de Positions qui lui sera utilise pour le renderer
    float              m_offset = 0.0f;
    Way           *m_currentWay = nullptr;
    Node              *m_lastNode = nullptr; // pourrait etre supprime car m_currentWay->from() et ->to()
    Node              *m_nextNode = nullptr;
};

using Agents = std::vector<std::unique_ptr<Agent>>;

#endif
