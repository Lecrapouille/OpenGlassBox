//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file Path.hpp
//! \brief Road network: crossroads, segments, and the graph.

#ifndef OPEN_GLASSBOX_PATH_HPP
#define OPEN_GLASSBOX_PATH_HPP

#include "OpenGlassBox/Types.hpp"
#include "OpenGlassBox/Vector.hpp"

#include <deque>
#include <map>
#include <memory>
#include <vector>

namespace ogb
{

class Segment;
class Path;
class Building;

// =============================================================================
//! \brief A crossroads. An Agent can change street here. A building can sit
//! here.
//!
//! This is a graph vertex. It stores its position, the segments that meet here,
//! and the buildings here. The router uses the segments to walk the network.
//! An Agent uses the buildings to know it has arrived. A Node with no segment
//! is an orphan from a demolished street. City::removeIsolatedNodes removes
//! them.
//!
//! Agents stop only at Nodes and at addresses on a Segment. A building on a
//! street cuts the street in two. The junction becomes an address. See
//! City::splitSegment.
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
//!     city.addBuilding(rules.getBuildingType("Home"), corner);
//! \endcode
// =============================================================================
class Node
{
    friend Segment;
    friend Path;

public:

    // -------------------------------------------------------------------------
    //! \brief Leaves id and position unset. Only friend classes use this.
    //! They fill in both fields.
    // -------------------------------------------------------------------------
    Node() = default;

    // -------------------------------------------------------------------------
    //! \param[in] id Unique id inside the Path.
    //! \param[in] position World position.
    // -------------------------------------------------------------------------
    Node(size_t id, Vector3f const& position);

    // -------------------------------------------------------------------------
    //! \brief Copies segment and building lists as-is. Neither side knows the
    //! copy. The save loader uses this.
    // -------------------------------------------------------------------------
    Node(Node const&) = default;

    // -------------------------------------------------------------------------
    //! \brief Register a building here.
    //!
    //! The Building calls this when it anchors to this Node. An Agent can then
    //! deliver to a building here.
    //!
    //! \param[in] building The building. Not owned. Not registered twice.
    // -------------------------------------------------------------------------
    void addBuilding(Building& building);

    // -------------------------------------------------------------------------
    //! \brief Unregister a building. Does nothing if it was not here.
    //! The building is not changed.
    //! \param[in] building The building to remove.
    // -------------------------------------------------------------------------
    void removeBuilding(Building& building);

    // -------------------------------------------------------------------------
    //! \return true if at least one segment meets here.
    //! false means an orphan. No Agent can reach or leave it.
    // -------------------------------------------------------------------------
    [[nodiscard]] bool hasSegments() const
    {
        return !m_segments.empty();
    }

    // -------------------------------------------------------------------------
    //! \brief Segment between this crossroads and a neighbor.
    //! \param[in] node The neighbor.
    //! \return First segment with both as ends, or nullptr if not neighbors.
    //! Two crossroads may share more than one segment. Use getSegments() for
    //! all.
    // -------------------------------------------------------------------------
    [[nodiscard]] Segment* findSegmentTo(Node const& node) const;

    // -------------------------------------------------------------------------
    //! \return Unique id inside the Path. Saves and undo use this id.
    // -------------------------------------------------------------------------
    [[nodiscard]] size_t getId() const
    {
        return m_id;
    }

    // -------------------------------------------------------------------------
    //! \return Index in the Path node list, in [0..Path::getNodeCount()[.
    //!
    //! Not the same as getId(). Ids are assigned once and survive demolition.
    //! The index is renumbered to stay dense. Routers can use plain arrays
    //! indexed by node.
    //!
    //! Meaningless for a Node built outside a Path, as in unit tests.
    // -------------------------------------------------------------------------
    [[nodiscard]] size_t getIndex() const
    {
        return m_index;
    }

    // -------------------------------------------------------------------------
    //! \return World position.
    // -------------------------------------------------------------------------
    [[nodiscard]] Vector3f const& getPosition() const
    {
        return m_position;
    }

