//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#ifndef OPEN_GLASSBOX_DSTAR_HPP
#  define OPEN_GLASSBOX_DSTAR_HPP

#  include "OpenGlassBox/Path.hpp"
#  include <unordered_set>
#  include <vector>
#  include <map>

//==============================================================================
//! \brief
//==============================================================================
class Dijkstra
{
public:

    Node* findNextPoint(Node& fromNode, std::string & searchTarget,
                        Resources & resources);

private:

    Node* getPointWithLowestScorePlusHeuristicFromStart();
    float heuristic(Node & p1, Node & p2);

private:

    std::unordered_set<Node*> m_closedSet;
    std::vector<Node*>        m_openSet;
    std::map<Node*, Node*>    m_cameFrom;
    std::map<Node*, float>    m_scoreFromStart;
    std::map<Node*, float>    m_scorePlusHeuristicFromStart;
};

#endif
