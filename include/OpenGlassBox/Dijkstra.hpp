// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-OpenGlassBox-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of OpenGlassBox. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#ifndef OPEN_GLASSBOX_DIJKSTRA_HPP
#  define OPEN_GLASSBOX_DIJKSTRA_HPP

#  include "OpenGlassBox/Path.hpp"
#  include <random>
#  include <unordered_set>
#  include <map>

//! \brief One-hop dynamic router (best-first search toward the nearest Unit
//! accepting the carried resources). Recalculated at every graph node.
class Dijkstra
{
public:

    Dijkstra();

    Node* findNextPoint(Node& fromNode, std::string& searchTarget,
                        Resources& resources);

    void setRandomSeed(unsigned seed);

private:

    float heuristic(Node& p1, Node& p2);
    Node* randomNeighbor(Node& fromNode);

private:

    std::unordered_set<Node*> m_closedSet;
    std::map<Node*, Node*>    m_cameFrom;
    std::map<Node*, float>    m_scoreFromStart;
    std::mt19937              m_rng;
};

#endif
