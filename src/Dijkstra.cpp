//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "OpenGlassBox/Dijkstra.hpp"
#include "OpenGlassBox/Vector.hpp"
#include "OpenGlassBox/Unit.hpp"
#include <queue>
#include <limits>

namespace
{
    struct QueueEntry
    {
        float f;
        float g;
        Node* node;

        bool operator>(QueueEntry const& other) const
        {
            if (f != other.f)
                return f > other.f;
            return g > other.g;
        }
    };

    Unit* acceptingUnitOnNode(Node* current, std::string const& searchTarget,
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

    Unit* acceptingUnitOnWay(Way* way, std::string const& searchTarget,
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
}

//------------------------------------------------------------------------------
Dijkstra::Dijkstra()
    : m_rng(std::random_device{}())
{}

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
Route Dijkstra::reconstruct(Node& fromNode, Node* goalNode, Unit* destination,
                            Way* approachWay, float approachOffset,
                            float cost) const
{
    Route route;
    route.found = true;
    route.destination = destination;
    route.approachWay = approachWay;
    route.approachOffset = approachOffset;
    route.cost = cost;

    if (goalNode == nullptr || goalNode == &fromNode)
        return route;

    std::vector<Node*> reverse;
    Node* current = goalNode;
    while (current != &fromNode)
    {
        auto const it = m_cameFrom.find(current);
        if (it == m_cameFrom.end())
            break;
        reverse.push_back(current);
        current = it->second;
    }

    route.nodes.assign(reverse.rbegin(), reverse.rend());
    return route;
}

//------------------------------------------------------------------------------
Route Dijkstra::findRoute(Node& fromNode, std::string const& searchTarget,
                          Resources const& resources)
{
    m_closedSet.clear();
    m_cameFrom.clear();
    m_scoreFromStart.clear();

    std::priority_queue<QueueEntry, std::vector<QueueEntry>, std::greater<QueueEntry>> open;
    Path* const scope = fromNode.path();
    float const maxSpeed = (scope != nullptr) ? scope->maxFreeFlowSpeed() : 1.0f;

    m_scoreFromStart[&fromNode] = 0.0f;
    open.push({0.0f, 0.0f, &fromNode});

    Route best;
    float bestCost = std::numeric_limits<float>::infinity();

    while (!open.empty())
    {
        QueueEntry const current = open.top();
        open.pop();

        Node* currentNode = current.node;
        float const g = current.g;

        if (g > m_scoreFromStart[currentNode])
            continue;
        if (m_closedSet.find(currentNode) != m_closedSet.end())
            continue;
        if (g >= bestCost)
            continue;

        if (Unit* unit = acceptingUnitOnNode(currentNode, searchTarget, resources))
        {
            return reconstruct(fromNode, currentNode, unit, nullptr, 0.0f, g);
        }

        // A Unit sitting on an incident Way is reached from this Node by
        // travelling a fraction of the segment. It is a candidate, not an
        // immediate winner: another Node a hop away may hold a cheaper Unit.
        for (auto* way: currentNode->ways())
        {
            Unit* unit = acceptingUnitOnWay(way, searchTarget, resources);
            if (unit == nullptr)
                continue;

            float const extra = way->travelTime() * (
                (&way->from() == currentNode) ? unit->wayOffset()
                                              : (1.0f - unit->wayOffset()));
            float const cost = g + extra;
            if (cost < bestCost)
            {
                best = reconstruct(fromNode, currentNode, unit, way,
                                   unit->wayOffset(), cost);
                bestCost = cost;
            }
        }

        m_closedSet.insert(currentNode);

        for (auto* way: currentNode->ways())
        {
            Node* neighbor = (&way->from() == currentNode) ? &way->to() : &way->from();

            if (scope != nullptr && neighbor->path() != scope)
                continue;

            float const tentativeG = g + way->travelTime();

            auto const it = m_scoreFromStart.find(neighbor);
            if (it != m_scoreFromStart.end() && tentativeG >= it->second)
                continue;

            if (m_closedSet.find(neighbor) != m_closedSet.end())
                m_closedSet.erase(neighbor);

            m_cameFrom[neighbor] = currentNode;
            m_scoreFromStart[neighbor] = tentativeG;
            open.push({tentativeG + heuristic(*neighbor, fromNode, maxSpeed),
                       tentativeG, neighbor});
        }
    }

    return best;
}

//------------------------------------------------------------------------------
Node* Dijkstra::findNextPoint(Node& fromNode, std::string& searchTarget,
                              Resources& resources)
{
    Route const route = findRoute(fromNode, searchTarget, resources);
    if (!route.found)
        return randomNeighbor(fromNode);
    if (route.nodes.empty())
        return &fromNode;
    return route.nodes.front();
}

//------------------------------------------------------------------------------
float Dijkstra::shortestPathCost(Node& fromNode, std::string const& searchTarget,
                                 Resources const& resources)
{
    Route const route = findRoute(fromNode, searchTarget, resources);
    return route.found ? route.cost : std::numeric_limits<float>::infinity();
}

//------------------------------------------------------------------------------
float Dijkstra::heuristic(Node& p1, Node& p2, float maxFreeFlowSpeed) const
{
    return magnitude(p2.position() - p1.position()) / maxFreeFlowSpeed;
}
