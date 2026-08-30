//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "OpenGlassBox/Agent.hpp"
#include "OpenGlassBox/Building.hpp"
#include "OpenGlassBox/Config.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>

namespace ogb
{

static const float MIN_WAY_LENGTH = 1e-6f;
//! \brief How close to the offset of a Building an Agent has to be to knock at
//! its door.
static const float ARRIVED_OFFSET = 0.05f;

namespace
{
Building* findAcceptingBuildingOnSegment(Segment const& segment,
                                         Name const& searchTarget,
                                         Resources const& resources)
{
    for (Building* building : segment.getBuildings())
    {
        if (building->accepts(searchTarget, resources))
        {
            return building;
        }
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
    if (!route.isFound() || route.hasWaypointsLeft())
    {
        return false;
    }

    if (route.getApproachSegment() == nullptr)
    {
        return true;
    }

    Segment const* const segment = getSegment();
    return (route.getApproachSegment() == segment) &&
           (std::fabs(getOffset() - route.getApproachOffset()) <=
            ARRIVED_OFFSET);
}

//------------------------------------------------------------------------------
bool Agent::giveUp()
{
    if (Building* owner = m_owner)
    {
        m_resources.transferTo(owner->getResources());
    }
    return true;
}

//------------------------------------------------------------------------------
void Agent::nextNodeFromRoute(Node* next)
{
    m_nextNode = next;
    if (m_lastNode == nullptr)
    {
        return;
    }

    setCurrentSegment(m_lastNode->findSegmentTo(*next));
    if (m_currentSegment != nullptr)
    {
        m_offset = (m_lastNode == &m_currentSegment->getFrom()) ? 0.0f : 1.0f;
    }
    else
    {
        m_nextNode = nullptr;
    }
}

//------------------------------------------------------------------------------
bool Agent::followRouteWhileAlongSegment()
{
    Route const& route = m_route;
    Segment const* segment = m_currentSegment;
    float const offset = m_offset;

    // The destination is a door on this very segment: drive towards whichever
    // end lies past it, and moveTowardsNextNode() stops at the door.
    if (route.isFound() && !route.hasWaypointsLeft() &&
        (route.getApproachSegment() == segment) &&
        (std::fabs(offset - route.getApproachOffset()) > ARRIVED_OFFSET))
    {
        m_nextNode = (route.getApproachOffset() >= offset)
                         ? &segment->getTo()
                         : &segment->getFrom();
        return true;
    }

    // Otherwise get off the segment first: routing only happens at crossroads.
    if (Node* const exit = getSegmentExit())
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
        router.findNextNode(*m_lastNode, m_searchTarget, m_resources);
    if (next == nullptr)
    {
        return;
    }

    m_nextNode = next;
    setCurrentSegment(m_lastNode->findSegmentTo(*next));
    if (m_currentSegment != nullptr)
    {
        m_offset = (m_lastNode == &m_currentSegment->getFrom()) ? 0.0f : 1.0f;
    }
    else
    {
        m_nextNode = nullptr;
    }
}

//------------------------------------------------------------------------------
void Agent::followRouteAlongNodes()
{
    m_nextNode = m_route.getNextWaypoint();
    m_route.takeWaypoint();
    nextNodeFromRoute(m_nextNode);
}

//------------------------------------------------------------------------------
void Agent::followRouteApproach()
{
    setCurrentSegment(m_route.getApproachSegment());
    if (m_lastNode == &m_currentSegment->getFrom())
    {
        m_offset = 0.0f;
        m_nextNode = &m_currentSegment->getTo();
    }
    else
    {
        m_offset = 1.0f;
        m_nextNode = &m_currentSegment->getFrom();
    }
}

// =============================================================================
// LIFETIME AND ATTACHMENT TO THE ROAD NETWORK
// =============================================================================

//------------------------------------------------------------------------------
Agent::Agent(size_t id,
             AgentType const& type,
             Building& owner,
             Resources const& resources,
             Name const& searchTarget)
    : Entity(id, type, owner.getPosition()),
      m_owner(&owner),
      m_resources(resources),
      m_searchTarget(searchTarget)
{
    if (owner.getSegment() != nullptr && owner.getNode() == nullptr)
    {
        // Sit on the Segment at the Building offset and walk to the closer
        // intersection before looking for a route.
        m_currentSegment = owner.getSegment();
        m_currentSegment->addAgent();
        m_offset = owner.getSegmentOffset();
        m_lastNode = owner.getAccessNode();
    }
    else
    {
        m_lastNode = owner.getAccessNode();
    }
}

//------------------------------------------------------------------------------
Agent::~Agent()
{
    setCurrentSegment(nullptr);
    // Covers the deliveries, the give-ups and the Agents the City takes away
    // when they end up stranded. A City destroys its Agents before its
    // Buildings, so the building is still there to be told.
    releaseDestination();
}

//------------------------------------------------------------------------------
void Agent::claimDestination()
{
    if ((m_reservation != nullptr) || (m_route.getDestination() == nullptr))
    {
        return;
    }

    m_reservation = m_route.getDestination();
    m_reservation->reserve();
}

//------------------------------------------------------------------------------
void Agent::releaseDestination()
{
    if (m_reservation == nullptr)
    {
        return;
    }

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
void Agent::setCurrentSegment(Segment* segment)
{
    // The segment counts the Agents on it, and that count is what the BPR
    // function turns into congestion. Every move between segments has to go
    // through here or a street stays busy for ever.
    if (m_currentSegment == segment)
    {
        return;
    }

    if (m_currentSegment != nullptr)
    {
        m_currentSegment->removeAgent();
    }

    m_currentSegment = segment;
    if (m_currentSegment != nullptr)
    {
        m_currentSegment->addAgent();
    }
}

// =============================================================================
// WHEN THE WORLD CHANGES UNDER IT
//
// The City calls these when something the Agent refers to is moved or pulled
// down, so that it never reads freed memory nor drives on a road that is gone.
// =============================================================================

//------------------------------------------------------------------------------
void Agent::relocate(Vector3f const& position,
                     Segment* segment,
                     float offset,
                     Node* last)
{
    setCurrentSegment(segment);
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
void Agent::forget(Segment const& segment)
{
    if (m_route.getApproachSegment() == &segment)
    {
        invalidateRoute();
    }

    if (m_currentSegment != &segment)
        return;

    // Step back onto the Node the Agent came from. It may itself be about to
    // go, in which case forget(Node) clears it too and the Agent is stranded.
    setCurrentSegment(nullptr);
    invalidateRoute();
    m_offset = 0.0f;
    if (m_lastNode != nullptr)
    {
        m_position = m_lastNode->getPosition();
    }
}

//------------------------------------------------------------------------------
void Agent::forget(Node const& node)
{
    if (uses(node))
    {
        invalidateRoute();
    }

    if (m_nextNode == &node)
    {
        m_nextNode = nullptr;
    }

    if (m_lastNode == &node)
    {
        m_lastNode = nullptr;
    }
}

//------------------------------------------------------------------------------
void Agent::forget(Building const& building)
{
    if (m_owner == &building)
    {
        m_owner = nullptr;
    }

    // City::removeBuilding calls this while the building is still standing,
    // which is the last moment a place held there can be given back.
    if (m_reservation == &building)
    {
        releaseDestination();
    }

    // The itinerary named that building as its destination. Anyone reading the
    // route, the inspector for one, would be reading freed memory.
    if (m_route.getDestination() == &building)
    {
        invalidateRoute();
    }
}

// =============================================================================
// READING ITS OWN STATE
// =============================================================================

//------------------------------------------------------------------------------
bool Agent::isStuck() const
{
    if (m_currentSegment != nullptr)
    {
        return false;
    }
    if (m_lastNode == nullptr)
    {
        return true;
    }
    return !m_lastNode->hasSegments();
}

//------------------------------------------------------------------------------
bool Agent::uses(Segment const& segment) const
{
    if (m_currentSegment == &segment)
    {
        return true;
    }
    if (m_route.getApproachSegment() == &segment)
    {
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
bool Agent::uses(Node const& node) const
{
    if ((m_lastNode == &node) || (m_nextNode == &node))
    {
        return true;
    }

    return std::any_of(m_route.begin(),
                       m_route.end(),
                       [&](Node const* n) { return n == &node; });
}

//------------------------------------------------------------------------------
Node* Agent::getNextRoutingNode() const
{
    return (m_nextNode != nullptr) ? m_nextNode : m_lastNode;
}

//------------------------------------------------------------------------------
Node* Agent::getSegmentExit() const
{
    if ((m_currentSegment == nullptr) || (m_lastNode == nullptr))
    {
        return nullptr;
    }

    if ((m_lastNode != &m_currentSegment->getFrom()) &&
        (m_lastNode != &m_currentSegment->getTo()))
    {
        return nullptr;
    }

    return m_lastNode;
}

// =============================================================================
// CHOOSING AN ITINERARY
// =============================================================================

//------------------------------------------------------------------------------
float Agent::getRemainingCost() const
{
    if (!m_route.isFound())
    {
        return 0.0f;
    }

    // Sum the segments still to drive, then the fraction of the last one that
    // leads to the door.
    float cost = 0.0f;
    Node const* current = getNextRoutingNode();
    for (Node const* next : m_route)
    {
        if (current == nullptr || next == nullptr)
        {
            break;
        }
        Segment const* segment = current->findSegmentTo(*next);
        if (segment != nullptr)
        {
            cost += segment->getTravelTime();
        }
        current = next;
    }
    if ((m_route.getApproachSegment() != nullptr) && (current != nullptr))
    {
        float const fraction =
            (&m_route.getApproachSegment()->getFrom() == current)
                ? m_route.getApproachOffset()
                : (1.0f - m_route.getApproachOffset());
        cost += m_route.getApproachSegment()->getTravelTime() * fraction;
    }
    return cost;
}

//------------------------------------------------------------------------------
float Agent::computeRerouteCost(IRouter& router)
{
    Node* const from = getNextRoutingNode();
    if (from == nullptr)
    {
        return ROUTING_INFINITY;
    }

    // Same reason as in update(): the claim is against the other Agents, not
    // against oneself. Restored exactly as it was, this being a measurement.
    bool const held = (m_reservation != nullptr);
    releaseDestination();

    float const cost =
        router.computeShortestPathCost(*from, m_searchTarget, m_resources);

    if (held)
    {
        claimDestination();
    }

    return cost;
}

//------------------------------------------------------------------------------
void Agent::maybeRecomputeRoute(IRouter& router, RoutingConfig const& config)
{
    // The cheap answers first: no itinerary at all, one that has been held long
    // enough, or a city that asked for a recomputation on every tick.
    if (!m_route.isFound() || (m_ticksOnRoute >= config.pathRecalcTicks) ||
        (config.pathCostDeviation <= 0.0f))
    {
        computeRoute(router);
        return;
    }

    // What follows costs a graph search, so it does not run on every tick.
    uint32_t const period =
        (config.pathCheckTicks == 0u) ? 1u : config.pathCheckTicks;
    if ((m_ticksOnRoute % period) != 0u)
    {
        return;
    }

    switchToCheaperRoute(router, config);
}

//------------------------------------------------------------------------------
void Agent::switchToCheaperRoute(IRouter& router, RoutingConfig const& config)
{
    // A search starts from a crossroads. Without one there is nothing to
    // compare against, so fall back on rebuilding the itinerary outright.
    Node* from = getNextRoutingNode();
    if (from == nullptr)
    {
        computeRoute(router);
        return;
    }

    // Almost there: no way round can save anything worth the search.
    float const remaining = getRemainingCost();
    if (remaining <= 1e-4f)
    {
        return;
    }

    // Ask for the itinerary rather than for its cost alone: if it turns out to
    // be worth switching to, it is the one the Agent wants, and searching for
    // it a second time would be the same work twice.
    Route candidate = router.findRoute(*from, m_searchTarget, m_resources);
    if (!candidate.isFound() || !std::isfinite(candidate.getCost()))
    {
        return;
    }

    // A new way has to save a share of what is left, not merely a second of it.
    // Without that margin every Agent would swap streets on every check and the
    // traffic would oscillate between two routes.
    if (candidate.getCost() + remaining * config.pathCostDeviation >= remaining)
    {
        return;
    }

    // Standing between two crossroads, the Agent may be better off leaving by
    // the other end, and that is a decision the candidate cannot express.
    if (standingAlongSegment())
    {
        computeRouteAlongSegment(router);
        return;
    }

    setRoute(std::move(candidate));
    m_ticksOnRoute = 0u;
}

//------------------------------------------------------------------------------
bool Agent::standingAlongSegment() const
{
    return (m_currentSegment != nullptr) && (m_nextNode == nullptr) &&
           (m_offset > 0.0f) && (m_offset < 1.0f);
}

//------------------------------------------------------------------------------
void Agent::computeRouteAlongSegment(IRouter& router)
{
    // Standing between two crossroads, the Agent can leave by either end. Cost
    // both, counting the stretch of the current segment it has to drive first.
    Node& from = m_currentSegment->getFrom();
    Node& to = m_currentSegment->getTo();
    float const travel = m_currentSegment->getTravelTime();
    float const infinity = ROUTING_INFINITY;

    Route byFrom = router.findRoute(from, m_searchTarget, m_resources);
    float const costFrom =
        byFrom.isFound() ? (byFrom.getCost() + travel * m_offset) : infinity;

    Route byTo = router.findRoute(to, m_searchTarget, m_resources);
    float const costTo = byTo.isFound()
                             ? (byTo.getCost() + travel * (1.0f - m_offset))
                             : infinity;

    m_ticksOnRoute = 0u;

    if (routingCostUnreachable(costFrom) && routingCostUnreachable(costTo))
    {
        setRoute(Route());
        return;
    }

    // m_lastNode is the end the Agent drives to before it may take another
    // segment, and followRoute reads it back through getSegmentExit().
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
    if (standingAlongSegment())
    {
        computeRouteAlongSegment(router);
        return;
    }

    Node* from = getNextRoutingNode();
    if (from == nullptr)
    {
        setRoute(Route());
        return;
    }

    setRoute(router.findRoute(*from, m_searchTarget, m_resources));
    m_ticksOnRoute = 0u;
}

//------------------------------------------------------------------------------
void Agent::followRoute(IRouter& router, RoutingConfig const& config)
{
    ++m_ticksOnRoute;

    maybeRecomputeRoute(router, config);

    // Each case below picks the next crossroads, from the most constrained
    // situation to the least. Standing between two crossroads comes first
    // because the way out of a segment is not a routing decision.
    if ((m_currentSegment != nullptr) && (m_offset > 0.0f) &&
        (m_offset < 1.0f) && followRouteWhileAlongSegment())
        return;

    // Nothing accepts the load, or nothing is reachable: wander.
    if (!m_route.isFound())
    {
        followRouteWhenLost(router);
        return;
    }

    // Crossroads still to go through.
    if (m_route.hasWaypointsLeft())
    {
        followRouteAlongNodes();
        return;
    }

    // Last leg: the door stands along a segment rather than at a crossroads.
    if (m_route.getApproachSegment() != nullptr)
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
bool Agent::update(IRouter& router, RoutingConfig const& config, float dt)
{
    // The claim is against the other Agents, not against oneself.
    // Building::accepts counts it in, so an Agent holding it through its own
    // tick would find its own destination full: it would be refused at the door
    // it was sent to, and every recomputation would send it somewhere else and
    // back again.
    releaseDestination();
    bool const done = tick(router, config, dt);

    // An Agent that has unloaded or given up is about to be taken away by the
    // City, and has nowhere left to go. Taking the place back only to hand it
    // over in the destructor would hide the building from everyone else for the
    // rest of the tick.
    if (!done)
    {
        claimDestination();
    }

    return done;
}

//------------------------------------------------------------------------------
bool Agent::tick(IRouter& router, RoutingConfig const& config, float dt)
{
    // Standing still: try to deliver, then decide where to go next. Driving:
    // just cover some ground. The two are exclusive, which is what keeps an
    // Agent from knocking at a door it is only passing by.
    if (m_nextNode == nullptr)
    {
        if (unloadResources())
        {
            return true;
        }

        // Stranded off the network, with no crossroads to route from.
        if (m_lastNode == nullptr)
        {
            return false;
        }

        // Standing at the door and refused: the building filled up while the
        // Agent was driving to it. Throwing the itinerary away is what sends it
        // somewhere else, since Dijkstra skips the buildings that cannot accept
        // what it carries. Keeping it made the Agent knock for ever.
        if (arrivedAtDestination())
        {
            invalidateRoute();
        }

        followRoute(router, config);
    }
    else
    {
        moveTowardsNextNode(dt);
    }

    // An Agent that never finds anything would drive for ever, so count the
    // ticks spent without an itinerary and hand the load back after a while.
    if (m_route.isFound())
    {
        m_ticksLost = 0u;
    }
    else if (config.agentGiveUpTicks != 0u)
    {
        ++m_ticksLost;
        if (m_ticksLost >= config.agentGiveUpTicks)
        {
            return giveUp();
        }
    }

    return false;
}

// =============================================================================
// DELIVERING THE LOAD
// =============================================================================

//------------------------------------------------------------------------------
Building* Agent::searchBuilding()
{
    // A door along the current segment, and the Agent has reached its offset.
    if ((m_currentSegment != nullptr) &&
        (m_route.getApproachSegment() == m_currentSegment))
    {
        float const arrived = std::fabs(m_offset - m_route.getApproachOffset());
        if (arrived <= ARRIVED_OFFSET || m_nextNode == nullptr)
        {
            return findAcceptingBuildingOnSegment(
                *m_currentSegment, m_searchTarget, m_resources);
        }
    }

    if (m_lastNode == nullptr)
    {
        return nullptr;
    }

    // Only knock at a door the Agent is standing at. Along a segment m_lastNode
    // is the end it came from or drives to, which can be a whole street away,
    // and a delivery made from there is a delivery made from nowhere.
    if ((m_currentSegment != nullptr) && (m_offset > 0.0f) && (m_offset < 1.0f))
    {
        return nullptr;
    }

    // Otherwise the Agent stands at a crossroads: try the buildings on it.
    std::vector<Building*> const& buildings = m_lastNode->getBuildings();
    size_t i = buildings.size();
    while (i--)
    {
        if (buildings[i]->accepts(m_searchTarget, m_resources))
        {
            return buildings[i];
        }
    }

    return nullptr;
}

//------------------------------------------------------------------------------
bool Agent::unloadResources()
{
    Building* building = searchBuilding();
    if ((building != nullptr) && building->accepts(m_searchTarget, m_resources))
    {
        m_resources.transferTo(building->getResources());
    }
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
    if (m_currentSegment == nullptr)
    {
        return;
    }

    // The destination stands on this very Segment, so the Agent drives to a
    // point on it rather than to one of its ends.
    if ((m_route.getApproachSegment() == m_currentSegment) &&
        !m_route.hasWaypointsLeft())
    {
        driveTowardsDoor(dt);
        return;
    }

    driveTowardsCrossroads(dt);
}

//------------------------------------------------------------------------------
void Agent::driveTowardsDoor(float dt)
{
    float const target = m_route.getApproachOffset();
    float const segmentLength = m_currentSegment->getLength();

    // A segment of zero length has no direction to drive along, so land on
    // the door at once rather than divide by it.
    if (segmentLength <= MIN_WAY_LENGTH)
    {
        m_offset = target;
        m_position = m_currentSegment->getPositionAt(m_offset);
        m_nextNode = nullptr;
        m_lastNode = (m_offset <= 0.5f) ? &m_currentSegment->getFrom()
                                        : &m_currentSegment->getTo();
        return;
    }

    // The door may lie behind the Agent, hence the signed direction.
    float const direction = (target >= m_offset) ? 1.0f : -1.0f;
    m_offset += direction * m_type.speed * dt / segmentLength;

    // Landing on the door ends the leg. The nearer end of the segment becomes
    // the Node the Agent is attached to, since it stands between the two.
    bool const arrived =
        (direction > 0.0f) ? (m_offset >= target) : (m_offset <= target);
    if (arrived)
    {
        m_offset = target;
        m_nextNode = nullptr;
        m_lastNode = (m_offset <= 0.5f) ? &m_currentSegment->getFrom()
                                        : &m_currentSegment->getTo();
    }

    m_position = m_currentSegment->getPositionAt(m_offset);
}

//------------------------------------------------------------------------------
void Agent::driveTowardsCrossroads(float dt)
{
    // A Segment is undirected: the offset grows towards 1 when the crossroads
    // ahead is the far end, and shrinks towards 0 when it is the near one.
    float const direction =
        (m_nextNode == &(m_currentSegment->getTo())) ? 1.0f : -1.0f;

    // Same as above: a segment with no length cannot be driven along, so step
    // straight onto the crossroads.
    float const segmentLength = m_currentSegment->getLength();
    if (segmentLength <= MIN_WAY_LENGTH)
    {
        m_lastNode = m_nextNode;
        m_nextNode = nullptr;
        m_offset = 0.0f;
        if (m_lastNode != nullptr)
        {
            m_position = m_lastNode->getPosition();
        }
        return;
    }

    m_offset += direction * m_type.speed * dt / segmentLength;

    // Reaching either end ends the leg: clearing m_nextNode is what makes
    // update() ask followRoute() for the next one on the tick after.
    if (m_offset < 0.0f)
    {
        m_offset = 0.0f;
        m_lastNode = &m_currentSegment->getFrom();
        m_nextNode = nullptr;
    }
    else if (m_offset > 1.0f)
    {
        m_offset = 1.0f;
        m_lastNode = &m_currentSegment->getTo();
        m_nextNode = nullptr;
    }

    m_position = m_currentSegment->getPositionAt(m_offset);
}

} // namespace ogb
