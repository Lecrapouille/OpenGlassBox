//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "Core/Dijkstra.hpp"
#include "Core/Vector.hpp"
#include "Core/Unit.hpp"
#include <algorithm>

//------------------------------------------------------------------------------
template<class T>
static void remove(std::vector<T>& vec, T item)
{
    size_t i = vec.size();
    while (i--)
    {
        if (vec[i] == item)
        {
            // Swap the position of the Agent with the last in the vector and
            // remove the last item of the vector.
            std::swap(vec[i], vec[vec.size() - 1u]);
            vec.pop_back();
        }
    }
}

//------------------------------------------------------------------------------
template<class T, class U>
static void remove(std::map<T, U>& map, T item)
{
    map.erase(map.find(item));
}

//------------------------------------------------------------------------------
template<class T>
static bool contains(std::vector<T>& vec, T item)
{
    return std::find(vec.begin(), vec.end(), item) != vec.end();
}

//------------------------------------------------------------------------------
static bool getUnitWithTargetAndCapacity(Node* current,
                                         std::string & searchTarget,
                                         Resources & resources)
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

//------------------------------------------------------------------------------
Node* Dijkstra::findNextPoint(Node & fromNode, std::string & searchTarget,
                              Resources & resources)
{
    m_closedSet.clear();
    m_openSet.clear();
    m_cameFrom.clear();
    m_scoreFromStart.clear();
    m_scorePlusHeuristicFromStart.clear();

    m_openSet.push_back(&fromNode);
    m_scoreFromStart[&fromNode] = m_scorePlusHeuristicFromStart[&fromNode] = 0.0f;

    while (m_openSet.size() > 0u)
    {
        Node* current = getPointWithLowestScorePlusHeuristicFromStart();
        if (current == nullptr)
            break;

        if (getUnitWithTargetAndCapacity(current, searchTarget, resources))
        {
            if (current == &fromNode)
                return current;

            while (m_cameFrom[current] != &fromNode)
                current = m_cameFrom[current];

            return current;
        }

        remove(m_openSet, current);
        m_closedSet.insert(current);

        for (auto& way: current->ways())
        {
            Node* neighbor = (&way->from() == current) ? &way->to() : &way->from();
            float neighborScoreFromStart = m_scoreFromStart[current] + way->magnitude();

            if (m_closedSet.find(neighbor) != m_closedSet.end())
            {
                if (neighborScoreFromStart >= m_scoreFromStart[neighbor])
                    continue;
            }

            if (!contains(m_openSet, neighbor) || neighborScoreFromStart < m_scoreFromStart[neighbor])
            {
                m_cameFrom[neighbor] = current;
                m_scoreFromStart[neighbor] = neighborScoreFromStart;
                m_scorePlusHeuristicFromStart[neighbor]
                        = neighborScoreFromStart + heuristic(*neighbor, fromNode);
                if (!contains(m_openSet, neighbor))
                    m_openSet.push_back(neighbor);
            }
        }
    }

    // No path found.. return random point!
    if (!fromNode.ways().empty())
    {
        Way* randomSegment = fromNode.ways()[size_t(rand()) % fromNode.ways().size()];

        if (&randomSegment->from() == &fromNode)
            return &randomSegment->to();
        else if (&randomSegment->to() == &fromNode)
            return &randomSegment->from();
    }

    return nullptr;
}

//------------------------------------------------------------------------------
Node* Dijkstra::getPointWithLowestScorePlusHeuristicFromStart()
{
    float lowestValue = std::numeric_limits<float>::max();
    Node* lowestPoint = nullptr;

    for (auto& it: m_scorePlusHeuristicFromStart)
    {
        if (it.second < lowestValue)
        {
            lowestValue = it.second;
            lowestPoint = it.first;
        }
    }

    if (lowestPoint != nullptr)
        remove(m_scorePlusHeuristicFromStart, lowestPoint);

    return lowestPoint;
}

//------------------------------------------------------------------------------
float Dijkstra::heuristic(Node& p1, Node& p2)
{
    return squaredMagnitude(p2.position() - p1.position());
}
