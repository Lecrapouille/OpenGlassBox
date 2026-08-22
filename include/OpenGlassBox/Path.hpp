//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file Path.hpp
//! \brief Road network model: paths, nodes, ways and graph editing operations.


#ifndef OPEN_GLASSBOX_PATH_HPP
#  define OPEN_GLASSBOX_PATH_HPP

#  include "OpenGlassBox/Types.hpp"
#  include "OpenGlassBox/Vector.hpp"
#  include <deque>
#  include <map>
#  include <vector>
#  include <memory>

namespace ogb {

class Way;
class Path;
class Unit;

// =============================================================================
//! \brief Class defining the extremity of arcs constituting a path. This
//! class can be seen as nodes of a graph (named Path). This class is not a
//! basic structure holding position but it holds more information such the list
//! of neighbor Ways (graph arcs) and Units (houses, buildings) referring to it.
//! Units consume and output Agent carrying Resources along a Path and Node are
//! origin and destination for Agents.
// =============================================================================
class Node
{
    friend Way;
    friend Path;

public:

    // -------------------------------------------------------------------------
    //! \brief Uninitialized internal states. Only friend classes can then
    //! initialize fields.
    // -------------------------------------------------------------------------
    Node() = default;

    // -------------------------------------------------------------------------
    //! \brief Initialized internal states.
    //! \param[in] id: a unique id used to reference the node.
    //! \param[in] position: the position in the world.
    // -------------------------------------------------------------------------
    Node(uint32_t id, Vector3f const& position);

    // -------------------------------------------------------------------------
    //! \brief Constructor by copy.
    // -------------------------------------------------------------------------
    Node(Node const&) = default;

    // -------------------------------------------------------------------------
    //! \brief Attach a Unit owning this node.
    // -------------------------------------------------------------------------
    void addUnit(Unit& unit);

    // -------------------------------------------------------------------------
    //! \brief Detach a Unit from this node. Does nothing if the Unit was not
    //! attached. The Unit itself is not destroyed.
    // -------------------------------------------------------------------------
    void removeUnit(Unit& unit);

    // -------------------------------------------------------------------------
    //! \brief Check if the node is not has ways (aka not orphan).
    // -------------------------------------------------------------------------
    bool hasWays() const
    {
        return m_ways.size() > 0u;
    }

    // -------------------------------------------------------------------------
    //! \brief Helper function calling
    //! getMapPosition(uint32_t, uint32_t, uint32_t&, uint32_t&) from class T
    //! that implements gridSizeU() and gridSizeV() ie T can be City or Map.
    // -------------------------------------------------------------------------
    template<class T>
    void getMapPosition(T const& x, uint32_t& u, uint32_t& v)
    {
        getMapPosition(x.gridSizeU(), x.gridSizeV(), u, v);
    }

    // -------------------------------------------------------------------------
    //! \brief Return the first segment in which the given node belongs to.
    //! \note to get the full list, call ways().
    //! \param[in] node: the neighbor node.
    //! \return the address of the segment where extremity points are node and
    //! this instance. Return nullptr if the node was not a neighbor.
    // -------------------------------------------------------------------------
    Way* getWayToNode(Node const& node);

    // -------------------------------------------------------------------------
    //! \brief Return the unique identifier.
    // -------------------------------------------------------------------------
    uint32_t id() const { return m_id; }

    // -------------------------------------------------------------------------
    //! \brief Return the position inside the World coordinate.
    // -------------------------------------------------------------------------
    Vector3f const& position() const { return m_position; }

    // -------------------------------------------------------------------------
    //! \brief Translate the Node position of a given direction.
    // -------------------------------------------------------------------------
    void translate(Vector3f const direction);

    // -------------------------------------------------------------------------
    //! \brief Const getter of Ways hold by this instance.
    // -------------------------------------------------------------------------
    std::vector<Way*>& ways() { return m_ways; }

    // -------------------------------------------------------------------------
    //! \brief Getter of Units hold by this instance.
    // -------------------------------------------------------------------------
    std::vector<Unit*>& units() { return m_units; }

    // -------------------------------------------------------------------------
    //! \brief Getter the nth Unit
    // -------------------------------------------------------------------------
    Unit& unit(uint32_t const nth) { return *m_units[nth]; }

