//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file Agent.hpp
//! \brief Travellers on the road network. They carry resources from one
//! building to another.

#ifndef OPEN_GLASSBOX_AGENT_HPP
#define OPEN_GLASSBOX_AGENT_HPP

#include "OpenGlassBox/Config.hpp"
#include "OpenGlassBox/Entity.hpp"
#include "OpenGlassBox/Router.hpp"

namespace ogb
{

class Building;

//==============================================================================
//! \brief A traveller carrying a load from one building to another that
//! accepts it.
//!
//! Building rules create Agents. One Agent is created per \c agent command that
//! fires. Agents are the visible traffic of the city. They run no rules of
//! their own. A city may have thousands of Agents alive at once. Giving each
//! one a ruleset would cost more than the rest of the simulation.
//!
//! An Agent knows what it carries, the name it looks for, and an itinerary.
//! It does not know which building it will reach. The router returns the
//! cheapest building with that name and free room. The answer may change while
//! the Agent drives. That is why the itinerary is recomputed from time to time.
//! Finding nothing is not an error. After RoutingConfig::agentGiveUpTicks ticks
//! with no destination, the Agent returns its load to the building that sent
//! it. In the demo this looks like wandering. It means the city lacks a
//! delivery target, not that the router is broken.
//!
//! An Agent lives on a Segment at an offset, not on a Node. It counts towards
//! the traffic of that Segment. That makes the road slower for everyone else.
//! It disappears when it unloads. There is no population inside a building. A
//! building only has traffic coming and going.
//!
//! Example:
//! \code
//! // What a building rule does: send one worker out with one person.
//! Resources load;
//! load.addResource("People", 1u);
//! city.addAgent(workerType, home, load, "Work");
//!
//! // The City drives them and removes finished ones. To watch a trip, read the
//! // list again instead of keeping a reference.
//! city.update();
//! for (auto const& agent : city.agents())
//! {
//!     std::cout << agent->type() << " looking for " << agent->searchTarget()
//!               << ", " << agent->remainingCost() << "s to go\n";
//! }
//! \endcode
//!
//! Matching script. \c to is the name the Agent looks for. The brackets hold
//! the load it carries:
//! \code
//! buildingRule SendPeopleToWork
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
    //! \param[in] id identifier from the City. Unique among its Agents.
    //! \param[in] type recipe of the traveller. Kept by reference. Must
    //! outlive the Agent.
    //! \param[in] owner the building it leaves. The load returns here if
    //! nothing accepts it. The Agent starts at its position, or along the
    //! street when the building stands on a Segment.
    //! \param[in] resources what it carries. Copied.
    //! \param[in] searchTarget the name it looks for. Matched against the
    //! \c targets of buildings.
    // -------------------------------------------------------------------------
    Agent(uint32_t id,
          AgentType const& type,
          Building& owner,
          Resources const& resources,
          Name const& searchTarget);

    // -------------------------------------------------------------------------
    //! \brief Remove itself from the traffic count of the Segment it drives on.
    // -------------------------------------------------------------------------
    ~Agent();

    // -------------------------------------------------------------------------
    //! \brief Drive for one tick. Follow the itinerary. Recompute it when it
    //! is stale. Try to unload when in front of a building.
    //! \param[in] router the router of the City. Reused between calls. It
    //! keeps scratch buffers.
    //! \param[in] config settings for give-up delay and routing intervals. The
    //! City passes its own. Not stored.
    //! \param[in] dt seconds of game time in one tick.
    //! \return true when the Agent is done. It has unloaded or given up. The
    //! City removes it then.
    // -------------------------------------------------------------------------
    bool update(IRouter& router, RoutingConfig const& config, float dt);

    // -------------------------------------------------------------------------
    //! \return what it carries and what room is left.
    //!
    //! \note Writable. Unloading moves the load into the building. Giving up
    //! moves it back home.
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
    //! \brief The name it looks for, such as "Work" or "Shop". Matched against
    //! the \c targets of buildings, not against their type.
    // -------------------------------------------------------------------------
    [[nodiscard]] Name const& getTarget() const
    {
        return m_searchTarget;
    }

