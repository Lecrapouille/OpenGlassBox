//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file Router.hpp
//! \brief Find routes on the road graph using travel time, not distance.

#ifndef OPEN_GLASSBOX_ROUTER_HPP
#define OPEN_GLASSBOX_ROUTER_HPP

#include "OpenGlassBox/Path.hpp"

namespace ogb
{

class Building;

//==============================================================================
//! \brief Result of a route search: where to go and how to get there.
//!
//! The router finds the destination. It searches the network for a building
//! that matches the name and has space for the load. Traffic can change the
//! answer, so two agents from the same place may go to different buildings.
//!
//! A building may sit on a road segment, not only at a crossroads. In that
//! case the last part of the trip is part of a segment. The approach segment
//! and offset tell the agent where to stop on the street.
//==============================================================================
class Route
{
public:

    //--------------------------------------------------------------------------
    //! \brief Store what the search found. Called by a router.
    //! \param[in] destination the building for the load.
    //! \param[in] approachSegment the segment it sits on, or nullptr at a
    //! crossroads.
    //! \param[in] approachOffset position on that segment, in [0..1].
    //! \param[in] cost total travel time in seconds of game time.
    //--------------------------------------------------------------------------
    void setTarget(Building* destination,
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
    //! \brief Store the crossroads to pass through, in order. Called by a
    //! router after it walks back from the search tree to the start.
    //! \param[in] first, last the crossroads, from the next one to the last.
    //--------------------------------------------------------------------------
    template <class Iterator>
    void setWaypoints(Iterator first, Iterator last)
    {
        m_nodes.assign(first, last);
        m_visited = 0u;
    }

    //--------------------------------------------------------------------------
    //! \brief Clear the destination after the building was removed. The route
    //! stays: the agent keeps driving and searches again.
    //--------------------------------------------------------------------------
    void clearDestination()
    {
        m_destination = nullptr;
    }

    //--------------------------------------------------------------------------
    //! \return true when a route was found. False means the city has no
    //! place for the load.
    //--------------------------------------------------------------------------
    [[nodiscard]] bool isFound() const
    {
        return m_found;
    }

    //--------------------------------------------------------------------------
    //! \return the building for the load, or nullptr when none was
    //! found or when it was removed during the trip.
    //--------------------------------------------------------------------------
    [[nodiscard]] Building* getDestination() const
    {
        return m_destination;
    }

    //--------------------------------------------------------------------------
    //! \return the segment the destination sits on, or nullptr when it
    //! sits at a crossroads.
    //--------------------------------------------------------------------------
    [[nodiscard]] Segment* getApproachSegment() const
    {
        return m_approachSegment;
    }

    //--------------------------------------------------------------------------
    //! \return position on the approach segment, in [0..1].
    //--------------------------------------------------------------------------
    [[nodiscard]] float getApproachOffset() const
    {
        return m_approachOffset;
    }

    //--------------------------------------------------------------------------
    //! \return total travel time in seconds of game time, using traffic
    //! at search time.
    //--------------------------------------------------------------------------
    [[nodiscard]] float getCost() const
    {
        return m_cost;
    }

    //--------------------------------------------------------------------------
    //! \return true when crossroads remain. False means the agent is on
    //! the last leg: the door or the approach segment.
    //--------------------------------------------------------------------------
    [[nodiscard]] bool hasWaypointsLeft() const
    {
        return m_visited < m_nodes.size();
    }

    //--------------------------------------------------------------------------
    //! \return the next crossroads to drive to. Do not call when
    //! hasWaypointsLeft() is false.
    //--------------------------------------------------------------------------
    [[nodiscard]] Node* getNextWaypoint() const
    {
        return m_nodes[m_visited];
    }

    //--------------------------------------------------------------------------
    //! \brief Mark the next crossroads as passed.
    //--------------------------------------------------------------------------
    void takeWaypoint()
    {
        ++m_visited;
    }

    //--------------------------------------------------------------------------
    //! \return an iterator on the first crossroads still ahead.
    //--------------------------------------------------------------------------
    std::vector<Node*>::const_iterator begin() const
    {
        return m_nodes.begin() + std::vector<Node*>::difference_type(m_visited);
    }

    //--------------------------------------------------------------------------
    //! \return the end of the remaining route.
    //--------------------------------------------------------------------------
    std::vector<Node*>::const_iterator end() const
    {
        return m_nodes.end();
    }

    //--------------------------------------------------------------------------
    //! \return how many crossroads remain.
    //--------------------------------------------------------------------------
    [[nodiscard]] size_t getWaypointCount() const
    {
        return m_nodes.size() - m_visited;
    }

private:

    //! \brief Crossroads of the trip, in drive order. Passed ones stay in the
    //! list.
    //!
    //! Empty when the agent already stands at the last one. This happens when
    //! the destination is on the start node or on a segment next to it.
    std::vector<Node*> m_nodes;

    //! \brief How many leading entries of m_nodes were passed. An index is
    //! used instead of erasing from the front, because removing the front each
    //! tick makes a long trip slow.
    size_t m_visited = 0u;

    //! \brief Segment the destination sits on, or nullptr at a crossroads.
    Segment* m_approachSegment = nullptr;

    //! \brief Building for the load, or nullptr when none was found.
    Building* m_destination = nullptr;

    // The small members come last, together, so that they fill one gap instead
    // of leaving one between each pointer. Every Agent owns a Route.

    //! \brief Position on m_approachSegment, in [0..1].
    float m_approachOffset = 0.0f;

    //! \brief Total travel time in seconds of game time, using traffic at
    //! search time.
    float m_cost = 0.0f;

    //! \brief True when a route was found.
    bool m_found = false;
};

//==============================================================================
//! \brief Interface for finding delivery routes. Lets you swap the routing
//! algorithm without changing City or Agent.
//==============================================================================
class IRouter
{
public:

    virtual ~IRouter() = default;

    //--------------------------------------------------------------------------
    //! \brief Find the cheapest building that accepts the load.
    //! \param[in] fromNode the crossroads to start from.
    //! \param[in] searchTarget the name to match against building targets.
    //! \param[in] resources the load, checked against free space. Not modified.
    //! \return the route. Route::isFound() is false when nothing accepts it.
    //--------------------------------------------------------------------------
    [[nodiscard]] virtual Route findRoute(Node& fromNode,
                                          Name const& searchTarget,
                                          Resources const& resources) = 0;

    //--------------------------------------------------------------------------
    //! \brief Return the travel time of findRoute() without storing the route.
    //! An agent uses this to see if the current road is worse than another.
    //! \param[in] fromNode the crossroads to start from.
    //! \param[in] searchTarget the name to match.
    //! \param[in] resources the load, checked against free space. Not modified.
    //! \return travel time in seconds of game time, or ROUTING_INFINITY when
    //! nothing accepts the load.
    //--------------------------------------------------------------------------
    [[nodiscard]] virtual float
    computeShortestPathCost(Node& fromNode,
                            Name const& searchTarget,
                            Resources const& resources) = 0;

    //--------------------------------------------------------------------------
    //! \brief Return the next crossroads without storing a full route.
    //! \param[in] fromNode the crossroads to start from.
    //! \param[in,out] searchTarget the name to match. Unchanged by the default
    //! router.
    //! \param[in,out] resources the load. Unchanged by the default router.
    //! \return the next node, \p fromNode when the destination is there, or a
    //! random neighbour when nothing was found.
    //--------------------------------------------------------------------------
    virtual Node*
    findNextNode(Node& fromNode, Name& searchTarget, Resources& resources) = 0;

    //--------------------------------------------------------------------------
    //! \brief Set the random seed for wandering. Does nothing on a router that
    //! never picks at random.
    //! \param[in] seed the seed value.
    //--------------------------------------------------------------------------
    virtual void setRandomSeed(unsigned seed)
    {
        (void)seed;
    }
};

} // namespace ogb

#endif
