//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file Path.hpp
//! \brief The road network: crossroads, segments, and the graph they make up.

#ifndef OPEN_GLASSBOX_PATH_HPP
#define OPEN_GLASSBOX_PATH_HPP

#include "OpenGlassBox/Types.hpp"
#include "OpenGlassBox/Vector.hpp"

#include <deque>
#include <map>
#include <memory>

namespace ogb
{

class Segment;
class Path;
class Unit;

// =============================================================================
//! \brief A crossroads: a point of the network where an Agent may change
//! street, and an address a building may sit on.
//!
//! This is a vertex of the graph, and it holds more than a position: the
//! segments incident to it, which is how the router walks the network, and the
//! buildings standing on it, which is how an Agent knows it has arrived. A Node
//! with no segment at all is an orphan, left behind by a demolished street, and
//! City::removeIsolatedNodes sweeps those away.
//!
//! Agents only ever stop at Nodes and at addresses along a Segment. That is why
//! placing a building on a street cuts the street in two: the junction becomes
//! an address. See City::splitSegment.
//!
//! Example:
//! \code
//! ogb::Ruleset const& rules = simulation.getRuleset();
//! Path& road = city.addPath(rules.getPathType("Road"));
//! Node& corner = road.addNode({ 0.0f, 0.0f, 0.0f });
//! Node& end = road.addNode({ 60.0f, 0.0f, 0.0f });
//! road.addSegment(rules.getSegmentType("Dirt"), corner, end);
//!
//! if (corner.hasSegments())
//!     city.addUnit(rules.getUnitType("Home"), corner);
//! \endcode
// =============================================================================
class Node
{
    friend Segment;
    friend Path;

public:

    // -------------------------------------------------------------------------
    //! \brief Leaves the identifier and the position uninitialised. Only the
    //! friend classes, which fill them in, may use it.
    // -------------------------------------------------------------------------
    Node() = default;

    // -------------------------------------------------------------------------
    //! \brief \param[in] id identifier, unique inside the Path.
    //! \param[in] position where it stands, in world coordinates.
    // -------------------------------------------------------------------------
    Node(uint32_t id, Vector3f const& position);

    // -------------------------------------------------------------------------
    //! \brief Copies the lists of segments and buildings as they are, which
    //! makes the copy known to neither. Used by the save loader.
    // -------------------------------------------------------------------------
    Node(Node const&) = default;

    // -------------------------------------------------------------------------
    //! \brief Take note of a building standing here.
    //!
    //! Called by the Unit itself: a building anchors to its Node and the Node
    //! learns about it, so that an Agent arriving here can be offered a door.
    //!
    //! \param[in] unit the building. Not owned, and not registered twice.
    // -------------------------------------------------------------------------
    void addUnit(Unit& unit);

    // -------------------------------------------------------------------------
    //! \brief Forget a building. Does nothing when it was not standing here.
    //! The building itself is left alone.
    //! \param[in] unit the building to forget.
    // -------------------------------------------------------------------------
    void removeUnit(Unit& unit);

    // -------------------------------------------------------------------------
    //! \brief \return true when at least one segment is incident to this
    //! crossroads. A false answer means an orphan, which no Agent can reach or
    //! leave.
    // -------------------------------------------------------------------------
    [[nodiscard]] bool hasSegments() const
    {
        return !m_segments.empty();
    }

    // -------------------------------------------------------------------------
    //! \brief The segment joining this crossroads to a neighbour.
    //! \param[in] node the neighbour.
    //! \return the first segment having the two of them as ends, or nullptr
    //! when they are not neighbours. Two crossroads may be joined by more than
    //! one segment; getSegments() lists them all.
    // -------------------------------------------------------------------------
    [[nodiscard]] Segment* findSegmentTo(Node const& node) const;

    // -------------------------------------------------------------------------
    //! \brief \return the identifier, unique inside the Path. This is what a
    //! save file writes down and what an undo refers to.
    // -------------------------------------------------------------------------
    [[nodiscard]] uint32_t getId() const
    {
        return m_id;
    }

