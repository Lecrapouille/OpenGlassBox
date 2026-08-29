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

namespace ogb
{

class Unit;

//==============================================================================
//! \brief The answer to "where do I take this, and how do I get there".
//!
//! The destination is not asked for, it is found: the router walks the network
//! outwards until it meets a building that answers to the name looked for and
//! has room for the load. Which one that is depends on the traffic at the time
//! of the search, so two agents leaving the same door a minute apart may well
//! be sent to different shops.
//!
//! A building may stand along a segment rather than on a crossroads, in which
//! case the last leg of the trip is a fraction of that segment. The approach
//! segment and the approach offset are how the agent knows to stop halfway down
//! the street.
//==============================================================================
class Route
{
public:

    //--------------------------------------------------------------------------
    //! \brief Record what the search found. Called by a router.
    //! \param[in] destination the building the load is for.
    //! \param[in] approachSegment the segment it stands along, or nullptr when
    //! it stands on a crossroads.
    //! \param[in] approachOffset where along that segment it stands, in [0..1].
    //! \param[in] cost travel time of the whole trip, in seconds of game time.
    //--------------------------------------------------------------------------
    void setTarget(Unit* destination,
                   Segment* approachSegment,
                   float approachOffset,
                   float cost)
    {
        m_destination = destination;
        m_approachSegment = approachSegment;
        m_approachOffset = approachOffset;
        m_cost = cost;
        m_found = true;
    }

    //--------------------------------------------------------------------------
    //! \brief Record the crossroads to drive through, in order. Called by a
    //! router once it has walked its search tree back to the start.
    //! \param[in] first, last the crossroads, from the next one to the last.
    //--------------------------------------------------------------------------
    template <class Iterator>
    void setWaypoints(Iterator first, Iterator last)
    {
        m_nodes.assign(first, last);
        m_visited = 0u;
    }

    //--------------------------------------------------------------------------
    //! \brief Forget the building the load was for, the building having been
    //! demolished. The itinerary itself is left alone: the agent drives on and
    //! looks again.
    //--------------------------------------------------------------------------
    void clearDestination()
    {
        m_destination = nullptr;
    }

    //--------------------------------------------------------------------------
    //! \brief \return whether anything was found at all. False is an ordinary
    //! answer: it means the city has nowhere to put what the agent carries.
    //--------------------------------------------------------------------------
    [[nodiscard]] bool isFound() const
    {
        return m_found;
    }

    //--------------------------------------------------------------------------
    //! \brief \return the building the load is for, or nullptr when none was
    //! found or when it was demolished mid-trip.
    //--------------------------------------------------------------------------
    [[nodiscard]] Unit* getDestination() const
    {
        return m_destination;
    }

    //--------------------------------------------------------------------------
    //! \brief \return the segment the destination stands along, or nullptr when
    //! it stands on a crossroads.
    //--------------------------------------------------------------------------
    [[nodiscard]] Segment* getApproachSegment() const
    {
        return m_approachSegment;
    }

    //--------------------------------------------------------------------------
    //! \brief \return where along the approach segment the destination stands,
    //! in [0..1].
    //--------------------------------------------------------------------------
    [[nodiscard]] float getApproachOffset() const
    {
        return m_approachOffset;
    }

    //--------------------------------------------------------------------------
    //! \brief \return the travel time of the whole trip, in seconds of game
    //! time, under the traffic as it was when the search ran.
    //--------------------------------------------------------------------------
    [[nodiscard]] float getCost() const
    {
        return m_cost;
    }

    //--------------------------------------------------------------------------
    //! \brief \return whether crossroads remain to be driven through. False
    //! means the agent is on the last leg, which is either the door itself or
    //! the stretch of the approach segment leading to it.
    //--------------------------------------------------------------------------
    [[nodiscard]] bool hasWaypointsLeft() const
    {
        return m_visited < m_nodes.size();
    }

    //--------------------------------------------------------------------------
    //! \brief \return the next crossroads to drive to. Undefined when
    //! hasWaypointsLeft() is false.
    //--------------------------------------------------------------------------
    [[nodiscard]] Node* getNextWaypoint() const
    {
        return m_nodes[m_visited];
    }