    // -------------------------------------------------------------------------
    //! \brief Return the Path this node belongs to, or nullptr if standalone.
    // -------------------------------------------------------------------------
    Path* path() const { return m_path; }

    // -------------------------------------------------------------------------
    //! \brief Getter/Setter of node color (global color)
    // -------------------------------------------------------------------------
    uint32_t& color()
    {
        static uint32_t c = 0xAAAAAA;
        return c;
    }

private:

    //! \brief Unique identifier.
    uint32_t              m_id;
    //! \brief World position.
    Vector3f              m_position;
    //! \brief Ways owning this node instance.
    std::vector<Way*>     m_ways;
    //! \brief Units is affected to a Path Node. Therefore nodes has to know them.
    std::vector<Unit*>    m_units;
    //! \brief Path owning this node (nullptr for standalone nodes).
    Path*                 m_path = nullptr;
};

using NodePtr = std::unique_ptr<Node>;
using Nodes = std::deque<NodePtr>;

// =============================================================================
//! \brief Class defining a segment inside a Path. An Way can been seen as an
//! arc on an undirected graph. An Way is the locomotion for Agents carrying
//! Resources. A way is defined by two Nodes. Arcs have no
//! direction because we (currently) do not manage one-way traffic.
// =============================================================================
class Way
{
    friend Node;
    friend Path;

public:

    Way() = delete;

    // -------------------------------------------------------------------------
    //! \brief Initialized the state of the Way.
    //! \param[in] id: unique identifier.
    //! \param[in] type: const reference of a given type of Way also referred
    //! internally. The referred instance shall not be deleted before this Way
    //! instance is destroyed.
    //! \param[in] from: The node of origin.
    //! \param[in] to: The node of destination.
    // -------------------------------------------------------------------------
    Way(uint32_t id, WayType const& type, Node& from, Node& to);

    // -------------------------------------------------------------------------
    //! \brief Return the unique identifier.
    // -------------------------------------------------------------------------
    uint32_t id() const { return m_id; }

    // -------------------------------------------------------------------------
    //! \brief Return the origin node.
    // -------------------------------------------------------------------------
    Node& from() { return *m_from; }
    Node const& from() const { return *m_from; }

    // -------------------------------------------------------------------------
    //! \brief Return the destination node.
    // -------------------------------------------------------------------------
    Node& to() { return *m_to; }
    Node const& to() const { return *m_to; }

    // -------------------------------------------------------------------------
    //! \brief Return the position of the origin node.
    // -------------------------------------------------------------------------
    Vector3f const& position1() const { return m_from->position(); }

    // -------------------------------------------------------------------------
    //! \brief Return the position of the destination node.
    // -------------------------------------------------------------------------
    Vector3f const& position2() const { return m_to->position(); }

    // -------------------------------------------------------------------------
    //! \brief Return the length of the segment that has been computed by
    //! updateLength().
    // -------------------------------------------------------------------------
    float magnitude() const { return m_magnitude; }

    // -------------------------------------------------------------------------
    //! \brief Getter: return the type of Way.
    // -------------------------------------------------------------------------
    std::string const& type() const { return m_type.name; }

    // -------------------------------------------------------------------------
    //! \brief Getter: return the color of the Way.
    // -------------------------------------------------------------------------
    uint32_t color() const { return m_type.color; }

    // -------------------------------------------------------------------------
    //! \brief Position of a point along the segment, with offset in [0..1]
    //! from the origin node.
    // -------------------------------------------------------------------------
    Vector3f positionAt(float offset) const;

    // -------------------------------------------------------------------------
    //! \brief Attach a Unit sitting on this segment rather than on a Node.
    // -------------------------------------------------------------------------
    void addUnit(Unit& unit);
    void removeUnit(Unit& unit);
    std::vector<Unit*>& units() { return m_units; }
    std::vector<Unit*> const& units() const { return m_units; }

    // -------------------------------------------------------------------------
    //! \brief Travel time of the segment when it carries no traffic, in seconds
    //! of game time. This is the length divided by the free flow speed.
    // -------------------------------------------------------------------------
    float freeFlowTime() const { return m_t0; }

    // -------------------------------------------------------------------------
    //! \brief Travel time of the segment under the current traffic, in seconds
    //! of game time, given by the BPR arc performance function
    //!
    //!     t = t0 * (1 + 0.15 * (flow / capacity)^beta)
    //!
    //! published by the Bureau of Public Roads in 1964. This is the cost the
    //! router minimizes, which is what makes Agents avoid congested segments.
    // -------------------------------------------------------------------------
    float travelTime() const;