    // -------------------------------------------------------------------------
    //! \brief \return where the crossroads sits in the list of its Path, in
    //! [0..Path::getNodeCount()[.
    //!
    //! Not the same thing as getId(): identifiers are handed out once and
    //! survive a demolition, whereas this is renumbered so it stays dense. That
    //! is what lets a router keep its bookkeeping in plain arrays indexed by
    //! node rather than in a tree keyed by address.
    //!
    //! Meaningless on a Node built outside a Path, as the unit tests do.
    // -------------------------------------------------------------------------
    [[nodiscard]] uint32_t getIndex() const
    {
        return m_index;
    }

    // -------------------------------------------------------------------------
    //! \brief \return where it stands, in world coordinates.
    // -------------------------------------------------------------------------
    [[nodiscard]] Vector3f const& getPosition() const
    {
        return m_position;
    }

    // -------------------------------------------------------------------------
    //! \brief Follow the City as it is moved in the world.
    //! \param[in] direction how far the City moved.
    // -------------------------------------------------------------------------
    void translate(Vector3f const& direction);

    // -------------------------------------------------------------------------
    //! \brief \return the segments touching this crossroads, in no particular
    //! order. The router walks them and reads the traffic of each.
    // -------------------------------------------------------------------------
    [[nodiscard]] std::vector<Segment*> const& getSegments() const
    {
        return m_segments;
    }

    // -------------------------------------------------------------------------
    //! \brief \return the buildings standing here. An agent arriving hands its
    //! load over to one of them.
    // -------------------------------------------------------------------------
    [[nodiscard]] std::vector<Unit*> const& getUnits() const
    {
        return m_units;
    }

    // -------------------------------------------------------------------------
    //! \brief \return the network this crossroads belongs to, or nullptr for a
    //! Node built on its own, as the unit tests do.
    // -------------------------------------------------------------------------
    [[nodiscard]] Path* getPath() const
    {
        return m_path;
    }

private:

    //! \brief Identifier, unique inside the Path.
    uint32_t m_id = 0u;

    //! \brief Rank in the list of the Path, kept dense by it. See getIndex().
    uint32_t m_index = 0u;

    //! \brief Where it stands, in world coordinates.
    Vector3f m_position;

    //! \brief The segments incident to it. Not owned: the Path owns them.
    std::vector<Segment*> m_segments;

    //! \brief The buildings standing on it. Not owned: the City owns them.
    std::vector<Unit*> m_units;

    //! \brief The network it belongs to, or nullptr for a standalone Node.
    Path* m_path = nullptr;
};

//! \brief Owning handle on a crossroads.
using NodePtr = std::unique_ptr<Node>;

//! \brief The crossroads of a Path. A deque rather than a vector so that adding
//! one does not move the others: everything refers to them by address.
using Nodes = std::deque<NodePtr>;

// =============================================================================
//! \brief A street: the segment between two crossroads, and what an Agent
//! drives on.
//!
//! This is an edge of the graph, undirected because one-way traffic is not
//! modelled. It knows its length, how many agents are on it, and from those two
//! how long it takes to drive: see getTravelTime(). That travel time, not the
//! length, is what the router minimises, which is what makes a city spread its
//! traffic over several streets instead of queueing all of it through the
//! shortest one.
//!
//! A Segment also holds the buildings standing along it, which is what lets a
//! street of forty houses stay one segment instead of becoming forty.
//!
//! Example:
//! \code
//! Segment& street = road.addSegment(dirtType, corner, end);
//!
//! // A shop halfway down the street, and how busy the street is.
//! city.addUnit(shopType, road, street, 0.5f);
//! std::cout << street.getAgentCount() << " agents, "
//!           << street.getTravelTime() << "s to drive, saturation "
//!           << street.getSaturation() << '\n';
//! \endcode
//!
//! The matching script, where the three numbers are the parameters of the BPR
//! function:
//! \code
//! segment Dirt color 0xAAAAAA speed 30 capacity 20 beta 4
//! \endcode
// =============================================================================
class Segment
{
    friend Node;
    friend Path;

public:

    Segment() = delete;

    // -------------------------------------------------------------------------
    //! \brief \param[in] id identifier, unique inside the Path.
    //! \param[in] type recipe of the segment: its speed, its capacity and how
    //! badly congestion hurts it. Kept by reference and has to outlive the Segment.
    //! \param[in] from one end.
    //! \param[in] to the other end. The two are interchangeable, traffic being
    //! undirected, but the offsets of the buildings are counted from \c from.
    // -------------------------------------------------------------------------
    Segment(uint32_t id, SegmentType const& type, Node& from, Node& to);

    // -------------------------------------------------------------------------
    //! \brief \return the identifier, unique inside the Path.
    // -------------------------------------------------------------------------
    [[nodiscard]] uint32_t getId() const
    {
        return m_id;
    }

    // -------------------------------------------------------------------------
    //! \brief \return the end offsets are counted from.
    //! \note Const, though the crossroads it hands out is not: the Path owns
    //! the crossroads, a segment only points at it.
    // -------------------------------------------------------------------------
    [[nodiscard]] Node& getFrom() const
    {
        return *m_from;
    }

    // -------------------------------------------------------------------------
    //! \brief \return the other end.
    // -------------------------------------------------------------------------
    [[nodiscard]] Node& getTo() const
    {
        return *m_to;
    }

    // -------------------------------------------------------------------------
    //! \brief \return where getFrom() stands, in world coordinates.
    // -------------------------------------------------------------------------
    [[nodiscard]] Vector3f const& getFromPosition() const
    {
        return m_from->getPosition();
    }

    // -------------------------------------------------------------------------
    //! \brief \return where getTo() stands, in world coordinates.
    // -------------------------------------------------------------------------
    [[nodiscard]] Vector3f const& getToPosition() const
    {
        return m_to->getPosition();
    }

    // -------------------------------------------------------------------------
    //! \brief \return the length in world units, cached at construction and
    //! whenever an end moves.
    // -------------------------------------------------------------------------
    [[nodiscard]] float getLength() const
    {
        return m_length;
    }

    // -------------------------------------------------------------------------
    //! \brief \return the name of its type, such as "Dirt".
    // -------------------------------------------------------------------------
    [[nodiscard]] Name const& getTypeName() const
    {
        return m_type.name;
    }

    // -------------------------------------------------------------------------
    //! \brief \return the colour of its type, as 0xRRGGBB. The demo draws the
    //! segment with it unless the traffic colours are on.
    // -------------------------------------------------------------------------
    [[nodiscard]] uint32_t getColor() const
    {
        return m_type.color;
    }

    // -------------------------------------------------------------------------
    //! \brief A point along the segment.
    //! \param[in] offset where along it, from 0 at getFrom() to 1 at getTo().
    //! Values outside that range extrapolate.
    //! \return the world position of that point.
    // -------------------------------------------------------------------------
    [[nodiscard]] Vector3f getPositionAt(float offset) const;

    // -------------------------------------------------------------------------
    //! \brief Take note of a building standing along this segment rather than
    //! on one of its ends. Called by the Unit itself.
    //! \param[in] unit the building. Not owned, and not registered twice.
    // -------------------------------------------------------------------------
    void addUnit(Unit& unit);

    // -------------------------------------------------------------------------
    //! \brief Forget a building. Does nothing when it was not standing here.
    //! \param[in] unit the building to forget.
    // -------------------------------------------------------------------------
    void removeUnit(Unit& unit);

    // -------------------------------------------------------------------------
    //! \brief \return the buildings standing along this segment. An agent
    //! driving past hands its load over to one of them.
    // -------------------------------------------------------------------------
    [[nodiscard]] std::vector<Unit*> const& getUnits() const
    {
        return m_units;
    }

    // -------------------------------------------------------------------------
    //! \brief \return how long the segment takes to drive with nobody on it, in
    //! seconds of game time: its length over the free flow speed of its type.
    // -------------------------------------------------------------------------
    [[nodiscard]] float getFreeFlowTime() const
    {
        return m_t0;
    }