    // -------------------------------------------------------------------------
    //! \brief Move with the City.
    //! \param[in] direction How far the City moved.
    // -------------------------------------------------------------------------
    void translate(Vector3f const& direction);

    // -------------------------------------------------------------------------
    //! \return Segments at this crossroads, in any order.
    //! The router walks them and reads traffic on each.
    // -------------------------------------------------------------------------
    [[nodiscard]] std::vector<Segment*> const& getSegments() const
    {
        return m_segments;
    }

    // -------------------------------------------------------------------------
    //! \return Buildings here. An Agent can deliver to one of them.
    // -------------------------------------------------------------------------
    [[nodiscard]] std::vector<Building*> const& getBuildings() const
    {
        return m_buildings;
    }

    // -------------------------------------------------------------------------
    //! \return The Path that owns this crossroads, or nullptr for a
    //! standalone Node, as in unit tests.
    // -------------------------------------------------------------------------
    [[nodiscard]] Path* getPath() const
    {
        return m_path;
    }

private:

    //! \brief Unique id inside the Path.
    size_t m_id = 0u;

    //! \brief Dense index in the Path list. See getIndex().
    size_t m_index = 0u;

    //! \brief Segments at this crossroads. Not owned. The Path owns them.
    std::vector<Segment*> m_segments;

    //! \brief Buildings here. Not owned. The City owns them.
    std::vector<Building*> m_buildings;

    //! \brief Owning Path, or nullptr for a standalone Node.
    Path* m_path = nullptr;

    //! \brief World position. Three floats, so it sits last where the gap it
    //! leaves is the one the struct would end on anyway.
    Vector3f m_position;
};

//! \brief Owns a crossroads.
using NodePtr = std::unique_ptr<Node>;

//! \brief Crossroads in a Path. A deque keeps addresses stable when you add
//! one.
using Nodes = std::deque<NodePtr>;

// =============================================================================
//! \brief A street between two crossroads. An Agent drives on it.
//!
//! This is an undirected graph edge. One-way traffic is not modeled. It stores
//! its length, how many Agents are on it, and the drive time from both. See
//! getTravelTime(). The router minimizes travel time, not length. Traffic
//! spreads across streets instead of queuing on the shortest one.
//!
//! A Segment also stores buildings along it. One street with forty houses stays
//! one segment instead of forty.
//!
//! Example:
//! \code
//! Segment& street = road.addSegment(dirtType, corner, end);
//!
//! // A shop halfway down the street, and how busy the street is.
//! city.addBuilding(shopType, road, street, 0.5f);
//! std::cout << street.getAgentCount() << " agents, "
//!           << street.getTravelTime() << "s to drive, saturation "
//!           << street.getSaturation() << '\n';
//! \endcode
//!
//! The matching script. The three numbers are BPR parameters:
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
    //! \param[in] id Unique id inside the Path.
    //! \param[in] type Segment recipe: speed, capacity, and congestion factor.
    //! Kept by reference. Must outlive the Segment.
    //! \param[in] from One end.
    //! \param[in] to The other end. Traffic is undirected. Building offsets
    //! count from \c from.
    // -------------------------------------------------------------------------
    Segment(size_t id, SegmentType const& type, Node& from, Node& to);

    // -------------------------------------------------------------------------
    //! \return Unique id inside the Path.
    // -------------------------------------------------------------------------
    [[nodiscard]] size_t getId() const
    {
        return m_id;
    }

    // -------------------------------------------------------------------------
    //! \return The end that building offsets count from.
    //! \note Returns const, but the Node is not: the Path owns the Node.
    //! The Segment only points to it.
    // -------------------------------------------------------------------------
    [[nodiscard]] Node& getFrom() const
    {
        return *m_from;
    }

    // -------------------------------------------------------------------------
    //! \return The other end.
    // -------------------------------------------------------------------------
    [[nodiscard]] Node& getTo() const
    {
        return *m_to;
    }