    // -------------------------------------------------------------------------
    //! \brief The Segment it drives on, or nullptr when it stands at a
    //! crossroads with nowhere to go.
    // -------------------------------------------------------------------------
    [[nodiscard]] Segment const* getSegment() const
    {
        return m_currentSegment;
    }

    // -------------------------------------------------------------------------
    //! \return position along getSegment(), from 0 at the origin Node
    //! to 1 at the other end.
    // -------------------------------------------------------------------------
    [[nodiscard]] float getOffset() const
    {
        return m_offset;
    }

    // -------------------------------------------------------------------------
    //! \brief Top speed of its type, in world units per second of game time.
    //! Actual speed is the lesser of this and the speed of the Segment under
    //! it.
    // -------------------------------------------------------------------------
    [[nodiscard]] float getSpeed() const
    {
        return m_type.speed;
    }

    // -------------------------------------------------------------------------
    //! \return the itinerary it follows. Route::isFound() is false when
    //! it has none. That happens when nothing accepts its load.
    // -------------------------------------------------------------------------
    [[nodiscard]] Route const& getRoute() const
    {
        return m_route;
    }

    // -------------------------------------------------------------------------
    //! \brief Time left for the rest of the trip, in seconds of game time,
    //! traffic included. Zero when it has no itinerary.
    // -------------------------------------------------------------------------
    [[nodiscard]] float getRemainingCost() const;

    // -------------------------------------------------------------------------
    //! \brief Cost to reach the cheapest acceptable building now, from the
    //! current position, at current travel times.
    //!
    //! Counterpart of getRemainingCost(). Compare against that value. Both
    //! start from getNextRoutingNode(). Call this instead of
    //! IRouter::computeShortestPathCost() directly. The Agent must stand out
    //! of its own way first. Its place at the destination is a claim against
    //! other Agents. A search without lifting it finds the destination full
    //! and returns a costlier building. That would look like rerouting would
    //! help when it would not.
    //!
    //! \param[in] router the router of the City.
    //! \return cost in seconds of game time, or ROUTING_INFINITY when nothing
    //! reachable accepts the load.
    // -------------------------------------------------------------------------
    [[nodiscard]] float computeRerouteCost(IRouter& router);

    // -------------------------------------------------------------------------
    //! \brief The crossroads it last stood at. Routing starts from here.
    //! nullptr when the road network under it was demolished.
    // -------------------------------------------------------------------------
    [[nodiscard]] Node* getPreviousNode() const
    {
        return m_lastNode;
    }

    // -------------------------------------------------------------------------
    //! \return the crossroads the router starts from: the one it drives
    //! to, or the one it last stood at. Matches getRemainingCost().
    // -------------------------------------------------------------------------
    [[nodiscard]] Node* getNextRoutingNode() const;

    // -------------------------------------------------------------------------
    //! \brief The building that sent it out, or nullptr once that building was
    //! demolished. The load returns here when the Agent gives up.
    // -------------------------------------------------------------------------
    [[nodiscard]] Building* getOwner() const
    {
        return m_owner;
    }

    // -------------------------------------------------------------------------
    //! \brief Follow the City when it moves in the world.
    //! \param[in] direction how far the City moved.
    // -------------------------------------------------------------------------
    void translate(Vector3f const& direction)
    {
        m_position += direction;
    }

    // -------------------------------------------------------------------------
    //! \brief Place the Agent exactly where a save file says it was. This is
    //! the only case where position is not set by driving.
    //! \param[in] position where it stands, in world coordinates.
    //! \param[in] segment the Segment it drove on, or nullptr.
    //! \param[in] offset position along that Segment, in [0..1].
    //! \param[in] last the crossroads it came from, or nullptr.
    //!
    //! The itinerary is dropped. The next tick recomputes it on the loaded
    //! network.
    // -------------------------------------------------------------------------
    void relocate(Vector3f const& position,
                  Segment* segment,
                  float offset,
                  Node* last);