    // -------------------------------------------------------------------------
    //! \brief How long the segment takes to drive under the current traffic, in
    //! seconds of game time.
    //!
    //! Given by the arc performance function published by the Bureau of Public
    //! Roads in 1964:
    //!
    //!     t = t0 * (1 + 0.15 * (flow / capacity)^beta)
    //!
    //! An empty segment costs its free flow time; one carrying its capacity
    //! costs fifteen percent more; beyond that the exponent makes the cost
    //! climb steeply. Nothing forbids the traffic from exceeding the capacity:
    //! a saturated street stays passable, it just becomes expensive, which is
    //! what makes the router send the next agent somewhere else.
    //!
    //! The flow used is the smoothed one, not the instantaneous count. See
    //! smoothFlow().
    //!
    //! Held rather than computed: the flow only moves once per tick, in
    //! smoothFlow(), whereas the router reads this twice per segment for every
    //! segment it considers. Evaluating the power here made it the single
    //! hottest instruction of the whole simulation.
    //!
    //! \return the travel time, always at least the free flow time.
    // -------------------------------------------------------------------------
    [[nodiscard]] float getTravelTime() const
    {
        return m_travelTime;
    }

    // -------------------------------------------------------------------------
    //! \brief \return the smoothed traffic over the capacity of the type. Above
    //! one the street carries more than it comfortably can, which is what the
    //! traffic colours of the demo shade.
    // -------------------------------------------------------------------------
    [[nodiscard]] float getSaturation() const
    {
        return m_flow / m_type.capacity;
    }

    // -------------------------------------------------------------------------
    //! \brief \return the smoothed traffic, in agents. Not a whole number: it
    //! is a moving average of the instantaneous count.
    // -------------------------------------------------------------------------
    [[nodiscard]] float getFlow() const
    {
        return m_flow;
    }

    // -------------------------------------------------------------------------
    //! \brief Set the smoothed traffic outright, which only a save being loaded
    //! has any business doing: the moving average is part of the state of a
    //! city, and starting it from zero makes every street look empty for the
    //! first few minutes.
    //! \param[in] flow the average to restore. Negative values are read as
    //! zero.
    // -------------------------------------------------------------------------
    void setFlow(float flow)
    {
        m_flow = (flow < 0.0f) ? 0.0f : flow;
        updateTravelTime();
    }

    // -------------------------------------------------------------------------
    //! \brief \return how many agents are on the segment right now.
    // -------------------------------------------------------------------------
    [[nodiscard]] uint32_t getAgentCount() const
    {
        return m_agentCount;
    }

    // -------------------------------------------------------------------------
    //! \brief \return how many agents the segment carries before the travel
    //! time starts to grow noticeably, from its type. Not a hard limit.
    // -------------------------------------------------------------------------
    [[nodiscard]] float getCapacity() const
    {
        return m_type.capacity;
    }

    // -------------------------------------------------------------------------
    //! \brief Count one more agent driving here. Called by the agent itself
    //! when it takes the segment.
    // -------------------------------------------------------------------------
    void addAgent();

    // -------------------------------------------------------------------------
    //! \brief Count one less. Called by the agent when it leaves the segment,
    //! and by its destructor: an agent deleted mid-street must not leave a
    //! phantom behind, or the street stays congested for ever.
    // -------------------------------------------------------------------------
    void removeAgent();

    // -------------------------------------------------------------------------
    //! \brief Move the smoothed traffic one step towards the instantaneous
    //! count, following the method of successive averages:
    //!
    //!     F <- (1 - alpha) * F + alpha * Y
    //!
    //! Called once per tick. Routing on the instantaneous count instead makes
    //! the whole population swing from one itinerary to the other and back, the
    //! classic oscillation of day-to-day traffic dynamics: everybody sees an
    //! empty road, everybody takes it, everybody sees it full.
    //!
    //! \param[in] alpha the step, in ]0..1]. Small values damp harder. One
    //! disables the smoothing altogether. See TrafficConfig::smoothing.
    // -------------------------------------------------------------------------
    void smoothFlow(float alpha);

private:

    // -------------------------------------------------------------------------
    //! \brief Recompute the length and the free flow travel time. Called at
    //! construction and whenever an end has moved.
    // -------------------------------------------------------------------------
    void updateLength();

    // -------------------------------------------------------------------------
    //! \brief Recompute the congested travel time from the free flow time and
    //! the smoothed traffic. Called from the only two places either of them
    //! changes: updateLength() and smoothFlow(), plus setFlow() when a save is
    //! loaded.
    // -------------------------------------------------------------------------
    void updateTravelTime();

private:

    //! \brief Smallest change in the smoothed traffic worth recomputing the
    //! travel time for, in vehicles. The smoothing converges towards the
    //! instantaneous count without ever reaching it, so a plain test for
    //! equality would keep evaluating the BPR power of every quiet street for
    //! ever, and would be an unsafe way to compare two floats besides.
    static constexpr float FLOW_EPSILON = 1e-3f;

    //! \brief Identifier, unique inside the Path.
    uint32_t m_id;
    //! \brief Recipe of the segment, shared with every segment of that type.
    SegmentType const& m_type;
    //! \brief The end offsets are counted from. Not owned.
    Node* m_from = nullptr;
    //! \brief The other end. Not owned.
    Node* m_to = nullptr;
    //! \brief Cached length, in world units.
    float m_length = 0.0f;
    //! \brief Cached travel time with nobody on it, in seconds of game time.
    float m_t0 = 0.0f;
    //! \brief Agents on the segment right now.
    uint32_t m_agentCount = 0u;
    //! \brief Moving average of m_agentCount, which is the flow the BPR
    //! function is fed.
    float m_flow = 0.0f;
    //! \brief Cached result of the BPR function for the current m_flow. See
    //! getTravelTime().
    float m_travelTime = 0.0f;
    //! \brief The buildings standing along it. Not owned.
    std::vector<Unit*> m_units;
};

//! \brief Owning handle on a segment.
using SegmentPtr = std::unique_ptr<Segment>;

//! \brief The segments of a Path. A deque for the same reason as Nodes.
using Segments = std::deque<SegmentPtr>;

// =============================================================================
//! \brief One network: a graph of crossroads and segments an Agent may drive
//! on.
//!
//! Drawn by the player, or laid by a save file. A City may hold several, and
//! they never meet: a road network and a rail network are two Paths, and an
//! Agent routed on one cannot hop onto the other. That separation is enforced
//! by the router, which never leaves the Path it started on.
//!
//! The Path owns its crossroads and its segments, and hands out references to
//! them. Those references stay valid as long as the thing they refer to is
//! alive, which is why the two containers are deques.
//!
//! Example:
//! \code
//! Path& road = city.addPath(simulation.getRuleset().getPathType("Road"));
//! Node& a = road.addNode({ 0.0f, 0.0f, 0.0f });
//! Node& b = road.addNode({ 60.0f, 0.0f, 0.0f });
//! Segment& street = road.addSegment(dirtType, a, b);
//!
//! // Putting a building halfway down the street: cut it in two, and the
//! // junction becomes the address. City::splitSegment does this and takes care of
//! // the buildings and the Agents already there.
//! Node& junction = city.splitSegment(road, street, 0.5f);
//! city.addUnit(homeType, junction);
//! \endcode
//!
//! The matching script, one line per kind of network and one per kind of
//! segment:
//! \code
//! paths
//!     path Road color 0xAAAAAA
//! end
//!
//! segments
//!     segment Dirt color 0xAAAAAA speed 30 capacity 20 beta 4
//! end
//! \endcode
// =============================================================================
class Path
{
public:

    // -------------------------------------------------------------------------
    //! \brief An empty network: no crossroads, no segments.
    //! \param[in] type recipe of the network. Kept by reference and has to
    //! outlive the Path.
    // -------------------------------------------------------------------------
    explicit Path(PathType const& type);

