//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file DijkstraRouter.hpp
//! \brief Default best-first router on the road graph.

#ifndef OPEN_GLASSBOX_DIJKSTRA_ROUTER_HPP
#define OPEN_GLASSBOX_DIJKSTRA_ROUTER_HPP

#include "OpenGlassBox/City.hpp"

#include <random>

namespace ogb
{

class Simulation;

//==============================================================================
//! \brief Search from a crossroads for the nearest building that accepts a
//! load. "Nearest" means lowest travel time.
//!
//! Two things make this more than a basic shortest path:
//!
//! - segment cost is travel time under current traffic, not length. See
//!   Segment::travelTime and its BPR function: a short busy road can cost more
//!   than a long empty one;
//!
//! - there is no fixed goal. The search stops at the first building that
//!   matches the name and has space. A building on a segment is only a
//!   candidate: the search keeps going because a nearby crossroads may have a
//!   cheaper one.
//!
//! With no goal, this uses Dijkstra, not A*. A* needs a target to estimate
//! distance. The search stops when every reachable crossroads costs more than
//! the best building found. That is optimal without a target.
//!
//! The search stays on the Path it started on. Road and rail cannot mix in one
//! trip. A crossroads with no Path belongs to no network and routes nowhere.
//!
//! Search data lives in arrays indexed by Node::index(). Arrays are not
//! cleared between searches: a stamp marks which search wrote each entry. After
//! the first run, the router allocates nothing. A busy city runs thousands of
//! searches per second.
//==============================================================================
class Dijkstra: public IRouter
{
public:

    // -------------------------------------------------------------------------
    //! \brief Build a router with a random seed for wandering.
    // -------------------------------------------------------------------------
    Dijkstra();

    //! \copydoc IRouter::findRoute
    [[nodiscard]] Route findRoute(Node& fromNode,
                    Name const& searchTarget,
                    Resources const& resources) override;

    //! \copydoc IRouter::findNextNode
    [[nodiscard]] Node* findNextNode(Node& fromNode,
                       Name& searchTarget,
                       Resources& resources) override;

    //! \copydoc IRouter::computeShortestPathCost
    [[nodiscard]] float computeShortestPathCost(Node& fromNode,
                                  Name const& searchTarget,
                                  Resources const& resources) override;

    //! \copydoc IRouter::setRandomSeed
    void setRandomSeed(unsigned seed) override;

    // -------------------------------------------------------------------------
    //! \brief Pick one neighbour crossroads at random.
    //!
    //! Used when an agent found no destination: it moves to a random street
    //! and searches again.
    //! \param[in] fromNode the crossroads to leave.
    //! \return a neighbour, or nullptr when no road leads out.
    // -------------------------------------------------------------------------
    [[nodiscard]] Node* findRandomNeighbor(Node& fromNode);

private:

    //==========================================================================
    //! \brief One crossroads waiting to expand, ordered by \c f.
    //==========================================================================
    struct QueueEntry
    {
        //! \brief Score for heap order. Equal to \c g in plain Dijkstra. Kept
        //! separate so a goal-based router can add an estimate here.
        float f;
        //! \brief Travel time from the start to \c node.
        float g;
        //! \brief Crossroads for this entry.
        Node* node;

        //! \brief Order the heap smallest first. On a tie in \c f, prefer the
        //! cheaper \c g.
        //!
        //! \note Uses two comparisons, not equality on \c f. The heap needs a
        //! strict weak ordering.
        bool operator>(QueueEntry const& other) const
        {
            if (f > other.f)
                return true;
            if (other.f > f)
                return false;
            return g > other.g;
        }
    };

    // -------------------------------------------------------------------------
    //! \brief Build a Route from the predecessor map made by findRoute().
    //! \param[in] fromNode the crossroads where the search started.
    //! \param[in] goalNode the last crossroads reached, or nullptr when the
    //! destination is on \p fromNode.
    //! \param[in] destination the building for the load.
    //! \param[in] approachSegment the segment the destination sits on, or
    //! nullptr at a crossroads.
    //! \param[in] approachOffset position on \p approachSegment, in [0..1].
    //! \param[in] cost total travel time in seconds of game time.
    //! \return the route for the agent.
    // -------------------------------------------------------------------------
    Route reconstruct(Node const& fromNode,
                      Node* goalNode,
                      Building* destination,
                      Segment* approachSegment,
                      float approachOffset,
                      float cost);

    // -------------------------------------------------------------------------
    //! \brief Start a new search: grow arrays to fit the network and bump the
    //! stamp so old entries look unused.
    //! \param[in] nodeCount number of crossroads in the network.
    // -------------------------------------------------------------------------
    void beginSearch(size_t nodeCount);

    // -------------------------------------------------------------------------
    //! \param[in] index crossroads index from Node::index().
    //! \return true when the current search already reached it.
    // -------------------------------------------------------------------------
    [[nodiscard]] bool isVisited(uint32_t const index) const
    {
        return m_stamp[index] == m_generation;
    }

    // -------------------------------------------------------------------------
    //! \brief Record a cheaper way to reach a crossroads.
    //!
    //! Also clears its closed flag. This rarely matters in practice, but keeps
    //! the arrays consistent.
    //! \param[in] index crossroads index.
    //! \param[in] score travel time from the search start.
    //! \param[in] from the crossroads reached from, or nullptr for the start.
    // -------------------------------------------------------------------------
    void setScore(uint32_t const index, float const score, Node* const from)
    {
        m_stamp[index] = m_generation;
        m_scoreFromStart[index] = score;
        m_cameFrom[index] = from;
        m_closed[index] = 0u;
    }

    //! \brief Which search last wrote each entry, indexed by Node::index().
    //! Entries without m_generation are treated as missing.
    std::vector<uint32_t> m_stamp;

    //! \brief Cheapest travel time from the start to each node so far.
    std::vector<float> m_scoreFromStart;

    //! \brief Predecessor of each crossroads on the cheapest path so far.
    std::vector<Node*> m_cameFrom;

    //! \brief Whether each crossroads was already expanded.
    std::vector<uint8_t> m_closed;

    //! \brief Current search id stored in m_stamp. Never zero, so new arrays
    //! filled with zero look unused.
    uint32_t m_generation = 0u;

    //! \brief Crossroads waiting to expand, kept as a heap.
    std::vector<QueueEntry> m_open;

    //! \brief Scratch buffer to build a route from predecessors. Built in
    //! reverse order.
    std::vector<Node*> m_reverse;

    //! \brief Random source for wandering when no destination is found. Not
    //! for security. Use setRandomSeed() for reproducible runs.
    std::mt19937 m_rng; // NOSONAR cpp:S2245
};

// -------------------------------------------------------------------------
//! \brief Install the default Dijkstra router on a city.
//! \param[in,out] city receives ownership of the router.
//! \param[in] config read for Config::randomSeed.
// -------------------------------------------------------------------------
inline void installDijkstraRouter(City& city, Config const& config)
{
    auto router = std::make_unique<Dijkstra>();
    if (config.randomSeed != 0u)
        router->setRandomSeed(config.randomSeed);
    city.setRouter(std::move(router));
}

// -------------------------------------------------------------------------
//! \brief Install the default Dijkstra router on every city of a simulation.
//!
//! A city loaded from a save has no router, and an Agent without one never
//! moves. Call this once every city is loaded. An application that reacts to
//! SimulationListener::onCityAdded does not need it.
//! \param[in,out] simulation the simulation whose cities receive a router.
// -------------------------------------------------------------------------
void installDijkstraRouters(Simulation const& simulation);

} // namespace ogb

#endif
