//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file Agent.hpp
//! \brief Travellers driving on the road network to carry resources from one
//! building to another.

#ifndef OPEN_GLASSBOX_AGENT_HPP
#define OPEN_GLASSBOX_AGENT_HPP

#include "OpenGlassBox/Config.hpp"
#include "OpenGlassBox/Entity.hpp"
#include "OpenGlassBox/Router.hpp"

namespace ogb
{

class Unit;

//==============================================================================
//! \brief One trip: a load of resources leaving a building, looking for another
//! one that accepts it.
//!
//! Agents are created by unit rules, one per \c agent command that fires, and
//! they are the visible traffic of the city. They run no rules of their own: a
//! city may have a thousand of them alive at once, and giving each one a
//! ruleset would cost more than the rest of the simulation put together.
//!
//! What an Agent knows is what it carries, the name it is looking for, and an
//! itinerary. It does not know which building it will end at: the router
//! returns the cheapest one answering to that name and having room, and the
//! answer may change while the Agent drives, which is why the itinerary is
//! recomputed from time to time. Finding nothing at all is not an error, and an
//! Agent that has looked for RoutingConfig::agentGiveUpTicks hands its load
//! back to the building that sent it out. That is the wandering the demo shows:
//! it means the city is short of somewhere to deliver, not that the router is
//! broken.
//!
//! An Agent lives on a Segment at an offset, not on a Node, and it counts
//! towards the traffic of that Segment, which is what makes the road slower for
//! everybody else. It disappears the moment it unloads, so there is no
//! population sitting inside a building: what a building has is the traffic
//! coming and going.
//!
//! Example:
//! \code
//! // What a unit rule does, in effect: send one worker out with one person.
//! Resources load;
//! load.addResource("People", 1u);
//! city.addAgent(workerType, home, load, "Work");
//!
//! // The City drives them and takes away the ones that are done, so watching a
//! // trip means reading the list again rather than keeping a reference.
//! city.update();
//! for (auto const& agent : city.agents())
//! {
//!     std::cout << agent->type() << " looking for " << agent->searchTarget()
//!               << ", " << agent->remainingCost() << "s to go\n";
//! }
//! \endcode
//!
//! The matching script, where \c to names what the Agent will look for and the
//! brackets are the load it carries:
//! \code
//! unitRule SendPeopleToWork
//!     rate 45 minutes
//!     local People remove 1
//!     agent Worker to Work add [ People 1 ]
//! end
//! \endcode
//==============================================================================
class Agent: public Entity<AgentType>
{
public:

    // -------------------------------------------------------------------------
    //! \brief Leave a building with a load to deliver.
    //! \param[in] id identifier given by the City, unique among its Agents.
    //! \param[in] type recipe of the traveller. Kept by reference and has to
    //! outlive the Agent.
    //! \param[in] owner the building it leaves, which is also where the load
    //! goes back if nothing accepts it. The Agent starts at its position, and
    //! along the street when the building stands along one.
    //! \param[in] resources what it carries. Copied.
    //! \param[in] searchTarget the name of what it is looking for, matched
    //! against the \c targets of the buildings.
    // -------------------------------------------------------------------------
    Agent(uint32_t id,
          AgentType const& type,
          Unit& owner,
          Resources const& resources,
          Name const& searchTarget);

    // -------------------------------------------------------------------------
    //! \brief Take itself out of the traffic count of the Segment it was
    //! driving on.
    // -------------------------------------------------------------------------
    ~Agent();

    // -------------------------------------------------------------------------
    //! \brief Drive for one tick: follow the itinerary, recompute it when it
    //! has gone stale, and knock at the door when standing in front of one.
    //! \param[in] router the router of the City. Reused rather than built
    //! here: it keeps its scratch buffers between calls.
    //! \param[in] config settings read for the giving up delay and the routing
    //! intervals. The City passes its own; it is not held.
    //! \param[in] dt seconds of game time in one tick.
    //! \return true when the Agent is done, either because it has unloaded or
    //! because it has given up, in which case the City takes it away.
    // -------------------------------------------------------------------------
    bool update(IRouter& router, RoutingConfig const& config, float dt);

    // -------------------------------------------------------------------------
    //! \brief \return what it carries, and what room is left.
    //!
    //! \note Writable: unloading moves the load into the building, and giving
    //! up moves it back home.
    // -------------------------------------------------------------------------
    [[nodiscard]] Resources& getResources()
    {
        return m_resources;
    }