    Path(Path const&) = delete;
    Path& operator=(Path const&) = delete;
    Path(Path&&) = delete;
    Path& operator=(Path&&) = delete;

    // -------------------------------------------------------------------------
    //! \brief Add a crossroads, numbering it on its own.
    //! \param[in] position where it stands, in world coordinates.
    //! \return the new crossroads.
    // -------------------------------------------------------------------------
    Node& addNode(Vector3f const& position);

    // -------------------------------------------------------------------------
    //! \brief Add a crossroads with the identifier it had before being removed.
    //!
    //! Undoing an edit has to give the identifier back, since that is what the
    //! commands stacked on top of it refer to, and so does loading a save.
    //!
    //! \param[in] id the identifier to give it.
    //! \param[in] position where it stands, in world coordinates.
    //! \return the new crossroads, or the existing one when that identifier is
    //! still in use.
    // -------------------------------------------------------------------------
    Node& addNode(uint32_t id, Vector3f const& position);

    // -------------------------------------------------------------------------
    //! \brief \param[in] id identifier to look up.
    //! \return the crossroads, or nullptr when it no longer exists.
    // -------------------------------------------------------------------------
    [[nodiscard]] Node* findNode(uint32_t id) const;

    // -------------------------------------------------------------------------
    //! \brief Destroy a crossroads and every segment it is an end of.
    //!
    //! \note The buildings standing on it and the Agents driving towards it
    //! still refer to it, so the caller has to deal with them first. That is
    //! what City::removeNode is for, and calling this one directly on a live
    //! city leaves dangling pointers behind.
    //!
    //! \param[in] node the crossroads to destroy.
    // -------------------------------------------------------------------------
    void removeNode(Node& node);

    // -------------------------------------------------------------------------
    //! \brief Drop every crossroads and every segment, keeping the type of the
    //! network: what the player drew goes away, the kind of network the ruleset
    //! declared stays, so roads can be laid again.
    //!
    //! \note Same warning as removeNode(): the caller deals with the buildings
    //! and the Agents first.
    // -------------------------------------------------------------------------
    void clear();

    // -------------------------------------------------------------------------
    //! \brief Add a segment between two existing crossroads, numbering it on
    //! its own.
    //! \param[in] type recipe of the segment. Kept by reference by the Segment.
    //! \param[in] p1 one end, the one offsets are counted from.
    //! \param[in] p2 the other end.
    //! \return the new segment.
    // -------------------------------------------------------------------------
    Segment& addSegment(SegmentType const& type, Node& p1, Node& p2);

    // -------------------------------------------------------------------------
    //! \brief Add a segment with the identifier it had before being removed.
    //! See addNode(uint32_t, Vector3f const&) for why that matters.
    //! \param[in] id the identifier to give it.
    //! \param[in] type recipe of the segment.
    //! \param[in] p1, p2 its two ends.
    //! \return the new segment, or the existing one when that identifier is
    //! still in use.
    // -------------------------------------------------------------------------
    Segment& addSegment(uint32_t id, SegmentType const& type, Node& p1, Node& p2);

    // -------------------------------------------------------------------------
    //! \brief \param[in] id identifier to look up.
    //! \return the segment, or nullptr when it no longer exists.
    // -------------------------------------------------------------------------
    [[nodiscard]] Segment* findSegment(uint32_t id) const;

    // -------------------------------------------------------------------------
    //! \brief Destroy a segment and take it out of its two ends.
    //!
    //! The two crossroads are kept, even when they are left orphan: the player
    //! may well be about to join them up again.
    //!
    //! \note The Agents driving on it still refer to it, so the caller has to
    //! deal with them first. That is what City::removeSegment is for.
    //!
    //! \param[in] segment the segment to destroy.
    // -------------------------------------------------------------------------
    void removeSegment(Segment& segment);