    // -------------------------------------------------------------------------
    //! \brief Clear the itinerary without moving. The network changed since it
    //! was computed.
    // -------------------------------------------------------------------------
    void invalidateRoute();

    // -------------------------------------------------------------------------
    //! \brief Release a Segment that will be destroyed.
    //!
    //! Call while the Segment is still alive. The Agent must remove itself
    //! from its traffic count. An Agent on it is parked on the Node it came
    //! from.
    //!
    //! \param[in] segment the Segment being demolished.
    // -------------------------------------------------------------------------
    void forget(Segment const& segment);

    // -------------------------------------------------------------------------
    //! \brief Release a Node that will be destroyed. Call after forget() for
    //! every Segment connected to that Node.
    //! \param[in] node the crossroads being demolished.
    // -------------------------------------------------------------------------
    void forget(Node const& node);

    // -------------------------------------------------------------------------
    //! \brief Release a Building that will be destroyed: the source building
    //! and the destination on the itinerary. A rule that removes a house does
    //! this while Agents still drive to it.
    //! \param[in] building the building being demolished.
    // -------------------------------------------------------------------------
    void forget(Building const& building);

    // -------------------------------------------------------------------------
    //! \brief Whether the Agent has nowhere to stand: no Segment under it and
    //! no Node with a road.
    //!
    //! Such an Agent cannot move or deliver. The City removes it instead of
    //! leaving it over a demolished area.
    // -------------------------------------------------------------------------
    [[nodiscard]] bool isStuck() const;

private:

    // -------------------------------------------------------------------------
    //! \brief Whether the Agent is on that Segment, or will approach a
    //! building along it.
    //! \param[in] segment the Segment to test.
    // -------------------------------------------------------------------------
    bool uses(Segment const& segment) const;

    // -------------------------------------------------------------------------
    //! \brief Whether that Node is on the itinerary, or is one of the two the
    //! Agent drives between.
    //! \param[in] node the crossroads to test.
    // -------------------------------------------------------------------------
    bool uses(Node const& node) const;

    // -------------------------------------------------------------------------
    //! \brief Try to unload at the building in front of the Agent.
    //! \return true when nothing is left to deliver.
    // -------------------------------------------------------------------------
    bool unloadResources();

    // -------------------------------------------------------------------------
    //! \brief Drive along the current Segment towards the next Node.
    //! \param[in] dt seconds of game time in one tick.
    // -------------------------------------------------------------------------
    void moveTowardsNextNode(float dt);

    // -------------------------------------------------------------------------
    //! \brief Take the next Segment of the itinerary. Recompute when there is
    //! none or when it is stale.
    //! \param[in] router the router of the City.
    //! \param[in] config settings for routing intervals.
    // -------------------------------------------------------------------------
    void followRoute(IRouter& router, RoutingConfig const& config);

    // -------------------------------------------------------------------------
    //! \brief Replace the itinerary when needed: when there is none, when it
    //! is old enough, or when traffic made it much worse than the current
    //! shortest path.
    //!
    //! Comparing to the shortest path costs a full graph search. That only
    //! runs every RoutingConfig::pathCheckTicks ticks. The itinerary from
    //! that search is kept instead of searching twice.
    //!
    //! \param[in] router the router of the City.
    //! \param[in] config settings for routing intervals.
    // -------------------------------------------------------------------------
    void maybeRecomputeRoute(IRouter& router, RoutingConfig const& config);

    // -------------------------------------------------------------------------
    //! \brief Ask the router for the cheapest building matching
    //! m_searchTarget. Keep the path there.
    //! \param[in] router the router of the City.
    // -------------------------------------------------------------------------
    void computeRoute(IRouter& router);

    // -------------------------------------------------------------------------
    //! \brief The building in front of the Agent that accepts its load, or
    //! nullptr.
    // -------------------------------------------------------------------------
    Building* searchBuilding();

    // -------------------------------------------------------------------------
    //! \brief Move to another Segment. Keep traffic counts correct on both.
    //! Pass nullptr to leave the network.
    //! \param[in] segment the Segment to drive on, or nullptr to leave the
    //! network.
    // -------------------------------------------------------------------------
    void setCurrentSegment(Segment* segment);