    // -------------------------------------------------------------------------
    //! \return World position of getFrom().
    // -------------------------------------------------------------------------
    [[nodiscard]] Vector3f const& getFromPosition() const
    {
        return m_from->getPosition();
    }

    // -------------------------------------------------------------------------
    //! \return World position of getTo().
    // -------------------------------------------------------------------------
    [[nodiscard]] Vector3f const& getToPosition() const
    {
        return m_to->getPosition();
    }

    // -------------------------------------------------------------------------
    //! \return Length in world units. Cached at construction and when
    //! an end moves.
    // -------------------------------------------------------------------------
    [[nodiscard]] float getLength() const
    {
        return m_length;
    }

    // -------------------------------------------------------------------------
    //! \return Type name, such as "Dirt".
    // -------------------------------------------------------------------------
    [[nodiscard]] Name const& getTypeName() const
    {
        return m_type.name;
    }

    // -------------------------------------------------------------------------
    //! \return Type color as 0xRRGGBB. The demo draws the segment with
    //! it unless traffic colors are on.
    // -------------------------------------------------------------------------
    [[nodiscard]] uint32_t getColor() const
    {
        return m_type.color;
    }

    // -------------------------------------------------------------------------
    //! \brief Point along the segment.
    //! \param[in] offset Position from 0 at getFrom() to 1 at getTo().
    //! Values outside that range extrapolate.
    //! \return World position at that offset.
    // -------------------------------------------------------------------------
    [[nodiscard]] Vector3f getPositionAt(float offset) const;

    // -------------------------------------------------------------------------
    //! \brief Register a building along this segment, not at an end.
    //! The Building calls this.
    //! \param[in] building The building. Not owned. Not registered twice.
    // -------------------------------------------------------------------------
    void addBuilding(Building& building);

    // -------------------------------------------------------------------------
    //! \brief Unregister a building. Does nothing if it was not here.
    //! \param[in] building The building to remove.
    // -------------------------------------------------------------------------
    void removeBuilding(Building& building);

    // -------------------------------------------------------------------------
    //! \return Buildings along this segment. An Agent can deliver to one
    //! while driving past.
    // -------------------------------------------------------------------------
    [[nodiscard]] std::vector<Building*> const& getBuildings() const
    {
        return m_buildings;
    }

    // -------------------------------------------------------------------------
    //! \return Drive time with no traffic, in game seconds.
    //! Length divided by free-flow speed of the type.
    // -------------------------------------------------------------------------
    [[nodiscard]] float getFreeFlowTime() const
    {
        return m_t0;
    }

    // -------------------------------------------------------------------------
    //! \brief Drive time under current traffic, in game seconds.
    //!
    //! Uses the Bureau of Public Roads arc performance function from 1964:
    //!
    //!     t = t0 * (1 + 0.15 * (flow / capacity)^beta)
    //!
    //! An empty segment costs free-flow time. At capacity it costs 15% more.
    //! Above capacity the exponent makes cost rise fast. Traffic may exceed
    //! capacity. A saturated street stays passable but expensive. The router
    //! sends the next Agent elsewhere.
    //!
    //! Uses smoothed flow, not the instant count. See smoothFlow().
    //!
    //! Cached, not computed each call. Flow updates once per tick in
    //! smoothFlow(). The router reads this twice per segment. Computing the
    //! power here was the hottest instruction in the simulation.
    //!
    //! \return Travel time, at least free-flow time.
    // -------------------------------------------------------------------------
    [[nodiscard]] float getTravelTime() const
    {
        return m_travelTime;
    }

    // -------------------------------------------------------------------------
    //! \return Smoothed traffic divided by type capacity. Above 1 means
    //! overload. The demo traffic colors use this.
    // -------------------------------------------------------------------------
    [[nodiscard]] float getSaturation() const
    {
        return m_flow / m_type.capacity;
    }