    // -------------------------------------------------------------------------
    //! \brief Cut a segment in two, joined by a new crossroads.
    //!
    //! The first half keeps the identifier and is shortened; the second half is
    //! created. Only the graph is rewired: the buildings standing along the
    //! segment are left anchored to the half that may no longer run under them,
    //! and the Agents driving on it keep an offset that now means something
    //! else. City::splitSegment is the one to call on a live city.
    //!
    //! \param[in] segment the segment to cut.
    //! \param[in] offset where to cut, from 0 at segment.getFrom() to 1 at
    //! segment.getTo().
    //! \return the new crossroads, or the end of the segment when the offset
    //! falls on one, in which case nothing was cut.
    // -------------------------------------------------------------------------
    Node& splitSegment(Segment& segment, float offset);

    // -------------------------------------------------------------------------
    //! \brief Move every crossroads, and therefore every segment, as the City
    //! is moved in the world.
    //! \param[in] direction how far the City moved.
    // -------------------------------------------------------------------------
    void translate(Vector3f const& direction);

    // -------------------------------------------------------------------------
    //! \brief \return the name of its type, such as "Road".
    // -------------------------------------------------------------------------
    [[nodiscard]] Name const& getTypeName() const
    {
        return m_type.name;
    }

    // -------------------------------------------------------------------------
    //! \brief \return the recipe itself, so another City can be given a network
    //! of the same kind without looking it up by name.
    // -------------------------------------------------------------------------
    [[nodiscard]] PathType const& getType() const
    {
        return m_type;
    }

    // -------------------------------------------------------------------------
    //! \brief \return every crossroads of the network, in creation order.
    // -------------------------------------------------------------------------
    [[nodiscard]] Nodes const& getNodes() const
    {
        return m_nodes;
    }

    // -------------------------------------------------------------------------
    //! \brief \return how many crossroads the network has, which is also the
    //! bound on Node::getIndex() and therefore the size a router has to give
    //! the arrays it indexes by node.
    // -------------------------------------------------------------------------
    [[nodiscard]] size_t getNodeCount() const
    {
        return m_nodes.size();
    }

    // -------------------------------------------------------------------------
    //! \brief \return every segment of the network, in creation order.
    // -------------------------------------------------------------------------
    [[nodiscard]] Segments const& getSegments() const
    {
        return m_segments;
    }

    // -------------------------------------------------------------------------
    //! \brief \return the highest free flow speed among the segments of the
    //! network, or one when it has none. The router turns a distance into a
    //! travel time with it.
    // -------------------------------------------------------------------------
    [[nodiscard]] float getMaxFreeFlowSpeed() const
    {
        return m_maxFreeFlowSpeed;
    }

    // -------------------------------------------------------------------------
    //! \brief Advance the smoothed traffic of every segment by one step.
    //! Called once per tick by the City. See Segment::smoothFlow().
    //! \param[in] alpha the step, in ]0..1].
    // -------------------------------------------------------------------------
    void updateTrafficSmoothing(float alpha);

private:

    // -------------------------------------------------------------------------
    //! \brief Recompute the cache read by getMaxFreeFlowSpeed(). Called after a
    //! removal, which is the only thing that can lower it.
    // -------------------------------------------------------------------------
    void updateMaxFreeFlowSpeed();

    // -------------------------------------------------------------------------
    //! \brief Renumber Node::getIndex() so the indices stay dense after a
    //! crossroads has been removed.
    // -------------------------------------------------------------------------
    void reindexNodes();

private:

    //! \brief Recipe of the network, shared with every network of that kind.
    PathType const& m_type;
    //! \brief The crossroads, owned. A deque rather than a vector so that
    //! adding one does not invalidate the references handed out for the others.
    Nodes m_nodes;
    //! \brief The segments, owned. Same reason as m_nodes.
    Segments m_segments;
    //! \brief Identifier the next crossroads will be given.
    uint32_t m_nextNodeId = 0u;
    //! \brief Identifier the next segment will be given.
    uint32_t m_nextSegmentId = 0u;
    //! \brief Cache of the highest free flow speed among m_segments.
    float m_maxFreeFlowSpeed = 1.0f;
};

//! \brief The networks of a City, by name, which owns them.
using Paths = std::map<std::string, std::unique_ptr<Path>, std::less<>>;

} // namespace ogb

#endif