    //! \copydoc getResources()
    [[nodiscard]] Resources const& getResources() const
    {
        return m_resources;
    }

    // -------------------------------------------------------------------------
    //! \brief The name it is looking for, such as "Work" or "Shop". Matched
    //! against the \c targets of the buildings, not against their type.
    // -------------------------------------------------------------------------
    [[nodiscard]] Name const& getTarget() const
    {
        return m_searchTarget;
    }

    // -------------------------------------------------------------------------
    //! \brief The segment it is driving on, or nullptr when it stands at a
    //! crossroads with nowhere to go.
    // -------------------------------------------------------------------------
    [[nodiscard]] Segment const* getSegment() const
    {
        return m_currentSegment;
    }

    // -------------------------------------------------------------------------
    //! \brief \return where along getSegment() it stands, from 0 at the
    //! origin node to 1 at the other end.
    // -------------------------------------------------------------------------
    [[nodiscard]] float getOffset() const
    {
        return m_offset;
    }

    // -------------------------------------------------------------------------
    //! \brief Top speed of its type, in world units per second of game time.
    //! What it actually drives is the lesser of this and the speed of the
    //! Segment under it.
    // -------------------------------------------------------------------------
    [[nodiscard]] float getSpeed() const
    {
        return m_type.speed;
    }

    // -------------------------------------------------------------------------
    //! \brief \return the itinerary it is following. Route::isFound() is false
    //! while it has none, which happens when nothing accepts its load.
    // -------------------------------------------------------------------------
    [[nodiscard]] Route const& getRoute() const
    {
        return m_route;
    }

    // -------------------------------------------------------------------------
    //! \brief How long the rest of the trip takes, in seconds of game time,
    //! traffic included. Zero when it has no itinerary.
    // -------------------------------------------------------------------------
    [[nodiscard]] float getRemainingCost() const;

    // -------------------------------------------------------------------------
    //! \brief What the Agent would pay if it left for the cheapest acceptable
    //! building right now, from where it stands, at the current travel times.
    //!
    //! The counterpart of getRemainingCost(), and what it has to be compared
    //! against: both are counted from getNextRoutingNode(). Ask for this rather
    //! than calling IRouter::computeShortestPathCost() directly, because the
    //! agent has to stand out of its own way first. The place it holds at its
    //! destination is a claim against the other agents, and a search made
    //! without lifting it finds that destination full and answers with a dearer
    //! building, which reads as an agent that would gain by rerouting when it
    //! would not.
    //!
    //! \param[in] router the router of the City.
    //! \return the cost in seconds of game time, or ROUTING_INFINITY when
    //! nothing reachable accepts the load.
    // -------------------------------------------------------------------------
    [[nodiscard]] float computeRerouteCost(IRouter& router);

    // -------------------------------------------------------------------------
    //! \brief The crossroads it last stood at, which is where it is routed
    //! from, or nullptr when the road network under it has been demolished.
    // -------------------------------------------------------------------------
    [[nodiscard]] Node* getPreviousNode() const
    {
        return m_lastNode;
    }

    // -------------------------------------------------------------------------
    //! \brief \return the crossroads the router starts from: the one being
    //! driven to, or the one last stood at. Matches getRemainingCost().
    // -------------------------------------------------------------------------
    [[nodiscard]] Node* getNextRoutingNode() const;

    // -------------------------------------------------------------------------
    //! \brief The building that sent it out, or nullptr once that building has
    //! been demolished. Where the load goes back when the Agent gives up.
    // -------------------------------------------------------------------------
    [[nodiscard]] Unit* getOwner() const
    {
        return m_owner;
    }

    // -------------------------------------------------------------------------
    //! \brief Follow the City as it is moved in the world.
    //! \param[in] direction how far the City moved.
    // -------------------------------------------------------------------------
    void translate(Vector3f const& direction)
    {
        m_position += direction;
    }

    // -------------------------------------------------------------------------
    //! \brief Put the Agent back exactly where a save file says it was, which
    //! is the one case where its position is not the outcome of driving.
    //! \param[in] position where it stands, in world coordinates.
    //! \param[in] segment the segment it was driving on, or nullptr.
    //! \param[in] offset where along that segment, in [0..1].
    //! \param[in] last the crossroads it came from, or nullptr.
    //!
    //! The itinerary is dropped: the next tick recomputes it on the network as
    //! it was loaded.
    // -------------------------------------------------------------------------
    void relocate(Vector3f const& position,
                  Segment* segment,
                  float offset,
                  Node* last);

