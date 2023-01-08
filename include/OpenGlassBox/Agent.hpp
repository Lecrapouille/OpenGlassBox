//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#ifndef OPEN_GLASSBOX_AGENT_HPP
#  define OPEN_GLASSBOX_AGENT_HPP

#  include "OpenGlassBox/Path.hpp"

class Dijkstra;
class Unit;

//==============================================================================
//! \brief Created by \c UnitRules. Each Agent has a set of resources and carry
//! these resources from one Unit to another. Agents do not run rules because
//! they are to many of them created by simulation (ideally 1000 ...) this would
//! reduce performance.
//! Each Agent is given a destination (Home, Work, Fire, Sickness).
//==============================================================================
class Agent
{
public:

    //--------------------------------------------------------------------------
    //! \brief Default constructor doing partial initialisation of internal
    //! states.  The complete initialisation of internal states will be done
    //! once the \c update() method will be called).
    //!
    //! \param[in] id: Unique identifier.
    //!
    //! \param[in] type: Const reference of a given type of Agent also refered
    //! internally. The reference shall not be deleted before this Agent
    //! instance is destroyed. The type of Agent is defined by simulation
    //! scripts.
    //!
    //! \param[in] owner: Unit creating this agent, also refered internally. The
    //! reference shall not be deleted before this Agent instance is destroyed.
    //! This param is used to place the Agent on the graph node (Node of a Path)
    //! moving towards arcs. Note: that for placing an Unit somewhere along a Way
    //! you'll have to split it into two ways linked by an Node and this Node
    //! will be the support for the Unit. Note also Unit's Node should be linked
    //! at least one Way of Path else Agents cannot move towards a Path Way. You
    //! can of course give an orphan Node now and attach to it Ways after (but
    //! before running the game simulation).
    //!
    //! \param[in] resources: Resources that the Agent is carrying.
    //! \param[in] searchTarget: the Unit target (type of destination).
    //--------------------------------------------------------------------------
    Agent(uint32_t id, AgentType const& type, Unit& owner, Resources const& resources,
          std::string const& searchTarget);

    //--------------------------------------------------------------------------
    //! \brief Virtual destructor only needed because of the presence of virtual
    //! methods only needed for unit tests.
    //--------------------------------------------------------------------------
    VIRTUAL ~Agent() = default;

    //--------------------------------------------------------------------------
    //! \brief Make the Agent transporting resources on the current Way and
    //! make unloading resource when the Agent has reached its destination.
    //! \return true when the Agent has reached its destination.
    //! \note VIRTUAL is only used for unit tests.
    //--------------------------------------------------------------------------
    VIRTUAL bool update(Dijkstra& dijkstra);

    // -------------------------------------------------------------------------
    //! \brief Return the unique identifier.
    // -------------------------------------------------------------------------
    uint32_t id() const { return m_id; }

    //--------------------------------------------------------------------------
    //! \brief Return the position inside the World coordinate.
    //--------------------------------------------------------------------------
    Vector3f const& position() const { return m_position; }

    // -------------------------------------------------------------------------
    //! \brief Getter: return the type of Agent.
    // -------------------------------------------------------------------------
    std::string const& type() const { return m_type.name; }

    // -------------------------------------------------------------------------
    //! \brief Return the color for the Renderer.
    // -------------------------------------------------------------------------
    uint32_t color() const { return m_type.color; }

    // -------------------------------------------------------------------------
    //! \brief Return in read only access resources (for debug purpose).
    // -------------------------------------------------------------------------
    std::vector<Resource> const& resources() const { return m_resources.container(); }

    // -------------------------------------------------------------------------
    //! \brief Translate the position of the Agent relatively from its parent
    //! (Node). Only use this fonction when moving place of a City (that will
    //! also affect to its contents).
    // -------------------------------------------------------------------------
    void translate(Vector3f const direction);

private:

    //--------------------------------------------------------------------------
    //! \brief Transfert the amount of resource to the target Unit.
    //! \return true if after if the Agent has no more resource to
    //! carry.
    //--------------------------------------------------------------------------
    bool unloadResources();

    //--------------------------------------------------------------------------
    //! \brief Move the Agent on the current Way towards the destination Node
    //! (border of the segment).
    //--------------------------------------------------------------------------
    void moveTowardsNextNode();

    //--------------------------------------------------------------------------
    //! \brief Search a new Way to reacht the destination Node/Unit.
    //--------------------------------------------------------------------------
    void findNextNode(Dijkstra& dijkstra);

    //--------------------------------------------------------------------------
    //! \brief When the Agent reach the Node of destination it has to transfer
    //! its resource to the good Unit. Indeed several Units can refer to the
    //! same Node.
    //--------------------------------------------------------------------------
    Unit* searchUnit();

private:

    //! \brief Unique identifier
    uint32_t           m_id;
    //! \brief Reference to the type of Agent defined in the simulation script.
    //! The reference shall not be destroyed before this instance.
    AgentType const&   m_type;
    //! \brief The type of Unit to search as destination.
    std::string        m_searchTarget;
    //! \brief Carried resource from an Unit to another Unit.
    Resources          m_resources;
    //! \brief Position of the Agent in the world coordinate. TODO idea:
    //! Vector3f& pointe sur un tableau de Positions qui lui sera utilise pour
    //! le renderer.
    Vector3f           m_position;
    //! \brief Position along m_currentWay from the origin node.
    float              m_offset = 0.0f;
    //! \brief The way this agent is moving along.
    Way               *m_currentWay = nullptr;
    //! \brief Origin node
    //! TODO pourrait etre supprime car m_currentWay->from() et ->to()
    Node              *m_lastNode = nullptr;
    //! \brief Target node
    Node              *m_nextNode = nullptr;
};

using Agents = std::vector<std::unique_ptr<Agent>>;

#endif
