//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file DijkstraRouter.hpp
//! \brief Default best-first router over the road graph.

#ifndef OPEN_GLASSBOX_DEMO_DIJKSTRA_ROUTER_HPP
#define OPEN_GLASSBOX_DEMO_DIJKSTRA_ROUTER_HPP

#include "OpenGlassBox/Router.hpp"

#include <random>
#include <vector>

namespace ogb
{

//==============================================================================
//! \brief Best-first search outwards from a crossroads, looking for the nearest
//! building that accepts a load, where "nearest" is counted in travel time.
//!
//! Two things make this more than a textbook shortest path:
//!
//! - the cost of a segment is its travel time under the current traffic, not
//!   its length. See Way::travelTime and the BPR function it applies: a short
//!   road carrying two hundred Agents costs more than a long empty one, which
//!   is what makes a city spread its traffic instead of queueing everything
//!   through the same street;
//!
//! - there is no goal to aim at. The search does not know where it is going, it
//!   stops at the first building it meets that answers to the name and has
//!   room. A building standing along a segment is a candidate rather than a
//!   winner: the search keeps it and carries on, because a crossroads one hop
//!   away may hold a cheaper one.
//!
//! Having no goal is also why this is a Dijkstra and not an A*: an estimate
//! needs a target to estimate the distance to. Stopping as soon as every
//! crossroads still reachable is dearer than the best building already in hand
//! is the best that can be done without one, and it is optimal: the search
//! expands exactly the ball of radius the cost of its own answer.
//!
//! The search never leaves the Path it started on, so a road network and a rail
//! network cannot be mixed in one trip. A crossroads belonging to no Path at
//! all is not part of any network and routes nowhere.
//!
//! The bookkeeping of a search lives in plain arrays indexed by Node::index()
//! rather than in trees or hash tables keyed by address, and those arrays are
//! never cleared between two searches: a stamp says which search wrote an
//! entry. A router that has run once therefore allocates nothing at all, which
//! matters because a busy city runs thousands of these per second.
//==============================================================================
class Dijkstra: public IRouter
{
public:

    // -------------------------------------------------------------------------
    //! \brief Build a router with a non-deterministic seed for wandering.
    // -------------------------------------------------------------------------
    Dijkstra();

    //! \copydoc IRouter::findRoute
    Route findRoute(Node& fromNode,
                    Name const& searchTarget,
                    Resources const& resources) override;

    //! \copydoc IRouter::findNextPoint
    Node* findNextPoint(Node& fromNode,
                        Name& searchTarget,
                        Resources& resources) override;

    //! \copydoc IRouter::shortestPathCost
    float shortestPathCost(Node& fromNode,
                           Name const& searchTarget,
                           Resources const& resources) override;

    //! \copydoc IRouter::setRandomSeed
    void setRandomSeed(unsigned seed) override;

    // -------------------------------------------------------------------------
    //! \brief One of the crossroads next door, drawn at random.
    //!
    //! Where an Agent that found nothing goes: it takes a street at random
    //! rather than standing still, and looks again from there.
    //! \param[in] fromNode the crossroads to leave.
    //! \return a neighbour, or nullptr when no road leads anywhere from there.
    // -------------------------------------------------------------------------
    Node* randomNeighbor(Node& fromNode);

private:

    //==========================================================================
    //! \brief One crossroads waiting to be expanded, ordered by \c f.
    //==========================================================================
    struct QueueEntry
    {
        //! \brief Score the heap is ordered by. Equal to \c g in a plain
        //! Dijkstra; the two are kept apart as the seam where a router with a
        //! goal to aim at would add its estimate.
        float f;
        //! \brief Travel time from the start to \c node.
        float g;
        //! \brief Crossroads this entry refers to.
        Node* node;

        //! \brief Orders the heap smallest first, breaking a tie on \c f by
        //! preferring the entry that is cheaper to reach.
        //!
        //! \note Written as two comparisons rather than a test for equality on
        //! \c f: the heap needs a strict weak ordering, and letting scores
        //! within some tolerance count as equal would not be transitive.
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
    //! \brief Turn the predecessor map built by findRoute() into a Route.
    //! \param[in] fromNode the crossroads the search started from.
    //! \param[in] goalNode the last crossroads reached, or nullptr when the
    //! destination stands on \p fromNode.
    //! \param[in] destination the building the load is for.
    //! \param[in] approachWay the segment the destination stands along, or
    //! nullptr when it stands on a crossroads.
    //! \param[in] approachOffset where along \p approachWay the destination
    //! stands, in [0..1].
    //! \param[in] cost travel time of the whole trip in seconds of game time.
    //! \return the itinerary handed to the Agent.
    // -------------------------------------------------------------------------
    Route reconstruct(Node const& fromNode,
                      Node* goalNode,
                      Unit* destination,
                      Way* approachWay,
                      float approachOffset,
                      float cost);

    // -------------------------------------------------------------------------
    //! \brief Open a new search: grow the arrays to the network and move the
    //! stamp on, which is what makes every entry stale in one step.
    //! \param[in] nodeCount how many crossroads the network has.
    // -------------------------------------------------------------------------
    void beginSearch(size_t nodeCount);

    // -------------------------------------------------------------------------
    //! \brief \param[in] index rank of a crossroads, from Node::index().
    //! \return whether the current search has already reached it.
    // -------------------------------------------------------------------------
    bool visited(uint32_t const index) const
    {
        return m_stamp[index] == m_generation;
    }

    // -------------------------------------------------------------------------
    //! \brief Record a cheaper way of reaching a crossroads.
    //!
    //! Also clears its closed flag. That never fires in practice, an expanded
    //! crossroads being unimprovable once the queue is ordered by cost alone,
    //! but it keeps the two arrays consistent for anyone reading them.
    //! \param[in] index rank of the crossroads.
    //! \param[in] score travel time from the start of the search.
    //! \param[in] from the crossroads it is reached from, or nullptr for the
    //! one the search started at.
    // -------------------------------------------------------------------------
    void setScore(uint32_t const index, float const score, Node* const from)
    {
        m_stamp[index] = m_generation;
        m_scoreFromStart[index] = score;
        m_cameFrom[index] = from;
        m_closed[index] = 0u;
    }

    //! \brief Which search last wrote each entry below, indexed by
    //! Node::index(). Anything not stamped with m_generation is left over from
    //! an earlier search and reads as absent.
    std::vector<uint32_t> m_stamp;

    //! \brief Cheapest travel time found so far from the start to each Node.
    std::vector<float> m_scoreFromStart;

    //! \brief Predecessor of each crossroads on the cheapest path found so far.
    std::vector<Node*> m_cameFrom;

    //! \brief Whether each crossroads has been expanded already.
    std::vector<uint8_t> m_closed;

    //! \brief Which search is running, and the value m_stamp entries carry.
    //! Never zero, so that the zeros a freshly grown m_stamp is filled with
    //! read as absent.
    uint32_t m_generation = 0u;

    //! \brief The crossroads waiting to be expanded, kept as a heap.
    std::vector<QueueEntry> m_open;

    //! \brief Scratch used to turn the predecessors into an itinerary, which
    //! comes out backwards.
    std::vector<Node*> m_reverse;

    //! \brief Picks a random neighbour when routing finds no destination. Not
    //! used for security; reproducibility goes through setRandomSeed().
    std::mt19937 m_rng; // NOSONAR cpp:S2245
};

} // namespace ogb

#endif