    // -------------------------------------------------------------------------
    //! \brief Forget the itinerary without moving: it was computed on a network
    //! that has since changed.
    // -------------------------------------------------------------------------
    void invalidateRoute();

    // -------------------------------------------------------------------------
    //! \brief Let go of a Segment that is about to be destroyed.
    //!
    //! Has to be called while the Segment is still alive, because the Agent has
    //! to take itself out of its traffic count. An Agent driving on it is
    //! parked on the Node it came from.
    //!
    //! \param[in] segment the segment being demolished.
    // -------------------------------------------------------------------------
    void forget(Segment const& segment);

    // -------------------------------------------------------------------------
    //! \brief Let go of a Node that is about to be destroyed. Call it after
    //! forget() has been called for every Segment incident to that Node.
    //! \param[in] node the crossroads being demolished.
    // -------------------------------------------------------------------------
    void forget(Node const& node);

    // -------------------------------------------------------------------------
    //! \brief Let go of a Unit that is about to be destroyed: the building it
    //! came from, and the one its itinerary was aiming at. A rule that abandons
    //! a house does that while Agents are still driving to it.
    //! \param[in] unit the building being demolished.
    // -------------------------------------------------------------------------
    void forget(Unit const& unit);

    // -------------------------------------------------------------------------
    //! \brief Whether the Agent has nowhere left to stand: no Segment under it
    //! and no Node with a road.
    //!
    //! Such an Agent can neither move nor deliver, and the City takes it away
    //! rather than leave it floating over a demolished neighbourhood.
    // -------------------------------------------------------------------------
    [[nodiscard]] bool isStuck() const;

private:

    // -------------------------------------------------------------------------
    //! \brief Whether the Agent is on that Segment, or means to approach a
    //! building along it.
    //! \param[in] segment the segment to test.
    // -------------------------------------------------------------------------
    bool uses(Segment const& segment) const;

    // -------------------------------------------------------------------------
    //! \brief Whether that Node is on the itinerary, or is one of the two the
    //! Agent is driving between.
    //! \param[in] node the crossroads to test.
    // -------------------------------------------------------------------------
    bool uses(Node const& node) const;

    // -------------------------------------------------------------------------
    //! \brief Knock at the door in front of the Agent and hand the load over.
    //! \return true when nothing is left to deliver.
    // -------------------------------------------------------------------------
    bool unloadResources();

    // -------------------------------------------------------------------------
    //! \brief Drive along the current Segment towards the next Node.
    //! \param[in] dt seconds of game time in one tick.
    // -------------------------------------------------------------------------
    void moveTowardsNextNode(float dt);

    // -------------------------------------------------------------------------
    //! \brief Take the next segment of the itinerary, recomputing it when there
    //! is none or when it has gone stale.
    //! \param[in] router the router of the City.
    //! \param[in] config settings read for the routing intervals.
    // -------------------------------------------------------------------------
    void followRoute(IRouter& router, RoutingConfig const& config);

    // -------------------------------------------------------------------------
    //! \brief Replace the itinerary if it is worth it: when there is none, when
    //! it has been held long enough, or when the traffic has made it much worse
    //! than the current shortest one.
    //!
    //! Comparing against the shortest one costs a whole graph search, so it
    //! only happens every RoutingConfig::pathCheckTicks ticks, and the
    //! itinerary that search produced is kept rather than searched for twice.
    //!
    //! \param[in] router the router of the City.
    //! \param[in] config settings read for the routing intervals.
    // -------------------------------------------------------------------------
    void maybeRecomputeRoute(IRouter& router, RoutingConfig const& config);

    // -------------------------------------------------------------------------
    //! \brief Ask the router for the cheapest building answering to
    //! m_searchTarget, and keep the way there.
    //! \param[in] router the router of the City.
    // -------------------------------------------------------------------------
    void computeRoute(IRouter& router);

    // -------------------------------------------------------------------------
    //! \brief The building the Agent is standing in front of and that accepts
    //! its load, or nullptr.
    // -------------------------------------------------------------------------
    Unit* searchUnit();

    // -------------------------------------------------------------------------
    //! \brief Move onto another Segment, keeping the traffic count of both of
    //! them straight. Pass nullptr to leave the network.
    //! \param[in] segment the segment to drive on, or nullptr to leave the
    //! network.
    // -------------------------------------------------------------------------
    void setCurrentSegment(Segment* segment);