    // -------------------------------------------------------------------------
    //! \brief Whether the Agent stands between the two ends of a Segment. A
    //! building on a street leaves the network here.
    // -------------------------------------------------------------------------
    bool standingAlongSegment() const;

    // -------------------------------------------------------------------------
    //! \brief Leave the Segment by the end where the destination really is.
    //!
    //! When standing along a Segment the Agent may drive either way. The near
    //! end is not always correct. Everyone leaving a factory at one fifth of
    //! the street drove to the nearest corner first. An Agent bound east was
    //! first seen heading west.
    //! \param[in] router the router of the City.
    // -------------------------------------------------------------------------
    void computeRouteAlongSegment(IRouter& router);

    // -------------------------------------------------------------------------
    //! \brief The end of the Segment the Agent must reach before taking
    //! another. This is the crossroads it entered by. Null when the Agent is
    //! not on a Segment, or when the Node it came from is not an end.
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
    //! \brief The only place m_route is written.
    //!
    //! One entry point keeps the destination claim correct. It releases the
    //! place at the old building and takes one at the new. Direct assignment
    //! would leave a building reserved for an Agent that no longer comes. A
    //! count that never drops hides the building from every Agent for the rest
    //! of the game.
    //!
    //! \param[in] route the new itinerary, or a default-built one for none.
    // -------------------------------------------------------------------------
    void setRoute(Route&& route);

    // -------------------------------------------------------------------------
    //! \brief Reserve a place at the destination of the current itinerary,
    //! unless one is already held or there is no destination.
    // -------------------------------------------------------------------------
    void claimDestination();

    // -------------------------------------------------------------------------
    //! \brief Release the held place, if any. Safe to call twice.
    // -------------------------------------------------------------------------
    void releaseDestination();

    // -------------------------------------------------------------------------
    //! \brief Body of update(). Brackets it with release and retake of the
    //! claim.
    //! \param[in] router the router of the City.
    //! \param[in] config settings for give-up delay and routing intervals.
    //! \param[in] dt seconds of game time in one tick.
    //! \return true when the Agent is done.
    // -------------------------------------------------------------------------
    bool tick(IRouter& router, RoutingConfig const& config, float dt);

private:

    //! \brief The building it left. Load returns here if nothing accepts it.
    //! Null once demolished. Not owned.
    Building* m_owner = nullptr;
    //! \brief The name it looks for, from the \c to of the script.
    Name m_searchTarget;
    //! \brief What it carries.
    Resources m_resources;
    //! \brief Position along m_currentSegment, in [0..1] from
    //! m_currentSegment->from().
    float m_offset = 0.0f;
    //! \brief The Segment under it. It counts as traffic on it. Not owned.
    Segment* m_currentSegment = nullptr;
    //! \brief The crossroads it came from. Routing starts here. Not owned.
    Node* m_lastNode = nullptr;
    //! \brief The crossroads it drives to, or null when it stands still. Not
    //! owned.
    Node* m_nextNode = nullptr;
    //! \brief Cached itinerary. Recomputed periodically and when remaining
    //! cost drifts too far from the shortest path. Written only through
    //! route().
    Route m_route;
    //! \brief Building where a place is currently held, or nullptr. Tracks
    //! whether a claim is active. The route destination says where the Agent
    //! goes; this says what must be released. Not owned. Always cleared before
    //! the building is destroyed.
    Building* m_reservation = nullptr;
    //! \brief Ticks on the current itinerary, compared to
    //! RoutingConfig::pathRecalcTicks.
    uint32_t m_ticksOnRoute = 0u;
    //! \brief Ticks without an itinerary, moving between crossroads because
    //! nothing accepts the load. Compared to RoutingConfig::agentGiveUpTicks.
    uint32_t m_ticksLost = 0u;
};

//! \brief The Agents of a City. The City owns them.
using Agents = std::vector<std::unique_ptr<Agent>>;

} // namespace ogb

#endif
