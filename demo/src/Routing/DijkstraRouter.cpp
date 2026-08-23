//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "Routing/DijkstraRouter.hpp"

#include "OpenGlassBox/Unit.hpp"
#include "OpenGlassBox/Vector.hpp"

#include <algorithm>
#include <limits>

namespace ogb
{

namespace
{
//! \brief First building on a crossroads that accepts the load, or nullptr.
Unit* acceptingUnitOnNode(Node* current,
                          Name const& searchTarget,
                          Resources const& resources)
{
    std::vector<Unit*>& units = current->units();
    size_t i = units.size();
    while (i--)
    {
        if (units[i]->accepts(searchTarget, resources))
            return units[i];
    }
    return nullptr;
}

//! \brief First building along a segment that accepts the load, or nullptr.
Unit* acceptingUnitOnWay(Way* way,
                         Name const& searchTarget,
                         Resources const& resources)
{
    std::vector<Unit*>& units = way->units();
    size_t i = units.size();
    while (i--)
    {
        if (units[i]->accepts(searchTarget, resources))
            return units[i];
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
Node* Dijkstra::randomNeighbor(Node& fromNode)
{
    std::vector<Way*>& ways = fromNode.ways();
    if (ways.empty())
        return nullptr;

    std::uniform_int_distribution<size_t> dist(0u, ways.size() - 1u);
    Way* randomSegment = ways[dist(m_rng)];

    if (&randomSegment->from() == &fromNode)
        return &randomSegment->to();
    if (&randomSegment->to() == &fromNode)
        return &randomSegment->from();

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
                            Unit* destination,
                            Way* approachWay,
                            float approachOffset,
                            float cost)
{
    Route route;
    route.found = true;
    route.destination = destination;
    route.approachWay = approachWay;
    route.approachOffset = approachOffset;
    route.cost = cost;

    if (goalNode == nullptr || goalNode == &fromNode)
        return route;

    // Walk the predecessors back to the start, which gives the trip in reverse.
    m_reverse.clear();
    Node* current = goalNode;
    while (current != &fromNode)
    {
        uint32_t const index = current->index();
        if (!visited(index))
            break;

        Node* const previous = m_cameFrom[index];
        if (previous == nullptr)
            break;

        m_reverse.push_back(current);
        current = previous;
    }

    route.nodes.assign(m_reverse.rbegin(), m_reverse.rend());
    return route;
}

//------------------------------------------------------------------------------
Route Dijkstra::findRoute(Node& fromNode,
                          Name const& searchTarget,
                          Resources const& resources)
{
    // Node::index() is what the arrays below are indexed by, and only a Path
    // hands out those indices. A crossroads outside any network is therefore
    // not somewhere a trip can start, and there is nowhere to go from it.
    Path const* const scope = fromNode.path();
    if (scope == nullptr)
        return Route();

    beginSearch(scope->nodeCount());

    float const maxSpeed = scope->maxFreeFlowSpeed();

    setScore(fromNode.index(), 0.0f, nullptr);
    m_open.push_back({ 0.0f, 0.0f, &fromNode });
    std::push_heap(m_open.begin(), m_open.end(), std::greater<QueueEntry>());

    // The cheapest building met so far, and what it costs to reach it. Not the
    // answer yet: the search carries on until every crossroads it could still
    // reach is already more expensive than this.
    Route best;
    float bestCost = std::numeric_limits<float>::infinity();

    while (!m_open.empty())
    {
        std::pop_heap(m_open.begin(), m_open.end(), std::greater<QueueEntry>());
        QueueEntry const current = m_open.back();
        m_open.pop_back();

        Node* const currentNode = current.node;
        uint32_t const currentIndex = currentNode->index();
        float const g = current.g;

        // A cheaper way of reaching this crossroads was found after this entry
        // was queued, it has been expanded already, or nothing beyond it can
        // beat what is already in hand.
        if (g > m_scoreFromStart[currentIndex])
            continue;
        if (m_closed[currentIndex] != 0u)
            continue;
        if (g >= bestCost)
            continue;

        // A building on the crossroads itself ends the search: nothing further
        // out can be cheaper, since every segment costs something.
        if (Unit* unit =
                acceptingUnitOnNode(currentNode, searchTarget, resources))
        {
            return reconstruct(fromNode, currentNode, unit, nullptr, 0.0f, g);
        }

        // A building standing along one of the streets is only a candidate: a
        // crossroads one hop further may hold a cheaper one.
        for (auto* way : currentNode->ways())
        {
            Unit* unit = acceptingUnitOnWay(way, searchTarget, resources);
            if (unit == nullptr)
                continue;

            float const extra =
                way->travelTime() * ((&way->from() == currentNode)
                                         ? unit->wayOffset()
                                         : (1.0f - unit->wayOffset()));
            float const cost = g + extra;
            if (cost < bestCost)
            {
                best = reconstruct(
                    fromNode, currentNode, unit, way, unit->wayOffset(), cost);
                bestCost = cost;
            }
        }

        m_closed[currentIndex] = 1u;

        // Relax the streets leaving the crossroads.
        for (auto* way : currentNode->ways())
        {
            Node* const neighbor =
                (&way->from() == currentNode) ? &way->to() : &way->from();

            // A road and a railway may share a crossroads without a trip being
            // allowed to change from one to the other.
            if (neighbor->path() != scope)
                continue;

            uint32_t const neighborIndex = neighbor->index();
            float const tentativeG = g + way->travelTime();

            if (visited(neighborIndex) &&
                (tentativeG >= m_scoreFromStart[neighborIndex]))
                continue;

            // Reopens the neighbour, which is the point: the heuristic is not
            // admissible, so a crossroads already expanded may still turn out
            // to be reachable more cheaply.
            setScore(neighborIndex, tentativeG, currentNode);

            m_open.push_back(
                { tentativeG + heuristic(*neighbor, fromNode, maxSpeed),
                  tentativeG,
                  neighbor });
            std::push_heap(
                m_open.begin(), m_open.end(), std::greater<QueueEntry>());
        }
    }

    return best;
}

//------------------------------------------------------------------------------
Node* Dijkstra::findNextPoint(Node& fromNode,
                              Name& searchTarget,
                              Resources& resources)
{
    Route const route = findRoute(fromNode, searchTarget, resources);
    if (!route.found)
        return randomNeighbor(fromNode);
    if (!route.waypointsLeft())
        return &fromNode;
    return route.nextWaypoint();
}

//------------------------------------------------------------------------------
float Dijkstra::shortestPathCost(Node& fromNode,
                                 Name const& searchTarget,
                                 Resources const& resources)
{
    Route const route = findRoute(fromNode, searchTarget, resources);
    return route.found ? route.cost : std::numeric_limits<float>::infinity();
}

//------------------------------------------------------------------------------
float Dijkstra::heuristic(Node const& p1,
                          Node const& p2,
                          float maxFreeFlowSpeed) const
{
    return magnitude(p2.position() - p1.position()) / maxFreeFlowSpeed;
}

} // namespace ogb
