//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file Router.hpp
//! \brief Routing over the road graph on travel time rather than on distance.

#ifndef OPEN_GLASSBOX_ROUTER_HPP
#define OPEN_GLASSBOX_ROUTER_HPP

#include "OpenGlassBox/Path.hpp"
#include <string>
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
    //! \brief The crossroads of the trip, in the order they are driven through.
    //! The ones already behind the Agent are still in there: \c visited is
    //! where it has got to. Read it through waypointsLeft() and nextWaypoint()
    //! rather than directly.
    //!
    //! Empty when the Agent already stands at the last one, which happens when
    //! the destination is on the Node it is routed from, or on a segment
    //! incident to it.
    std::vector<Node*> nodes;

    //! \brief How many leading entries of \c nodes have been driven through.
    //! Advancing an index rather than removing the front entry: an Agent walks
    //! its itinerary one crossroads per tick, and shifting the whole vector
    //! down each time makes a long trip quadratic.
    size_t visited = 0u;

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

    //--------------------------------------------------------------------------
    //! \brief \return whether crossroads remain to be driven through. False
    //! means the Agent is on the last leg, which is either the door itself or
    //! the stretch of \c approachWay leading to it.
    //--------------------------------------------------------------------------
    bool waypointsLeft() const
    {
        return visited < nodes.size();
    }

    //--------------------------------------------------------------------------
    //! \brief \return the next crossroads to drive to. Undefined when
    //! waypointsLeft() is false.
    //--------------------------------------------------------------------------
    Node* nextWaypoint() const
    {
        return nodes[visited];
    }

    //--------------------------------------------------------------------------
    //! \brief Mark the next crossroads as driven through.
    //--------------------------------------------------------------------------
    void takeWaypoint()
    {
        ++visited;
    }

    //--------------------------------------------------------------------------
    //! \brief \return an iterator on the first crossroads still to come, for
    //! walking what is left of the trip.
    //--------------------------------------------------------------------------
    std::vector<Node*>::const_iterator begin() const
    {
        return nodes.begin() + std::vector<Node*>::difference_type(visited);
    }

    //--------------------------------------------------------------------------
    //! \brief \return the end of what is left of the trip.
    //--------------------------------------------------------------------------
    std::vector<Node*>::const_iterator end() const
    {
        return nodes.end();
    }

    //--------------------------------------------------------------------------
    //! \brief \return how many crossroads are still to come.
    //--------------------------------------------------------------------------
    size_t waypointCount() const
    {
        return nodes.size() - visited;
    }
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
                            Name const& searchTarget,
                            Resources const& resources) = 0;

    //--------------------------------------------------------------------------
    //! \brief The travel time of the trip findRoute() would return, without
    //! keeping the itinerary. What tells an Agent that the road it is on has
    //! become worse than the alternative.
    //! \param[in] fromNode the crossroads to start from.
    //! \param[in] searchTarget the name looked for, matched against the
    //! \c targets of the buildings.
    //! \param[in] resources the load, tested against the room left in the
    //! candidates. Not modified.
    //! \return the travel time in seconds of game time, or infinity when
    //! nothing accepts the load.
    //--------------------------------------------------------------------------
    virtual float shortestPathCost(Node& fromNode,
                                   Name const& searchTarget,
                                   Resources const& resources) = 0;

    //--------------------------------------------------------------------------
    //! \brief The next crossroads to drive to, without keeping an itinerary.
    //! \param[in] fromNode the crossroads to start from.
    //! \param[in,out] searchTarget the name looked for. Unchanged by the
    //! default router.
    //! \param[in,out] resources the load. Unchanged by the default router.
    //! \return the next Node of the trip, \c fromNode itself when the
    //! destination is right there, or a random neighbour when nothing was
    //! found: an Agent with nowhere to go wanders rather than stops.
    //--------------------------------------------------------------------------
    virtual Node* findNextPoint(Node& fromNode,
                                Name& searchTarget,
                                Resources& resources) = 0;

    //--------------------------------------------------------------------------
    //! \brief Seed the generator behind the wandering, so that a run can be
    //! reproduced. Does nothing on a router that never guesses.
    //! \param[in] seed the value to seed the generator with.
    //--------------------------------------------------------------------------
    virtual void setRandomSeed(unsigned seed)
    {
        (void)seed;
    }
};

} // namespace ogb

#endif