    // -------------------------------------------------------------------------
    //! \return Smoothed traffic in Agents. Not a whole number. It is a
    //! moving average of the instant count.
    // -------------------------------------------------------------------------
    [[nodiscard]] float getFlow() const
    {
        return m_flow;
    }

    // -------------------------------------------------------------------------
    //! \brief Set smoothed traffic directly. Only a save load should do this.
    //! The moving average is part of city state. Starting at zero makes every
    //! street look empty for the first few minutes.
    //! \param[in] flow Average to restore. Negative values become zero.
    // -------------------------------------------------------------------------
    void setFlow(float flow)
    {
        m_flow = (flow < 0.0f) ? 0.0f : flow;
        updateTravelTime();
    }

    // -------------------------------------------------------------------------
    //! \return Number of Agents on the segment right now.
    // -------------------------------------------------------------------------
    [[nodiscard]] uint32_t getAgentCount() const
    {
        return m_agentCount;
    }

    // -------------------------------------------------------------------------
    //! \return Agent count before travel time grows much, from the type.
    //! Not a hard limit.
    // -------------------------------------------------------------------------
    [[nodiscard]] float getCapacity() const
    {
        return m_type.capacity;
    }

    // -------------------------------------------------------------------------
    //! \brief Count one more Agent. The Agent calls this when it enters.
    // -------------------------------------------------------------------------
    void addAgent();

    // -------------------------------------------------------------------------
    //! \brief Count one less Agent. The Agent calls this when it leaves.
    //! The destructor also calls this. An Agent deleted mid-street must not
    //! leave a phantom count, or the street stays congested forever.
    // -------------------------------------------------------------------------
    void removeAgent();

    // -------------------------------------------------------------------------
    //! \brief Move smoothed traffic one step toward the instant count.
    //! Uses successive averages:
    //!
    //!     F <- (1 - alpha) * F + alpha * Y
    //!
    //! Called once per tick. Routing on the instant count makes everyone switch
    //! routes back and forth. Everyone sees an empty road, everyone takes it,
    //! everyone sees it full.
    //!
    //! \param[in] alpha Step size in ]0..1]. Small values damp more. One
    //! disables smoothing. See TrafficConfig::smoothing.
    // -------------------------------------------------------------------------
    void smoothFlow(float alpha);

private:

    // -------------------------------------------------------------------------
    //! \brief Recompute length and free-flow travel time. Called at
    //! construction and when an end moves.
    // -------------------------------------------------------------------------
    void updateLength();

    // -------------------------------------------------------------------------
    //! \brief Recompute congested travel time from free-flow time and smoothed
    //! traffic. Called when either changes: updateLength(), smoothFlow(), and
    //! setFlow() on save load.
    // -------------------------------------------------------------------------
    void updateTravelTime();

private:

    //! \brief Smallest flow change worth recomputing travel time, in vehicles.
    //! Smoothing never reaches the instant count exactly. An equality test
    //! would keep evaluating BPR on every quiet street forever. It is also
    //! unsafe for float comparison.
    static constexpr float FLOW_EPSILON = 1e-3f;

    //! \brief Unique id inside the Path.
    size_t m_id;
    //! \brief Segment recipe, shared by every segment of that type.
    SegmentType const& m_type;
    //! \brief End that building offsets count from. Not owned.
    Node* m_from = nullptr;
    //! \brief Other end. Not owned.
    Node* m_to = nullptr;
    //! \brief Buildings along the segment. Not owned.
    std::vector<Building*> m_buildings;
    //! \brief Cached length in world units.
    float m_length = 0.0f;
    //! \brief Cached free-flow travel time in game seconds.
    float m_t0 = 0.0f;
    //! \brief Agents on the segment right now.
    uint32_t m_agentCount = 0u;
    //! \brief Moving average of m_agentCount. BPR uses this as flow.
    float m_flow = 0.0f;
    //! \brief Cached BPR result for current m_flow. See getTravelTime().
    float m_travelTime = 0.0f;
};