    //--------------------------------------------------------------------------
    //! \brief Mark the next crossroads as driven through.
    //--------------------------------------------------------------------------
    void takeWaypoint()
    {
        ++m_visited;
    }

    //--------------------------------------------------------------------------
    //! \brief \return an iterator on the first crossroads still to come, for
    //! walking what is left of the trip.
    //--------------------------------------------------------------------------
    std::vector<Node*>::const_iterator begin() const
    {
        return m_nodes.begin() + std::vector<Node*>::difference_type(m_visited);
    }

    //--------------------------------------------------------------------------
    //! \brief \return the end of what is left of the trip.
    //--------------------------------------------------------------------------
    std::vector<Node*>::const_iterator end() const
    {
        return m_nodes.end();
    }

    //--------------------------------------------------------------------------
    //! \brief \return how many crossroads are still to come.
    //--------------------------------------------------------------------------
    [[nodiscard]] size_t getWaypointCount() const
    {
        return m_nodes.size() - m_visited;
    }

private:

    //! \brief The crossroads of the trip, in the order they are driven through.
    //! The ones already behind the agent are still in there.
    //!
    //! Empty when the agent already stands at the last one, which happens when
    //! the destination is on the node it is routed from, or on a segment
    //! touching it.
    std::vector<Node*> m_nodes;

    //! \brief How many leading entries of m_nodes have been driven through.
    //! An index rather than removing the front entry: an agent walks its
    //! itinerary one crossroads per tick, and shifting the whole vector down
    //! each time makes a long trip quadratic.
    size_t m_visited = 0u;

    //! \brief The segment the destination stands along, or nullptr when it
    //! stands on a crossroads.
    Segment* m_approachSegment = nullptr;

    //! \brief Where along m_approachSegment the destination stands, in [0..1].
    float m_approachOffset = 0.0f;

    //! \brief The building the load is for, or nullptr when none was found.
    Unit* m_destination = nullptr;

    //! \brief Travel time of the whole trip, in seconds of game time, under the
    //! traffic as it was when the search ran.
    float m_cost = 0.0f;

    //! \brief Whether anything was found at all.
    bool m_found = false;
};

//==============================================================================
//! \brief How agents find somewhere to deliver. An interface, so a traffic
//! assignment solver can be dropped in without City or Agent noticing.
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
    //! \return the itinerary. Route::isFound() is false when nothing accepts
    //! it.
    //--------------------------------------------------------------------------
    [[nodiscard]] virtual Route findRoute(Node& fromNode,
                                          Name const& searchTarget,
                                          Resources const& resources) = 0;

    //--------------------------------------------------------------------------
    //! \brief The travel time of the trip findRoute() would return, without
    //! keeping the itinerary. This is what tells an agent that the road it is
    //! on has become worse than the alternative.
    //! \param[in] fromNode the crossroads to start from.
    //! \param[in] searchTarget the name looked for.
    //! \param[in] resources the load, tested against the room left in the
    //! candidates. Not modified.
    //! \return the travel time in seconds of game time, or ROUTING_INFINITY
    //! when nothing accepts the load.
    //--------------------------------------------------------------------------
    [[nodiscard]] virtual float
    computeShortestPathCost(Node& fromNode,
                            Name const& searchTarget,
                            Resources const& resources) = 0;

    //--------------------------------------------------------------------------
    //! \brief The next crossroads to drive to, without keeping an itinerary.
    //! \param[in] fromNode the crossroads to start from.
    //! \param[in,out] searchTarget the name looked for. Unchanged by the
    //! default router.
    //! \param[in,out] resources the load. Unchanged by the default router.
    //! \return the next node of the trip, \p fromNode itself when the
    //! destination is right there, or a random neighbour when nothing was
    //! found: an agent with nowhere to go wanders rather than stops.
    //--------------------------------------------------------------------------
    virtual Node*
    findNextNode(Node& fromNode, Name& searchTarget, Resources& resources) = 0;

    //--------------------------------------------------------------------------
    //! \brief Seed the generator behind the wandering, so a run can be
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
