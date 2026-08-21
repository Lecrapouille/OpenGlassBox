//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#ifndef OPEN_GLASSBOX_DIJKSTRA_HPP
#  define OPEN_GLASSBOX_DIJKSTRA_HPP

#  include "OpenGlassBox/Path.hpp"
#  include <random>
#  include <unordered_set>
#  include <map>
#  include <vector>

class Unit;

//==============================================================================
//! \brief An itinerary from a Node to a Unit that accepts the carried
//! resources. The path is the sequence of Nodes to visit; when the destination
//! sits on a Way rather than on a Node, the last hop is that Way at a given
//! offset.
//==============================================================================
struct Route
{
    //! \brief Remaining Nodes, next one first. Empty when the Agent is already
    //! at the destination Node, or when the destination is on an incident Way.
    std::vector<Node*> nodes;
    //! \brief Way the destination sits on, or nullptr when it sits on a Node.
    Way* approachWay = nullptr;
    //! \brief Offset along approachWay of the destination Unit.
    float approachOffset = 0.0f;
    //! \brief The Unit that accepted the resources, or nullptr on failure.
    Unit* destination = nullptr;
    //! \brief Travel time of the whole itinerary under the current traffic.
    float cost = 0.0f;
    bool found = false;
};

//==============================================================================
//! \brief Best-first search toward the nearest Unit accepting the carried
//! resources, with a travel-time cost (BPR) rather than a length.
//==============================================================================
class Dijkstra
{
public:

    Dijkstra();

    //--------------------------------------------------------------------------
    //! \brief Compute a complete itinerary from \c fromNode. Returns an empty
    //! route (found=false) when nothing accepts the resources; the caller then
    //! typically wanders to a random neighbour.
    //--------------------------------------------------------------------------
    Route findRoute(Node& fromNode, std::string const& searchTarget,
                    Resources const& resources);

    //--------------------------------------------------------------------------
    //! \brief First Node of findRoute, kept for the tests that check one-hop
    //! decisions. On failure, a random neighbour.
    //--------------------------------------------------------------------------
    Node* findNextPoint(Node& fromNode, std::string& searchTarget,
                        Resources& resources);

    //--------------------------------------------------------------------------
    //! \brief Travel time of the cheapest itinerary, or a large value when
    //! none exists. Used by the relative gap diagnostic.
    //--------------------------------------------------------------------------
    float shortestPathCost(Node& fromNode, std::string const& searchTarget,
                           Resources const& resources);

    void setRandomSeed(unsigned seed);

    Node* randomNeighbor(Node& fromNode);

private:

    float heuristic(Node& p1, Node& p2, float maxFreeFlowSpeed) const;
    Route reconstruct(Node& fromNode, Node* goalNode, Unit* destination,
                      Way* approachWay, float approachOffset, float cost) const;

private:

    std::unordered_set<Node*> m_closedSet;
    std::map<Node*, Node*>    m_cameFrom;
    std::map<Node*, float>    m_scoreFromStart;
    std::mt19937              m_rng;
};

#endif