    // -------------------------------------------------------------------------
    //! \brief Whether the Agent stands between the two ends of a Segment, which
    //! is where a building anchored along a street leaves it.
    // -------------------------------------------------------------------------
    bool standingAlongSegment() const;

    // -------------------------------------------------------------------------
    //! \brief Leave the Segment by the end the destination is really behind.
    //!
    //! Standing along a segment the Agent may drive off either way, and the
    //! near end is not always the good one: everyone leaving a factory placed
    //! at a fifth of the street drove to the nearest corner first, so an Agent
    //! bound for a shop to the east was first seen heading west.
    //! \param[in] router the router of the City.
    // -------------------------------------------------------------------------
    void computeRouteAlongSegment(IRouter& router);

    // -------------------------------------------------------------------------
    //! \brief The end of the Segment the Agent has to reach before it can take
    //! another one, which is the crossroads it came in by. Null when the Agent
    //! is not standing on a Segment, or when the Node it came from is not one
    //! of its ends.
    // -------------------------------------------------------------------------
    [[nodiscard]] Node* getSegmentExit() const;

    bool followRouteWhileAlongSegment();
    void nextNodeFromRoute(Node* next);
    void followRouteWhenLost(IRouter& router);
    void followRouteAlongNodes();
    void followRouteApproach();
    bool arrivedAtDestination() const;
    bool giveUp();

    // -------------------------------------------------------------------------
    //! \brief The only way m_route is ever written.
    //!
    //! Going through one place is what keeps the claim on the destination
    //! honest: it gives back the place held at the old building and takes one
    //! at the new. An assignment that escaped it would leave a building
    //! reserved for an Agent that is no longer coming, and a building whose
    //! count never comes back down is invisible to every Agent for the rest of
    //! the game.
    //!
    //! \param[in] route the new itinerary, or a default-built one to have none.
    // -------------------------------------------------------------------------
    void setRoute(Route&& route);

    // -------------------------------------------------------------------------
    //! \brief Take a place at the destination of the current itinerary, unless
    //! one is held already or there is no destination.
    // -------------------------------------------------------------------------
    void claimDestination();

    // -------------------------------------------------------------------------
    //! \brief Give back the place held, if any. Safe to call twice.
    // -------------------------------------------------------------------------
    void releaseDestination();

    // -------------------------------------------------------------------------
    //! \brief The body of update(), which brackets it with the release and the
    //! retaking of the claim.
    //! \param[in] router the router of the City.
    //! \param[in] config settings read for the giving up delay and the routing
    //! intervals.
    //! \param[in] dt seconds of game time in one tick.
    //! \return true when the Agent is done.
    // -------------------------------------------------------------------------
    bool tick(IRouter& router, RoutingConfig const& config, float dt);

private:

    //! \brief The building it left, and where the load goes back if nothing
    //! accepts it. Null once that building has been demolished. Not owned.
    Unit* m_owner = nullptr;
    //! \brief The name it is looking for, from the \c to of the script.
    Name m_searchTarget;
    //! \brief What it carries.
    Resources m_resources;
    //! \brief Where along m_currentSegment it stands, in [0..1] from
    //! m_currentSegment->from().
    float m_offset = 0.0f;
    //! \brief The segment under it, which counts it as traffic. Not owned.
    Segment* m_currentSegment = nullptr;
    //! \brief The crossroads it came from, and what it is routed from. Not
    //! owned.
    Node* m_lastNode = nullptr;
    //! \brief The crossroads it is driving to, or null when it stands still.
    //! Not owned.
    Node* m_nextNode = nullptr;
    //! \brief Cached itinerary. Recomputed periodically, and as soon as the
    //! remaining cost drifts too far from the current shortest path. Written
    //! through route() and nowhere else.
    Route m_route;
    //! \brief The building a place is currently held at, or nullptr. The one
    //! authority on whether a claim is outstanding: the route destination says
    //! where the Agent is going, this says what has to be given back. Not
    //! owned, and always cleared before the building is destroyed.
    Unit* m_reservation = nullptr;
    //! \brief Ticks spent on the current itinerary, against
    //! RoutingConfig::pathRecalcTicks.
    uint32_t m_ticksOnRoute = 0u;
    //! \brief Ticks spent without an itinerary, wandering from one crossroads
    //! to the next because nothing accepts what the Agent carries. Against
    //! RoutingConfig::agentGiveUpTicks.
    uint32_t m_ticksLost = 0u;
};

//! \brief The Agents of a City, which owns them.
using Agents = std::vector<std::unique_ptr<Agent>>;

} // namespace ogb

#endif
