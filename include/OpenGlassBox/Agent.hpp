//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file Agent.hpp
//! \brief Travelling entities that follow roads and deliver resources between units.


#ifndef OPEN_GLASSBOX_AGENT_HPP
#  define OPEN_GLASSBOX_AGENT_HPP

#  include "OpenGlassBox/Config.hpp"
#  include "OpenGlassBox/Dijkstra.hpp"
#  include "OpenGlassBox/Path.hpp"

namespace ogb {

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

    Agent(uint32_t id, AgentType const& type, Unit& owner, Resources const& resources,
          std::string const& searchTarget);

    VIRTUAL ~Agent();

    VIRTUAL bool update(Dijkstra& dijkstra, float dt);
    void setConfig(SimulationConfig const& config) { m_simConfig = &config; }

    uint32_t id() const { return m_id; }
    Vector3f const& position() const { return m_position; }
    std::string const& type() const { return m_type.name; }
    uint32_t color() const { return m_type.color; }
    std::vector<Resource> const& resources() const { return m_resources.container(); }
    std::string const& searchTarget() const { return m_searchTarget; }
    Way const* currentWay() const { return m_currentWay; }
    float offset() const { return m_offset; }
    float speed() const { return m_type.speed; }
    Route const& route() const { return m_route; }
    float remainingCost() const { return remainingRouteCost(); }
    Node* lastNode() const { return m_lastNode; }
    Resources& carried() { return m_resources; }
    Resources const& carried() const { return m_resources; }
    Unit* owner() const { return m_owner; }
    void detachOwner() { m_owner = nullptr; }

    void translate(Vector3f const direction);
    void relocate(Vector3f const& position, Way* way, float offset, Node* last);

    //! \brief Drop the cached itinerary so the next tick recomputes it.
    void disruptRoute();

    // -------------------------------------------------------------------------
    //! \brief Forget the cached itinerary without moving: it was computed on a
    //! network that has since changed.
    // -------------------------------------------------------------------------
    void invalidateRoute();

    // -------------------------------------------------------------------------
    //! \brief Let go of a Way that is about to be destroyed. Has to be called
    //! while the Way is still alive, because the Agent has to take itself out
    //! of its traffic count. An Agent driving on it is parked on the Node it
    //! came from.
    // -------------------------------------------------------------------------
    void forget(Way const& way);

    // -------------------------------------------------------------------------
    //! \brief Let go of a Node that is about to be destroyed. Call it after
    //! forget() has been called for every Way incident to that Node.
    // -------------------------------------------------------------------------
    void forget(Node const& node);

    // -------------------------------------------------------------------------
    //! \brief Whether the Agent has nowhere left to stand: no Way under it and
    //! no Node with a road. Such an Agent can neither move nor deliver, and the
    //! City takes it away rather than leave it floating over the map.
    // -------------------------------------------------------------------------
    bool stranded() const;

    bool uses(Way const& way) const;
    bool uses(Node const& node) const;

private:

    bool unloadResources();
    void moveTowardsNextNode(float dt);
    void followRoute(Dijkstra& dijkstra, SimulationConfig const& config);
    bool shouldRecomputeRoute(Dijkstra& dijkstra, SimulationConfig const& config) const;
    void computeRoute(Dijkstra& dijkstra);
    Unit* searchUnit();
    void setCurrentWay(Way* way);
    Node* routingNode() const;
    float remainingRouteCost() const;
    bool update(Dijkstra& dijkstra, SimulationConfig const& config, float dt);

private:

    uint32_t           m_id;
    AgentType const&   m_type;
    Unit*              m_owner = nullptr;
    std::string        m_searchTarget;
    Resources          m_resources;
    Vector3f           m_position;
    float              m_offset = 0.0f;
    Way               *m_currentWay = nullptr;
    Node              *m_lastNode = nullptr;
    Node              *m_nextNode = nullptr;
    //! \brief Cached itinerary. Recomputed periodically or when the remaining
    //! cost drifts too far from the current shortest path.
    Route              m_route;
    uint32_t           m_ticksOnRoute = 0u;
    SimulationConfig const* m_simConfig = nullptr;
};

using Agents = std::vector<std::unique_ptr<Agent>>;

} // namespace ogb

#endif