    // -------------------------------------------------------------------------
    //! \brief Ratio of the smoothed flow over the capacity. Above one the
    //! segment is oversaturated. Used by the renderer to color the segment.
    // -------------------------------------------------------------------------
    float saturation() const { return m_flow / m_type.capacity; }

    // -------------------------------------------------------------------------
    //! \brief Smoothed number of Agents carried by this segment.
    // -------------------------------------------------------------------------
    float flow() const { return m_flow; }
    void setFlow(float flow) { m_flow = (flow < 0.0f) ? 0.0f : flow; }

    // -------------------------------------------------------------------------
    //! \brief Number of Agents currently on this segment.
    // -------------------------------------------------------------------------
    uint32_t agentCount() const { return m_agentCount; }

    // -------------------------------------------------------------------------
    //! \brief Practical capacity of the segment, from its type.
    // -------------------------------------------------------------------------
    float capacity() const { return m_type.capacity; }

    // -------------------------------------------------------------------------
    //! \brief Called by Agent when it starts and stops travelling on this
    //! segment.
    // -------------------------------------------------------------------------
    void addAgent();
    void removeAgent();

    // -------------------------------------------------------------------------
    //! \brief Move the smoothed flow one step towards the instantaneous agent
    //! count, following the method of successive averages of CiudadSim:
    //!
    //!     F <- (1 - alpha) * F + alpha * Y
    //!
    //! Called once per tick. Without this smoothing, routing on the
    //! instantaneous count makes the whole population swing from one itinerary
    //! to the other and back, the classic oscillation of day-to-day traffic
    //! dynamics.
    //!
    //! \param[in] alpha: the step, in ]0..1]. Small values damp harder.
    // -------------------------------------------------------------------------
    void smoothFlow(float alpha);

private:

    // -------------------------------------------------------------------------
    //! \brief Compute the length of the segment and its free flow travel time.
    // -------------------------------------------------------------------------
    void updateMagnitude();

private:

    //! \brief Unique identifier.
    uint32_t           m_id;
    //! \brief Reference to the type of Way.
    WayType const&     m_type;
    //! \brief Node of origin.
    Node              *m_from = nullptr;
    //! \brief Node of destination.
    Node              *m_to = nullptr;
    //! \brief Cache the computation of the segment length.
    float              m_magnitude = 0.0f;
    //! \brief Cache of the travel time at zero flow.
    float              m_t0 = 0.0f;
    //! \brief Number of Agents currently travelling on this segment.
    uint32_t           m_agentCount = 0u;
    //! \brief Time averaged agent count, the flow fed to the BPR function.
    float              m_flow = 0.0f;
    //! \brief Units sitting on this segment rather than on a Node.
    std::vector<Unit*> m_units;
};

using WayPtr = std::unique_ptr<Way>;
using Ways = std::deque<WayPtr>;

// =============================================================================
//! \brief Is a Graph, typically player created, holding nodes (Node) and arcs
//! (Way). Ways, connecting Nodes, make up Path sets in which Agent can carry
//! Resources along from an Unit to another Unit. Example of Paths: Dirt roads,
//! highway, one-way road, power lines, water pipes, flight paths ...
// =============================================================================
class Path
{
public:

    // -------------------------------------------------------------------------
    //! \brief Empty Path: no nodes, no arcs.
    //! \param[in] type: const reference of a given type of Path also referred
    //! internally. The referred instance shall not be deleted before this Path
    //! instance is destroyed.
    // -------------------------------------------------------------------------
    Path(PathType const& type);

    // -------------------------------------------------------------------------
    //! \brief Create and store a new node given its world position.
    //! \return the newly created node.
    // -------------------------------------------------------------------------
    Node& addNode(Vector3f const& position);

    // -------------------------------------------------------------------------
    //! \brief Recreate a node with the identifier it had before being removed.
    //! Undoing an edit has to give back the identifier, since that is what the
    //! commands stacked on top of it refer to.
    //! \return the newly created node, or the existing one if the identifier is
    //! still in use.
    // -------------------------------------------------------------------------
    Node& addNode(uint32_t id, Vector3f const& position);

