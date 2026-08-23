//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file Dijkstra.hpp
//! \brief Routing over the road graph on travel time rather than on distance.

#ifndef OPEN_GLASSBOX_DIJKSTRA_HPP
#define OPEN_GLASSBOX_DIJKSTRA_HPP

#include "OpenGlassBox/Path.hpp"
#include <map>
#include <random>
#include <unordered_set>
#include <vector>

namespace ogb
{

class Unit;

//==============================================================================
//! \brief The answer to "where do I take this, and how do I get there".
//!
//! The destination is not asked for, it is found: the router walks the network
//! outwards until it meets a building that answers to the name looked for and
//! has room for the load. Which one that is depends on the traffic at the time
//! of the search, so two Agents leaving the same door a minute apart may well
//! be sent to different shops.
//!
//! A building may stand along a segment rather than on a crossroads, in which
//! case the last leg of the trip is a fraction of that segment: \c approachWay
//! and \c approachOffset are how the Agent knows to stop halfway down the
//! street.
//==============================================================================
struct Route
{
    //! \brief The crossroads left to drive through, the next one first. Empty
    //! when the Agent already stands at the last one, which happens when the
    //! destination is on the Node it is routed from, or on a segment incident
    //! to it.
    std::vector<Node*> nodes;

    //! \brief The segment the destination stands along, or nullptr when it
    //! stands on a crossroads.
    Way* approachWay = nullptr;

    //! \brief Where along \c approachWay the destination stands, in [0..1].
    float approachOffset = 0.0f;

    //! \brief The building the load is for, or nullptr when none was found.
    //! Cleared by Agent::forget when that building is demolished mid-trip.
    Unit* destination = nullptr;

    //! \brief Travel time of the whole trip, in seconds of game time, under the
    //! traffic as it was when the search ran.
    float cost = 0.0f;

    //! \brief Whether anything was found at all. False is an ordinary answer:
    //! it means the city has nowhere to put what the Agent carries.
    bool found = false;
};

//==============================================================================
//! \brief How Agents find somewhere to deliver. Kept as an interface so that a
//! traffic assignment solver can be dropped in without City or Agent noticing.
//==============================================================================
class IRouter
{
public:

    virtual ~IRouter() = default;

    //--------------------------------------------------------------------------
    //! \brief Find the cheapest building that accepts that load.
    //! \param[in] fromNode the crossroads to start from.
    //! \param[in] searchTarget the name looked for, matched against the
    //! \c targets of the buildings.
    //! \param[in] resources the load, tested against the room left in the
    //! candidates. Not modified.
    //! \return the itinerary. Route::found is false when nothing accepts it.
    //--------------------------------------------------------------------------
    virtual Route findRoute(Node& fromNode,
                            std::string const& searchTarget,
                            Resources const& resources) = 0;

    //--------------------------------------------------------------------------
    //! \brief The travel time of the trip findRoute() would return, without
    //! keeping the itinerary. What tells an Agent that the road it is on has
    //! become worse than the alternative.
    //! \return the travel time in seconds of game time, or infinity when
    //! nothing accepts the load.
    //--------------------------------------------------------------------------
    virtual float shortestPathCost(Node& fromNode,
                                   std::string const& searchTarget,
                                   Resources const& resources) = 0;

    //--------------------------------------------------------------------------
    //! \brief The next crossroads to drive to, without keeping an itinerary.
    //! \return the next Node of the trip, \c fromNode itself when the
    //! destination is right there, or a random neighbour when nothing was
    //! found: an Agent with nowhere to go wanders rather than stops.
    //--------------------------------------------------------------------------
    virtual Node* findNextPoint(Node& fromNode,
                                std::string& searchTarget,
                                Resources& resources) = 0;

