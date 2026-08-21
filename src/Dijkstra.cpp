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

    bool getUnitWithTargetAndCapacity(Node* current,
                                      std::string& searchTarget,
                                      Resources& resources)
    {
        std::vector<Unit*>& units = current->units();

        size_t i = units.size();
        while (i--)
        {
            if (units[i]->accepts(searchTarget, resources))
                return true;
        }

        return false;
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
Node* Dijkstra::findNextPoint(Node& fromNode, std::string& searchTarget,
                              Resources& resources)
{
    m_closedSet.clear();
    m_cameFrom.clear();
    m_scoreFromStart.clear();

    std::priority_queue<QueueEntry, std::vector<QueueEntry>, std::greater<QueueEntry>> open;
    Path* const scope = fromNode.path();

    m_scoreFromStart[&fromNode] = 0.0f;
    open.push({0.0f, 0.0f, &fromNode});

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

        if (getUnitWithTargetAndCapacity(currentNode, searchTarget, resources))
        {
            if (currentNode == &fromNode)
                return currentNode;

            while (m_cameFrom[currentNode] != &fromNode)
                currentNode = m_cameFrom[currentNode];

            return currentNode;
        }

        m_closedSet.insert(currentNode);

        for (auto* way: currentNode->ways())
        {
            Node* neighbor = (&way->from() == currentNode) ? &way->to() : &way->from();

            if (scope != nullptr && neighbor->path() != scope)
                continue;

            float const tentativeG = g + way->magnitude();

            auto const it = m_scoreFromStart.find(neighbor);
            if (it != m_scoreFromStart.end() && tentativeG >= it->second)
                continue;

            if (m_closedSet.find(neighbor) != m_closedSet.end())
                m_closedSet.erase(neighbor);

            m_cameFrom[neighbor] = currentNode;
            m_scoreFromStart[neighbor] = tentativeG;
            open.push({tentativeG + heuristic(*neighbor, fromNode), tentativeG, neighbor});
        }
    }

    return randomNeighbor(fromNode);
}

//------------------------------------------------------------------------------
float Dijkstra::heuristic(Node& p1, Node& p2)
{
    return magnitude(p2.position() - p1.position());
}
