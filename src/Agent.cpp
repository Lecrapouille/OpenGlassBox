//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "OpenGlassBox/Agent.hpp"
#include "OpenGlassBox/Config.hpp"
#include "OpenGlassBox/Unit.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>

namespace ogb
{

static const float MIN_WAY_MAGNITUDE = 1e-6f;
//! \brief How close to the offset of a Unit an Agent has to be to knock at
//! its door.
static const float ARRIVED_OFFSET = 0.05f;

namespace
{
Unit* findAcceptingUnitOnWay(Way const& way,
                             Name const& searchTarget,
                             Resources const& resources)
{
    for (Unit* unit : way.units())
    {
        if (unit->accepts(searchTarget, resources))
            return unit;
    }
    return nullptr;
}
} // namespace

// =============================================================================
// THE STEPS OF FOLLOWING AN ITINERARY
//
// One of these picks the next crossroads to drive to, and followRoute() below
// tries them in order until one succeeds.
// =============================================================================

//------------------------------------------------------------------------------
bool Agent::arrivedAtDestination() const
{
    Route const& route = m_route;
    if (!route.found || route.waypointsLeft())
        return false;

    if (route.approachWay == nullptr)
        return true;

    Way const* const way = currentWay();
    return (route.approachWay == way) &&
           (std::fabs(offset() - route.approachOffset) <= ARRIVED_OFFSET);
}

//------------------------------------------------------------------------------
bool Agent::giveUp()
{
    if (Unit* owner = m_owner)
        m_resources.transferResourcesTo(owner->resources());
    return true;
}

//------------------------------------------------------------------------------
void Agent::setNextNodeFromRoute(Node* next)
{
    m_nextNode = next;
    if (m_lastNode == nullptr)
        return;

    setCurrentWay(m_lastNode->getWayToNode(*next));
    if (m_currentWay != nullptr)
    {
        m_offset =
            (m_lastNode == &m_currentWay->from()) ? 0.0f : 1.0f;
    }
    else
    {
        m_nextNode = nullptr;
    }
}

//------------------------------------------------------------------------------
bool Agent::followRouteWhileAlongWay()
{
    Route const& route = m_route;
    Way* const way = m_currentWay;
    float const offset = m_offset;

    // The destination is a door on this very segment: drive towards whichever
    // end lies past it, and moveTowardsNextNode() stops at the door.
    if (route.found && !route.waypointsLeft() && (route.approachWay == way) &&
        (std::fabs(offset - route.approachOffset) > ARRIVED_OFFSET))
    {
        m_nextNode = (route.approachOffset >= offset) ? &way->to()
                                                      : &way->from();
        return true;
    }

    // Otherwise get off the segment first: routing only happens at crossroads.
    if (Node* const exit = wayExit())
    {
        m_nextNode = exit;
        return true;
    }

    return false;
}

//------------------------------------------------------------------------------
void Agent::followRouteWhenLost(IRouter& router)
{
    if (m_lastNode == nullptr)
        return;

    Node* const next =
        router.findNextPoint(*m_lastNode, m_searchTarget, m_resources);
    if (next == nullptr)
        return;

    m_nextNode = next;
    setCurrentWay(m_lastNode->getWayToNode(*next));
    if (m_currentWay != nullptr)
    {
        m_offset =
            (m_lastNode == &m_currentWay->from()) ? 0.0f : 1.0f;
    }
    else
    {
        m_nextNode = nullptr;
    }
}

//------------------------------------------------------------------------------
void Agent::followRouteAlongNodes()
{
    m_nextNode = m_route.nextWaypoint();
    m_route.takeWaypoint();
    setNextNodeFromRoute(m_nextNode);
}

//------------------------------------------------------------------------------
void Agent::followRouteApproach()
{
    setCurrentWay(m_route.approachWay);
    if (m_lastNode == &m_currentWay->from())
    {
        m_offset = 0.0f;
        m_nextNode = &m_currentWay->to();
    }
    else
    {
        m_offset = 1.0f;
        m_nextNode = &m_currentWay->from();
    }
}

// =============================================================================
// LIFETIME AND ATTACHMENT TO THE ROAD NETWORK
// =============================================================================

//------------------------------------------------------------------------------
Agent::Agent(uint32_t id,
             AgentType const& type,
             Unit& owner,
             Resources const& resources,
             Name const& searchTarget)
    : Entity(id, type, owner.position()),
      m_owner(&owner),
      m_searchTarget(searchTarget),
      m_resources(resources)
{
    if (owner.way() != nullptr && owner.node() == nullptr)
    {
        // Sit on the Way at the Unit offset and walk to the closer
        // intersection before looking for a route.
        m_currentWay = owner.way();
        m_currentWay->addAgent();
        m_offset = owner.wayOffset();
        m_lastNode = owner.accessNode();
    }
    else
    {
        m_lastNode = owner.accessNode();
    }
}

//------------------------------------------------------------------------------
Agent::~Agent()
{
    setCurrentWay(nullptr);
    // Covers the deliveries, the give-ups and the Agents the City takes away
    // when they end up stranded. A City destroys its Agents before its Units,
    // so the building is still there to be told.
    releaseDestination();
}

//------------------------------------------------------------------------------
void Agent::claimDestination()
{
    if ((m_reservation != nullptr) || (m_route.destination == nullptr))
        return;

    m_reservation = m_route.destination;
    m_reservation->reserve();
}

//------------------------------------------------------------------------------
void Agent::releaseDestination()
{
    if (m_reservation == nullptr)
        return;

    m_reservation->release();
    m_reservation = nullptr;
}

//------------------------------------------------------------------------------
void Agent::setRoute(Route&& route)
{
    releaseDestination();
    m_route = std::move(route);
    claimDestination();
}

//------------------------------------------------------------------------------
void Agent::setCurrentWay(Way* way)
{
    // The segment counts the Agents on it, and that count is what the BPR
    // function turns into congestion. Every move between segments has to go
    // through here or a street stays busy for ever.
    if (m_currentWay == way)
        return;

    if (m_currentWay != nullptr)
        m_currentWay->removeAgent();

    m_currentWay = way;

    if (m_currentWay != nullptr)
        m_currentWay->addAgent();
}

// =============================================================================
// WHEN THE WORLD CHANGES UNDER IT
//
// The City calls these when something the Agent refers to is moved or pulled
// down, so that it never reads freed memory nor drives on a road that is gone.
// =============================================================================

//------------------------------------------------------------------------------
void Agent::translate(Vector3f const direction)
{
    m_position += direction;
}

//------------------------------------------------------------------------------
void Agent::relocate(Vector3f const& position,
                     Way* way,
                     float offset,
                     Node* last)
{
    setCurrentWay(way);
    m_position = position;
    m_offset = offset;
    m_lastNode = last;
    m_nextNode = last;
    setRoute(Route{});
}

//------------------------------------------------------------------------------
void Agent::invalidateRoute()
{
    setRoute(Route{});
    m_nextNode = nullptr;
    m_ticksOnRoute = 0u;
}

//------------------------------------------------------------------------------
void Agent::forget(Way const& way)
{
    if (m_route.approachWay == &way)
        invalidateRoute();

    if (m_currentWay != &way)
        return;

    // Step back onto the Node the Agent came from. It may itself be about to
    // go, in which case forget(Node) clears it too and the Agent is stranded.
    setCurrentWay(nullptr);
    invalidateRoute();
    m_offset = 0.0f;
    if (m_lastNode != nullptr)
        m_position = m_lastNode->position();
}

//------------------------------------------------------------------------------
void Agent::forget(Node const& node)
{
    if (uses(node))
        invalidateRoute();

    if (m_nextNode == &node)
        m_nextNode = nullptr;

    if (m_lastNode == &node)
        m_lastNode = nullptr;
}

//------------------------------------------------------------------------------
void Agent::forget(Unit const& unit)
{
    if (m_owner == &unit)
        m_owner = nullptr;

    // City::removeUnit calls this while the building is still standing, which
    // is the last moment a place held there can be given back.
    if (m_reservation == &unit)
        releaseDestination();

    // The itinerary named that building as its destination. Anyone reading the
    // route, the inspector for one, would be reading freed memory.
    if (m_route.destination == &unit)
        invalidateRoute();
}

// =============================================================================
// READING ITS OWN STATE
// =============================================================================

//------------------------------------------------------------------------------
bool Agent::stranded() const
{
    if (m_currentWay != nullptr)
        return false;
    if (m_lastNode == nullptr)
        return true;
    return !m_lastNode->hasWays();
}

//------------------------------------------------------------------------------
bool Agent::uses(Way const& way) const
{
    if (m_currentWay == &way)
        return true;
    if (m_route.approachWay == &way)
        return true;
    return false;
}

//------------------------------------------------------------------------------
bool Agent::uses(Node const& node) const
{
    if ((m_lastNode == &node) || (m_nextNode == &node))
        return true;

    return std::any_of(m_route.begin(),
                       m_route.end(),
                       [&](Node const* n) { return n == &node; });
}

//------------------------------------------------------------------------------
Node* Agent::routingNode() const
{
    return (m_nextNode != nullptr) ? m_nextNode : m_lastNode;
}

//------------------------------------------------------------------------------
Node* Agent::wayExit() const
{
    if ((m_currentWay == nullptr) || (m_lastNode == nullptr))
        return nullptr;

    if ((m_lastNode != &m_currentWay->from()) &&
        (m_lastNode != &m_currentWay->to()))
    {
        return nullptr;
    }

    return m_lastNode;
}

// =============================================================================
// CHOOSING AN ITINERARY
// =============================================================================

//------------------------------------------------------------------------------
float Agent::remainingCost() const
{
    if (!m_route.found)
        return 0.0f;

    // Sum the segments still to drive, then the fraction of the last one that
    // leads to the door.
    float cost = 0.0f;
    Node* current = routingNode();
    for (Node* next : m_route)
    {
        if (current == nullptr || next == nullptr)
            break;
        Way const* way = current->getWayToNode(*next);
        if (way != nullptr)
            cost += way->travelTime();
        current = next;
    }
    if ((m_route.approachWay != nullptr) && (current != nullptr))
    {
        float const fraction = (&m_route.approachWay->from() == current)
                                   ? m_route.approachOffset
                                   : (1.0f - m_route.approachOffset);
        cost += m_route.approachWay->travelTime() * fraction;
    }
    return cost;
}

//------------------------------------------------------------------------------
float Agent::rerouteCost(IRouter& router)
{
    Node* const from = routingNode();
    if (from == nullptr)
        return std::numeric_limits<float>::infinity();

    // Same reason as in update(): the claim is against the other Agents, not
    // against oneself. Restored exactly as it was, this being a measurement.
    bool const held = (m_reservation != nullptr);
    releaseDestination();

    float const cost =
        router.shortestPathCost(*from, m_searchTarget, m_resources);

    if (held)
        claimDestination();

    return cost;
}

//------------------------------------------------------------------------------
void Agent::maybeRecomputeRoute(IRouter& router, SimulationConfig const& config)
{
    // The cheap answers first: no itinerary at all, one that has been held long
    // enough, or a city that asked for a recomputation on every tick.
    if (!m_route.found || (m_ticksOnRoute >= config.pathRecalcTicks) ||
        (config.pathCostDeviation <= 0.0f))
    {
        computeRoute(router);
        return;
    }

    // What follows costs a graph search, so it does not run on every tick.
    uint32_t const period =
        (config.pathCheckTicks == 0u) ? 1u : config.pathCheckTicks;
    if ((m_ticksOnRoute % period) != 0u)
        return;

    Node* from = routingNode();
    if (from == nullptr)
    {
        computeRoute(router);
        return;
    }

    float const remaining = remainingCost();
    if (remaining <= 1e-4f)
        return;

    // Ask for the itinerary rather than for its cost alone: if it turns out to
    // be worth switching to, it is the one the Agent wants, and searching for
    // it a second time would be the same work twice.
    Route candidate = router.findRoute(*from, m_searchTarget, m_resources);
    if (!candidate.found || !std::isfinite(candidate.cost))
        return;

    if (candidate.cost + remaining * config.pathCostDeviation >= remaining)
        return;

    // Standing between two crossroads, the Agent may be better off leaving by
    // the other end, and that is a decision the candidate cannot express.
    if (standingAlongWay())
    {
        computeRouteAlongWay(router);
        return;
    }

    setRoute(std::move(candidate));
    m_ticksOnRoute = 0u;
}

//------------------------------------------------------------------------------
bool Agent::standingAlongWay() const
{
    return (m_currentWay != nullptr) && (m_nextNode == nullptr) &&
           (m_offset > 0.0f) && (m_offset < 1.0f);
}

//------------------------------------------------------------------------------
void Agent::computeRouteAlongWay(IRouter& router)
{
    // Standing between two crossroads, the Agent can leave by either end. Cost
    // both, counting the stretch of the current segment it has to drive first.
    Node& from = m_currentWay->from();
    Node& to = m_currentWay->to();
    float const travel = m_currentWay->travelTime();
    float const infinity = std::numeric_limits<float>::infinity();

    Route byFrom = router.findRoute(from, m_searchTarget, m_resources);
    float const costFrom =
        byFrom.found ? (byFrom.cost + travel * m_offset) : infinity;

    Route byTo = router.findRoute(to, m_searchTarget, m_resources);
    float const costTo =
        byTo.found ? (byTo.cost + travel * (1.0f - m_offset)) : infinity;

    m_ticksOnRoute = 0u;

    if (std::isinf(costFrom) && std::isinf(costTo))
    {
        setRoute(Route());
        return;
    }

    // m_lastNode is the end the Agent drives to before it may take another
    // segment, and followRoute reads it back through wayExit().
    if (costTo < costFrom)
    {
        m_lastNode = &to;
        setRoute(std::move(byTo));
    }
    else
    {
        m_lastNode = &from;
        setRoute(std::move(byFrom));
    }
}

//------------------------------------------------------------------------------
void Agent::computeRoute(IRouter& router)
{
    if (standingAlongWay())
    {
        computeRouteAlongWay(router);
        return;
    }

    Node* from = routingNode();
    if (from == nullptr)
    {
        setRoute(Route());
        return;
    }

    setRoute(router.findRoute(*from, m_searchTarget, m_resources));
    m_ticksOnRoute = 0u;
}

//------------------------------------------------------------------------------
void Agent::followRoute(IRouter& router, SimulationConfig const& config)
{
    ++m_ticksOnRoute;

    maybeRecomputeRoute(router, config);

    // Each case below picks the next crossroads, from the most constrained
    // situation to the least. Standing between two crossroads comes first
    // because the way out of a segment is not a routing decision.
    if ((m_currentWay != nullptr) && (m_offset > 0.0f) && (m_offset < 1.0f) &&
        followRouteWhileAlongWay())
        return;

    // Nothing accepts the load, or nothing is reachable: wander.
    if (!m_route.found)
    {
        followRouteWhenLost(router);
        return;
    }

    // Crossroads still to go through.
    if (m_route.waypointsLeft())
    {
        followRouteAlongNodes();
        return;
    }

    // Last leg: the door stands along a segment rather than at a crossroads.
    if (m_route.approachWay != nullptr)
    {
        followRouteApproach();
        return;
    }

    // Arrived, and the door is right here. Stand still and let update() knock.
    m_nextNode = nullptr;
}

// =============================================================================
// ONE TICK
// =============================================================================

//------------------------------------------------------------------------------
bool Agent::update(IRouter& router, float dt)
{
    static SimulationConfig const defaults;
    return update(router, defaults, dt);
}

//------------------------------------------------------------------------------
bool Agent::update(IRouter& router, SimulationConfig const& config, float dt)
{
    // The claim is against the other Agents, not against oneself. Unit::accepts
    // counts it in, so an Agent holding it through its own tick would find its
    // own destination full: it would be refused at the door it was sent to, and
    // every recomputation would send it somewhere else and back again.
    releaseDestination();
    bool const done = tick(router, config, dt);

    // An Agent that has unloaded or given up is about to be taken away by the
    // City, and has nowhere left to go. Taking the place back only to hand it
    // over in the destructor would hide the building from everyone else for the
    // rest of the tick.
    if (!done)
        claimDestination();

    return done;
}

//------------------------------------------------------------------------------
bool Agent::tick(IRouter& router, SimulationConfig const& config, float dt)
{
    // Standing still: try to deliver, then decide where to go next. Driving:
    // just cover some ground. The two are exclusive, which is what keeps an
    // Agent from knocking at a door it is only passing by.
    if (m_nextNode == nullptr)
    {
        if (unloadResources())
            return true;

        // Stranded off the network, with no crossroads to route from.
        if (m_lastNode == nullptr)
            return false;

        // Standing at the door and refused: the building filled up while the
        // Agent was driving to it. Throwing the itinerary away is what sends it
        // somewhere else, since Dijkstra skips the Units that cannot accept
        // what it carries. Keeping it made the Agent knock for ever.
        if (arrivedAtDestination())
            invalidateRoute();

        followRoute(router, config);
    }
    else
    {
        moveTowardsNextNode(dt);
    }

    // An Agent that never finds anything would drive for ever, so count the
    // ticks spent without an itinerary and hand the load back after a while.
    if (m_route.found)
    {
        m_ticksLost = 0u;
    }
    else if (config.agentGiveUpTicks != 0u)
    {
        ++m_ticksLost;
        if (m_ticksLost >= config.agentGiveUpTicks)
            return giveUp();
    }

    return false;
}

// =============================================================================
// DELIVERING THE LOAD
// =============================================================================

//------------------------------------------------------------------------------
Unit* Agent::searchUnit()
{
    // A door along the current segment, and the Agent has reached its offset.
    if ((m_currentWay != nullptr) && (m_route.approachWay == m_currentWay))
    {
        float const arrived = std::fabs(m_offset - m_route.approachOffset);
        if (arrived <= ARRIVED_OFFSET || m_nextNode == nullptr)
        {
            return findAcceptingUnitOnWay(
                *m_currentWay, m_searchTarget, m_resources);
        }
    }

    if (m_lastNode == nullptr)
        return nullptr;

    // Only knock at a door the Agent is standing at. Along a segment m_lastNode
    // is the end it came from or drives to, which can be a whole street away,
    // and a delivery made from there is a delivery made from nowhere.
    if ((m_currentWay != nullptr) && (m_offset > 0.0f) && (m_offset < 1.0f))
        return nullptr;

    // Otherwise the Agent stands at a crossroads: try the buildings on it.
    std::vector<Unit*>& units = m_lastNode->units();
    size_t i = units.size();
    while (i--)
    {
        if (units[i]->accepts(m_searchTarget, m_resources))
            return units[i];
    }

    return nullptr;
}

//------------------------------------------------------------------------------
bool Agent::unloadResources()
{
    Unit* unit = searchUnit();
    if ((unit != nullptr) && unit->accepts(m_searchTarget, m_resources))
        m_resources.transferResourcesTo(unit->resources());
    return m_resources.isEmpty();
}

// =============================================================================
// DRIVING
//
// Position is derived from m_offset, the fraction of the current segment the
// Agent has covered, so a segment that is moved or resized carries it along.
// =============================================================================

//------------------------------------------------------------------------------
void Agent::moveTowardsNextNode(float dt)
{
    if (m_currentWay == nullptr)
        return;

    // Destination is a point on the current Way rather than a Node.
    if ((m_route.approachWay == m_currentWay) && !m_route.waypointsLeft())
    {
        float const target = m_route.approachOffset;
        float const wayLength = m_currentWay->magnitude();

        // A segment of zero length has no direction to drive along, so land on
        // the door at once rather than divide by it.
        if (wayLength <= MIN_WAY_MAGNITUDE)
        {
            m_offset = target;
            m_position = m_currentWay->positionAt(m_offset);
            m_nextNode = nullptr;
            m_lastNode = (m_offset <= 0.5f) ? &m_currentWay->from()
                                            : &m_currentWay->to();
            return;
        }

        // The door may lie behind the Agent, hence the signed direction.
        float const direction = (target >= m_offset) ? 1.0f : -1.0f;
        m_offset += direction * m_type.speed * dt / wayLength;

        bool const arrived =
            (direction > 0.0f) ? (m_offset >= target) : (m_offset <= target);
        if (arrived)
        {
            m_offset = target;
            m_nextNode = nullptr;
            m_lastNode = (m_offset <= 0.5f) ? &m_currentWay->from()
                                            : &m_currentWay->to();
        }

        m_position = m_currentWay->positionAt(m_offset);
        return;
    }

    // Ordinary case: drive towards a crossroads, m_offset growing towards 1
    // when it is the far end of the segment and towards 0 when it is the near
    // one, since a Way is undirected.
    float direction = (m_nextNode == &(m_currentWay->to())) ? 1.0f : -1.0f;

    float const wayLength = m_currentWay->magnitude();
    if (wayLength <= MIN_WAY_MAGNITUDE)
    {
        m_lastNode = m_nextNode;
        m_nextNode = nullptr;
        m_offset = 0.0f;
        if (m_lastNode != nullptr)
            m_position = m_lastNode->position();
        return;
    }

    m_offset += direction * m_type.speed * dt / wayLength;

    // Reaching either end ends the leg: clearing m_nextNode is what makes
    // update() ask followRoute() for the next one on the tick after.
    if (m_offset < 0.0f)
    {
        m_offset = 0.0f;
        m_lastNode = &m_currentWay->from();
        m_nextNode = nullptr;
    }
    else if (m_offset > 1.0f)
    {
        m_offset = 1.0f;
        m_lastNode = &m_currentWay->to();
        m_nextNode = nullptr;
    }

    m_position = m_currentWay->positionAt(m_offset);
}

} // namespace ogb
