//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "OpenGlassBox/DijkstraRouter.hpp"
#include "OpenGlassBox/Config.hpp"

#include "OpenGlassBox/Building.hpp"
#include "OpenGlassBox/Simulation.hpp"
#include "OpenGlassBox/Vector.hpp"

#include <algorithm>
#include <limits>

namespace ogb
{

namespace
{
//! \brief First building on a crossroads that accepts the load, or nullptr.
Building* acceptingBuildingOnNode(Node const* current,
                                  Name const& searchTarget,
                                  Resources const& resources)
{
    std::vector<Building*> const& buildings = current->getBuildings();
    size_t i = buildings.size();
    while (i--)
    {
        if (buildings[i]->accepts(searchTarget, resources))
            return buildings[i];
    }
    return nullptr;
}

//! \brief First building along a segment that accepts the load, or nullptr.
Building* acceptingBuildingOnSegment(Segment const* segment,
                                     Name const& searchTarget,
                                     Resources const& resources)
{
    std::vector<Building*> const& buildings = segment->getBuildings();
    size_t i = buildings.size();
    while (i--)
    {
        if (buildings[i]->accepts(searchTarget, resources))
            return buildings[i];
    }
    return nullptr;
}
} // namespace

//------------------------------------------------------------------------------
Dijkstra::Dijkstra() : m_rng(std::random_device{}()) {}

//------------------------------------------------------------------------------
void Dijkstra::setRandomSeed(unsigned seed)
{
    m_rng.seed(seed);
}

//------------------------------------------------------------------------------
Node* Dijkstra::findRandomNeighbor(Node const& fromNode)
{
    std::vector<Segment*> const& segments = fromNode.getSegments();
    if (segments.empty())
        return nullptr;

    std::uniform_int_distribution<size_t> dist(0u, segments.size() - 1u);
    Segment const* randomSegment = segments[dist(m_rng)];

    if (&randomSegment->getFrom() == &fromNode)
        return &randomSegment->getTo();
    if (&randomSegment->getTo() == &fromNode)
        return &randomSegment->getFrom();

    return nullptr;
}

//------------------------------------------------------------------------------
void Dijkstra::beginSearch(size_t const nodeCount)
{
    if (m_stamp.size() < nodeCount)
    {
        // The zeros the new entries are filled with never match a generation,
        // so growing costs nothing beyond the allocation itself.
        m_stamp.resize(nodeCount, 0u);
        m_scoreFromStart.resize(nodeCount, 0.0f);
        m_cameFrom.resize(nodeCount, nullptr);
        m_closed.resize(nodeCount, 0u);
    }

    ++m_generation;
    if (m_generation == 0u)
    {
        // Wrapped around after four billion searches. Entries left over from
        // the previous cycle would read as fresh, so retire them by hand. This
        // is the only place any of these arrays is ever written wholesale.
        std::fill(m_stamp.begin(), m_stamp.end(), 0u);
        m_generation = 1u;
    }

    m_open.clear();
}

//------------------------------------------------------------------------------
Route Dijkstra::reconstruct(Node const& fromNode,
                            Node* goalNode,
                            Building* destination,
                            Segment* approachSegment,
                            float approachOffset,
                            float cost)
{
    Route route;
    route.setTarget(destination, approachSegment, approachOffset, cost);

    if (goalNode == nullptr || goalNode == &fromNode)
        return route;

    // Walk the predecessors back to the start, which gives the trip in reverse.
    m_reverse.clear();
    Node* current = goalNode;
    while (current != &fromNode)
    {
        size_t const index = current->getIndex();
        if (!isVisited(index))
            break;

        Node* const previous = m_cameFrom[index];
        if (previous == nullptr)
            break;

        m_reverse.push_back(current);
        current = previous;
    }

    route.setWaypoints(m_reverse.rbegin(), m_reverse.rend());
    return route;
}

//------------------------------------------------------------------------------
Route Dijkstra::findRoute(Node& fromNode,
                          Name const& searchTarget,
                          Resources const& resources)
{
    // Node::getIndex() is what the arrays below are indexed by, and only a Path
    // hands out those indices. A crossroads outside any network is therefore
    // not somewhere a trip can start, and there is nowhere to go from it.
    Path const* const scope = fromNode.getPath();
    if (scope == nullptr)
    {
        return Route();
    }

    // Open the search on the crossroads the traveller stands at.
    beginSearch(scope->getNodeCount());
    setScore(fromNode.getIndex(), 0.0f, nullptr);
    m_open.push_back({ 0.0f, 0.0f, &fromNode });
    std::push_heap(m_open.begin(), m_open.end(), std::greater<QueueEntry>());

    // The cheapest building met so far, and what it costs to reach it. Not the
    // answer yet: the search carries on until every crossroads it could still
    // reach is already more expensive than this.
    Route best;
    float bestCost = ROUTING_INFINITY;

    while (!m_open.empty())
    {
        // Take the crossroads that is cheapest to reach.
        std::pop_heap(m_open.begin(), m_open.end(), std::greater<QueueEntry>());
        QueueEntry const current = m_open.back();
        m_open.pop_back();

        Node* const currentNode = current.node;
        size_t const currentIndex = currentNode->getIndex();
        float const g = current.g;

        if (isStaleEntry(currentIndex, g, bestCost))
        {
            continue;
        }

        // A building on the crossroads itself ends the search: nothing further
        // out can be cheaper, since every segment costs something.
        if (Building* building =
                acceptingBuildingOnNode(currentNode, searchTarget, resources))
        {
            return reconstruct(
                fromNode, currentNode, building, nullptr, 0.0f, g);
        }

        considerBuildingsAlongStreets(
            fromNode, *currentNode, g, searchTarget, resources, best, bestCost);

        m_closed[currentIndex] = 1u;

        relaxStreetsFrom(*currentNode, g, *scope);
    }

    return best;
}

//------------------------------------------------------------------------------
bool Dijkstra::isStaleEntry(size_t const index,
                            float const g,
                            float const bestCost) const
{
    return (g > m_scoreFromStart[index]) || (m_closed[index] != 0u) ||
           (g >= bestCost);
}

//------------------------------------------------------------------------------
void Dijkstra::considerBuildingsAlongStreets(Node const& fromNode,
                                             Node& currentNode,
                                             float const g,
                                             Name const& searchTarget,
                                             Resources const& resources,
                                             Route& best,
                                             float& bestCost)
{
    for (auto* segment : currentNode.getSegments())
    {
        Building* building =
            acceptingBuildingOnSegment(segment, searchTarget, resources);
        if (building == nullptr)
        {
            continue;
        }

        // The building stands part of the way down the street, so only that
        // part of the travel time is paid. Which part depends on which end the
        // traveller comes in by.
        float const extra = segment->getTravelTime() *
                            ((&segment->getFrom() == &currentNode)
                                 ? building->getSegmentOffset()
                                 : (1.0f - building->getSegmentOffset()));
        float const cost = g + extra;
        if (cost < bestCost)
        {
            best = reconstruct(fromNode,
                               &currentNode,
                               building,
                               segment,
                               building->getSegmentOffset(),
                               cost);
            bestCost = cost;
        }
    }
}

//------------------------------------------------------------------------------
void Dijkstra::relaxStreetsFrom(Node& currentNode,
                                float const g,
                                Path const& scope)
{
    for (auto const* segment : currentNode.getSegments())
    {
        Node* const neighbor = (&segment->getFrom() == &currentNode)
                                   ? &segment->getTo()
                                   : &segment->getFrom();

        // A road and a railway may share a crossroads without a trip being
        // allowed to change from one to the other.
        if (neighbor->getPath() != &scope)
        {
            continue;
        }

        // Keep the neighbour only when coming through here is an improvement.
        size_t const neighborIndex = neighbor->getIndex();
        float const tentativeG = g + segment->getTravelTime();

        if (isVisited(neighborIndex) &&
            (tentativeG >= m_scoreFromStart[neighborIndex]))
        {
            continue;
        }

        setScore(neighborIndex, tentativeG, &currentNode);

        m_open.push_back({ tentativeG, tentativeG, neighbor });
        std::push_heap(m_open.begin(), m_open.end(), std::greater<QueueEntry>());
    }
}

//------------------------------------------------------------------------------
Node* Dijkstra::findNextNode(Node& fromNode,
                             Name& searchTarget,
                             Resources& resources)
{
    Route const route = findRoute(fromNode, searchTarget, resources);
    if (!route.isFound())
    {
        return findRandomNeighbor(fromNode);
    }
    if (!route.hasWaypointsLeft())
    {
        return &fromNode;
    }
    return route.getNextWaypoint();
}

//------------------------------------------------------------------------------
float Dijkstra::computeShortestPathCost(Node& fromNode,
                                        Name const& searchTarget,
                                        Resources const& resources)
{
    Route const route = findRoute(fromNode, searchTarget, resources);
    return route.isFound() ? route.getCost() : ROUTING_INFINITY;
}

//------------------------------------------------------------------------------
void installDijkstraRouters(Simulation const& simulation)
{
    for (auto const& [_, city] : simulation.getCities())
    {
        installDijkstraRouter(*city, simulation.getConfig());
    }
}

} // namespace ogb