    //--------------------------------------------------------------------------
    //! \brief Seed the generator behind the wandering, so that a run can be
    //! reproduced. Does nothing on a router that never guesses.
    //--------------------------------------------------------------------------
    virtual void setRandomSeed(unsigned seed)
    {
        (void)seed;
    }
};

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
//! Having no goal also means there is nothing for an A* estimate to aim at.
//! What is added to the cost of a node is the free flow travel time back to the
//! Node the search started from, which biases the order of exploration towards
//! the neighbourhood of the departure, where the nearest destination usually
//! is. It is a speed-up, not an admissible heuristic: the answer is the
//! cheapest building the search met first, which in an unusual geometry may not
//! be the cheapest one there is. The AStarRouter alias is a leftover of the
//! days when the search did have a goal.
//!
//! The search never leaves the Path it started on, so a road network and a rail
//! network cannot be mixed in one trip.
//!
//! Example:
//! \code
//! Resources load;
//! load.addResource("Goods", 1u);
//!
//! Route const route = city.router().findRoute(*factory.accessNode(),
//!                                            "Shop", load);
//! if (!route.found)
//!     std::cout << "no shop has room for it\n";
//! else
//!     std::cout << route.destination->type() << " in "
//!               << route.cost << "s through " << route.nodes.size()
//!               << " crossroads\n";
//! \endcode
//==============================================================================
class Dijkstra: public IRouter
{
public:

    //--------------------------------------------------------------------------
    //! \brief Seeds the wandering from the system entropy. The City that owns
    //! the router reseeds it from SimulationConfig::randomSeed as soon as it is
    //! founded, which is what makes a run reproducible.
    //--------------------------------------------------------------------------
    Dijkstra();

    //! \copydoc IRouter::findRoute
    Route findRoute(Node& fromNode,
                    std::string const& searchTarget,
                    Resources const& resources) override;

    //! \copydoc IRouter::findNextPoint
    Node* findNextPoint(Node& fromNode,
                        std::string& searchTarget,
                        Resources& resources) override;

    //! \copydoc IRouter::shortestPathCost
    float shortestPathCost(Node& fromNode,
                           std::string const& searchTarget,
                           Resources const& resources) override;

    //! \copydoc IRouter::setRandomSeed
    void setRandomSeed(unsigned seed) override;

    //--------------------------------------------------------------------------
    //! \brief One of the crossroads next door, drawn at random.
    //!
    //! Where an Agent that found nothing goes: it takes a street at random
    //! rather than standing still, and looks again from there. Two game hours
    //! of that and it gives up, see SimulationConfig::agentGiveUpTicks.
    //!
    //! \param[in] fromNode the crossroads to leave.
    //! \return a neighbour, or nullptr when no road leads anywhere from there.
    //--------------------------------------------------------------------------
    Node* randomNeighbor(Node& fromNode);

private:

    //--------------------------------------------------------------------------
    //! \brief What is added to the cost of a node to order the exploration: the
    //! free flow travel time between the two Nodes. See the note on the class.
    //! \param[in] p1, p2 the two crossroads.
    //! \param[in] maxFreeFlowSpeed the speed of the fastest segment type of the
    //! network, so that the estimate cannot overtake the real cost of a hop.
    //--------------------------------------------------------------------------
    float
    heuristic(Node const& p1, Node const& p2, float maxFreeFlowSpeed) const;

    //--------------------------------------------------------------------------
    //! \brief Walk the trail of visited nodes back to the start and turn it
    //! into an itinerary in the driving order.
    //! \param[in] fromNode where the search started.
    //! \param[in] goalNode the last crossroads of the trip.
    //! \param[in] destination the building found.
    //! \param[in] approachWay the segment it stands along, or nullptr.
    //! \param[in] approachOffset where along that segment, in [0..1].
    //! \param[in] cost travel time of the whole trip.
    //--------------------------------------------------------------------------
    Route reconstruct(Node const& fromNode,
                      Node* goalNode,
                      Unit* destination,
                      Way* approachWay,
                      float approachOffset,
                      float cost) const;

    //! \brief Nodes whose cheapest cost is settled. Kept as members rather than
    //! as locals: a city routes thousands of trips per second, and these three
    //! containers would otherwise be allocated and freed for every one of them.
    std::unordered_set<Node*> m_closedSet;

    //! \brief For each node reached, the node it was reached from. The trail
    //! reconstruct() walks back.
    std::map<Node*, Node*> m_cameFrom;

    //! \brief For each node reached, the travel time to get there.
    std::map<Node*, float> m_scoreFromStart;

    //! \brief Behind randomNeighbor(). Seeded by setRandomSeed().
    std::mt19937 m_rng;
};

//! \brief Kept for the callers written when the search had a goal to aim at.
using AStarRouter = Dijkstra;

} // namespace ogb

#endif
