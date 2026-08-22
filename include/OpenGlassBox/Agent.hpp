//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

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
    Resources const& carried() const { return m_resources; }

    void translate(Vector3f const direction);

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