    // -------------------------------------------------------------------------
    //! \brief Find a node by identifier, or nullptr when it no longer exists.
    // -------------------------------------------------------------------------
    Node* node(uint32_t id);

    // -------------------------------------------------------------------------
    //! \brief Destroy a node and every segment it is an extremity of.
    //! \note Units sitting on the node and Agents travelling towards it keep a
    //! reference to it, so the caller has to get rid of them first. City does
    //! that in City::removeNode.
    // -------------------------------------------------------------------------
    void removeNode(Node& node);

    // -------------------------------------------------------------------------
    //! \brief Create and store a new segment given two existing nodes.
    //! \return the newly created segment.
    // -------------------------------------------------------------------------
    Way& addWay(WayType const& type, Node& p1, Node& p2);

    // -------------------------------------------------------------------------
    //! \brief Recreate a segment with the identifier it had before being
    //! removed. See addNode(uint32_t, Vector3f const&).
    // -------------------------------------------------------------------------
    Way& addWay(uint32_t id, WayType const& type, Node& p1, Node& p2);

    // -------------------------------------------------------------------------
    //! \brief Find a segment by identifier, or nullptr when it no longer
    //! exists.
    // -------------------------------------------------------------------------
    Way* way(uint32_t id);

    // -------------------------------------------------------------------------
    //! \brief Destroy a segment and detach it from its two extremities. The
    //! nodes are kept, even when they become orphan: the player may well be
    //! about to reconnect them.
    //! \note Agents travelling on the segment keep a reference to it, so the
    //! caller has to get rid of them first. City does that in City::removeWay.
    // -------------------------------------------------------------------------
    void removeWay(Way& way);

    // -------------------------------------------------------------------------
    //! \brief Split a segment into two sub arcs linked by a newly created
    //! node (except if the offset is set to one of the segment extremity)
    //!
    //! \param segment: the segment to split.
    //! \param offset: [0..1] the normalized length from from where to split
    //! the segment.
    //! \return the newly created position if offset = ]0..1[ or return the
    //! segment vertex if offset is 0 or 1.
    // -------------------------------------------------------------------------
    Node& splitWay(Way& segment, float offset);

    // -------------------------------------------------------------------------
    //! \brief Translate Node positions of a given direction.
    // -------------------------------------------------------------------------
    void translate(Vector3f const direction);

    // -------------------------------------------------------------------------
    //! \brief Getter: return the type of Path.
    // -------------------------------------------------------------------------
    std::string const& type() const { return m_type.name; }
    PathType const& pathType() const { return m_type; }

    // -------------------------------------------------------------------------
    //! \brief Return the list of nodes.
    // -------------------------------------------------------------------------
    Nodes const& nodes() const { return m_nodes; }

    // -------------------------------------------------------------------------
    //! \brief Return the list of Ways.
    // -------------------------------------------------------------------------
    Ways const& ways() const { return m_ways; }

    // -------------------------------------------------------------------------
    //! \brief Highest free flow speed among the Ways of this graph. The router
    //! needs it to turn a distance into a lower bound of a travel time.
    // -------------------------------------------------------------------------
    float maxFreeFlowSpeed() const { return m_maxFreeFlowSpeed; }

    // -------------------------------------------------------------------------
    //! \brief Advance the time averaged flow of every Way of this graph by one
    //! step. See Way::smoothFlow.
    // -------------------------------------------------------------------------
    void smoothFlows(float alpha);

private:

    // -------------------------------------------------------------------------
    //! \brief Recompute the cache read by maxFreeFlowSpeed() after a removal,
    //! since a removal can only lower it.
    // -------------------------------------------------------------------------
    void updateMaxFreeFlowSpeed();

private:

    PathType const& m_type;
    //! \brief Hold nodes. Do not use vector<> to avoid references to be
    //! invalidated.
    Nodes           m_nodes;
    //! \brief Hold arcs. Do not use vector<> to avoid references to be
    //! invalidated.
    Ways           m_ways;
    //! \brief
    uint32_t       m_nextNodeId = 0u;
    //! \brief
    uint32_t       m_nextWayId = 0u;
    //! \brief Cache of the highest free flow speed among m_ways.
    float          m_maxFreeFlowSpeed = 1.0f;
};

using Paths = std::map<std::string, std::unique_ptr<Path>>;

} // namespace ogb

#endif