//! \brief Owns a segment.
using SegmentPtr = std::unique_ptr<Segment>;

//! \brief Segments in a Path. A deque keeps addresses stable when you add one.
using Segments = std::deque<SegmentPtr>;

// =============================================================================
//! \brief A point where a street being drawn runs over one already laid.
//!
//! Returned by Path::findCrossings(). It says where to cut, not what to do
//! about it: the caller splits the segment and joins the pieces, because only
//! the caller knows which identifiers to hand out and how to take the edit
//! back. See City::splitSegment().
// =============================================================================
struct Crossing
{
    //! \brief The segment already laid that the line runs over. Never null.
    Segment* segment = nullptr;

    //! \brief Where on that segment, from 0 at its getFrom() to 1 at its
    //! getTo(). Exactly 0 or 1 when the line passes over one of its ends,
    //! which is a junction that needs no cut.
    float segmentOffset = 0.0f;

    //! \brief Where on the line, from 0 at its start to 1 at its end.
    float lineOffset = 0.0f;
};

// =============================================================================
//! \brief One network: crossroads and segments an Agent can drive on.
//!
//! The player draws it, or a save file loads it. A City may hold several Paths.
//! They never meet. A road network and a rail network are two Paths. An Agent
//! on one cannot hop to the other. The router never leaves the Path it started
//! on.
//!
//! The Path owns its crossroads and segments. It returns references to them.
//! Those references stay valid while the object lives. That is why both
//! containers are deques.
//!
//! Example:
//! \code
//! Path& road = city.addPath(simulation.getRuleset().getPathType("Road"));
//! Node& a = road.addNode({ 0.0f, 0.0f, 0.0f });
//! Node& b = road.addNode({ 60.0f, 0.0f, 0.0f });
//! Segment& street = road.addSegment(dirtType, a, b);
//!
//! // Putting a building halfway down the street: cut it in two, and the
//! // junction becomes the address. City::splitSegment does this and takes care
//! of
//! // the buildings and the Agents already there.
//! Node& junction = city.splitSegment(road, street, 0.5f);
//! city.addBuilding(homeType, junction);
//! \endcode
//!
//! The matching script, one line per network kind and one per segment kind:
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
    //! \brief Empty network: no crossroads, no segments.
    //! \param[in] type Network recipe. Kept by reference. Must outlive the
    //! Path.
    // -------------------------------------------------------------------------
    explicit Path(PathType const& type);

    Path(Path const&) = delete;
    Path& operator=(Path const&) = delete;
    Path(Path&&) = delete;
    Path& operator=(Path&&) = delete;

    // -------------------------------------------------------------------------
    //! \brief Add a crossroads. Assign a new id.
    //! \param[in] position World position.
    //! \return The new crossroads.
    // -------------------------------------------------------------------------
    Node& addNode(Vector3f const& position);

    // -------------------------------------------------------------------------
    //! \brief Add a crossroads with a saved id.
    //!
    //! Undo and save load need the same id. Commands on top refer to it.
    //!
    //! \param[in] id Id to assign.
    //! \param[in] position World position.
    //! \return The new crossroads, or the existing one if that id is in use.
    // -------------------------------------------------------------------------
    Node& addNode(size_t id, Vector3f const& position);

    // -------------------------------------------------------------------------
    //! \param[in] id Id to look up.
    //! \return The crossroads, or nullptr if it no longer exists.
    // -------------------------------------------------------------------------
    [[nodiscard]] Node* findNode(size_t id) const;

    // -------------------------------------------------------------------------
    //! \brief Destroy a crossroads and every segment attached to it.
    //!
    //! \note Buildings and Agents may still refer to it. The caller must handle
    //! them first. Use City::removeNode on a live city. Calling this directly
    //! leaves dangling pointers.
    //!
    //! \param[in] node The crossroads to destroy.
    // -------------------------------------------------------------------------
    void removeNode(Node& node);

    // -------------------------------------------------------------------------
    //! \brief Remove every crossroads and segment. Keep the network type.
    //! Player-drawn geometry goes away. The ruleset network kind stays so roads
    //! can be laid again.
    //!
    //! \note Same warning as removeNode(). The caller handles buildings and
    //! Agents first.
    // -------------------------------------------------------------------------
    void clear();

    // -------------------------------------------------------------------------
    //! \brief Add a segment between two crossroads. Assign a new id.
    //! \param[in] type Segment recipe. Kept by reference in the Segment.
    //! \param[in] p1 One end. Building offsets count from here.
    //! \param[in] p2 The other end.
    //! \return The new segment.
    // -------------------------------------------------------------------------
    Segment& addSegment(SegmentType const& type, Node& p1, Node& p2);

    // -------------------------------------------------------------------------
    //! \brief Add a segment with a saved id.
    //! See addNode(size_t, Vector3f const&) for why that matters.
    //! \param[in] id Id to assign.
    //! \param[in] type Segment recipe.
    //! \param[in] p1, p2 Its two ends.
    //! \return The new segment, or the existing one if that id is in use.
    // -------------------------------------------------------------------------
    Segment& addSegment(size_t id, SegmentType const& type, Node& p1, Node& p2);

    // -------------------------------------------------------------------------
    //! \param[in] id Id to look up.
    //! \return The segment, or nullptr if it no longer exists.
    // -------------------------------------------------------------------------
    [[nodiscard]] Segment* findSegment(size_t id) const;

    // -------------------------------------------------------------------------
    //! \brief Destroy a segment and remove it from both ends.
    //!
    //! Both crossroads stay, even as orphans. The player may reconnect them.
    //!
    //! \note Agents on the segment may still refer to it. The caller must
    //! handle them first. Use City::removeSegment.
    //!
    //! \param[in] segment The segment to destroy.
    // -------------------------------------------------------------------------
    void removeSegment(Segment& segment);

    // -------------------------------------------------------------------------
    //! \brief Cut a segment in two at a new crossroads.
    //!
    //! The first half keeps its id and gets shorter. The second half is new.
    //! Only the graph changes. Buildings stay on the half that may no longer
    //! run under them. Agents keep an offset that now means something else.
    //! Call City::splitSegment on a live city.
    //!
    //! \param[in] segment Segment to cut.
    //! \param[in] offset Cut position from 0 at segment.getFrom() to 1 at
    //! segment.getTo().
    //! \return The new crossroads, or an end if offset is on an end. Nothing
    //! was cut in that case.
    // -------------------------------------------------------------------------
    Node& splitSegment(Segment& segment, float offset);

    // -------------------------------------------------------------------------
    //! \brief Where a straight line from one point to another runs over the
    //! segments already laid, in the order it meets them.
    //!
    //! Two streets drawn over one another are a crossroads a driver can turn
    //! at, and this is what tells the caller where to make one. A network whose
    //! type says its lines do not cross returns nothing, so that a water main
    //! passing under a power line stays two separate networks.
    //!
    //! A meeting at an end of an existing segment is reported with a
    //! Crossing::segmentOffset of exactly 0 or 1: the junction is that node,
    //! and splitting there cuts nothing. A line running along a segment rather
    //! than across it is not reported: there is no single point to cut at.
    //!
    //! \param[in] from Start of the line, usually a node that exists already.
    //! \param[in] to End of the line.
    //! \return The crossings, sorted by Crossing::lineOffset. Points at the
    //! very ends of the line are left out: the ends are junctions already.
    // -------------------------------------------------------------------------
    [[nodiscard]] std::vector<Crossing> findCrossings(Vector3f const& from,
                                                      Vector3f const& to) const;

    // -------------------------------------------------------------------------
    //! \brief The segment a point stands on, so that a street ending in the
    //! middle of another can be made to join it there.
    //!
    //! A dead end drawn onto a street is a T junction, and without this it was
    //! a node touching a street it shared nothing with: the two looked joined
    //! and no agent could turn from one into the other. Obeys the crossings of
    //! the network type for the same reason findCrossings() does.
    //!
    //! \param[in] position The point.
    //! \param[in] tolerance How far from a segment the point may be, in world
    //! units, and still count as standing on it.
    //! \param[out] offset Where along the segment, from 0 at its getFrom() to 1
    //! at its getTo(), snapped to an end when it lands near one.
    //! \return The nearest such segment, or nullptr when the point stands on
    //! none.
    // -------------------------------------------------------------------------
    [[nodiscard]] Segment* findSegmentAt(Vector3f const& position,
                                         float tolerance,
                                         float& offset) const;

    // -------------------------------------------------------------------------
    //! \brief Move every crossroads and segment with the City.
    //! \param[in] direction How far the City moved.
    // -------------------------------------------------------------------------
    void translate(Vector3f const& direction) const;

    // -------------------------------------------------------------------------
    //! \return Type name, such as "Road".
    // -------------------------------------------------------------------------
    [[nodiscard]] Name const& getTypeName() const
    {
        return m_type.name;
    }

    // -------------------------------------------------------------------------
    //! \return The type recipe. Another City can get the same network
    //! kind without a name lookup.
    // -------------------------------------------------------------------------
    [[nodiscard]] PathType const& getType() const
    {
        return m_type;
    }

    // -------------------------------------------------------------------------
    //! \return All crossroads in creation order.
    // -------------------------------------------------------------------------
    [[nodiscard]] Nodes const& getNodes() const
    {
        return m_nodes;
    }

    // -------------------------------------------------------------------------
    //! \return Crossroads count. Also the upper bound for Node::getIndex().
    //! Routers size node-indexed arrays from this.
    // -------------------------------------------------------------------------
    [[nodiscard]] size_t getNodeCount() const
    {
        return m_nodes.size();
    }

    // -------------------------------------------------------------------------
    //! \return All segments in creation order.
    // -------------------------------------------------------------------------
    [[nodiscard]] Segments const& getSegments() const
    {
        return m_segments;
    }

    // -------------------------------------------------------------------------
    //! \return Highest free-flow speed among segments, or 1 if empty.
    //! The router converts distance to travel time with this.
    // -------------------------------------------------------------------------
    [[nodiscard]] float getMaxFreeFlowSpeed() const
    {
        return m_maxFreeFlowSpeed;
    }

    // -------------------------------------------------------------------------
    //! \brief Advance smoothed traffic on every segment by one step.
    //! The City calls this once per tick. See Segment::smoothFlow().
    //! \param[in] alpha Step size in ]0..1].
    // -------------------------------------------------------------------------
    void updateTrafficSmoothing(float alpha) const;

private:

    // -------------------------------------------------------------------------
    //! \brief Recompute getMaxFreeFlowSpeed() cache. Called after removal,
    //! the only operation that can lower it.
    // -------------------------------------------------------------------------
    void updateMaxFreeFlowSpeed();

    // -------------------------------------------------------------------------
    //! \brief Renumber Node::getIndex() to stay dense after a removal.
    // -------------------------------------------------------------------------
    void reindexNodes() const;

private:

    //! \brief Network recipe, shared by every network of that kind.
    PathType const& m_type;
    //! \brief Owned crossroads. A deque keeps references valid when you add
    //! one.
    Nodes m_nodes;
    //! \brief Owned segments. Same reason as m_nodes.
    Segments m_segments;
    //! \brief Id for the next crossroads.
    size_t m_nextNodeId = 0u;
    //! \brief Id for the next segment.
    size_t m_nextSegmentId = 0u;
    //! \brief Cache of the highest free-flow speed in m_segments.
    float m_maxFreeFlowSpeed = 1.0f;
};

//! \brief Networks in a City, by name. The City owns them.
using Paths = std::map<std::string, std::unique_ptr<Path>, std::less<>>;

} // namespace ogb

#endif
