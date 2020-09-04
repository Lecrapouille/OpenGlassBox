#ifndef PATH_HPP
#  define PATH_HPP

#  include "Unit.hpp"
#  include "Vector.hpp"
#  include <vector>
#  include <cstdint>
#  include <string>

class Node;
class Segment;
class Path;
class Unit;
class Box;

// =============================================================================
//! \brief Class defining the extrimity of a Segment constituing a Path. This
//! class is not a basic structure holding a vertex position but it holds other
//! information such as which Path it belongs to, the list of Segments (graph
//! arcs) and Units refering to it (needed for Agents).
// =============================================================================
class Node
{
    friend Segment;
    friend Path;

public:

    // -------------------------------------------------------------------------
    //! \brief Uninitialized internal states. Only friend classes can then
    //! initialize fileds.
    // -------------------------------------------------------------------------
    Node() = default;

    // -------------------------------------------------------------------------
    //! \brief Initialized internal states.
    //! \param id: a unique id used to reference the node.
    //! \param path: the Path owning this node.
    //! \param position: the world position.
    // -------------------------------------------------------------------------
    Node(uint32_t id, Vector3f const& position, Path& path);

    // -------------------------------------------------------------------------
    //! \brief Constructor by copy.
    // -------------------------------------------------------------------------
    Node(Node const&) = default;

    // -------------------------------------------------------------------------
    //! \brief Attach a Unit owning this node.
    // -------------------------------------------------------------------------
    void addUnit(Unit& unit);

    // -------------------------------------------------------------------------
    //! \brief A Maps are simple uniform size grids. This method return the
    //! indices on the map (maps have all the same pavement).
    // -------------------------------------------------------------------------
    void getMapPosition(int32_t& u, int32_t& v);

    // -------------------------------------------------------------------------
    //! \brief Return the segment which the given node belongs to.
    //! \param node: the neighbor node.
    //! \return the address of the segment where extremity points are node and
    //! this instance. Return nullptr if the node was not a neighbor.
    // -------------------------------------------------------------------------
    Segment* getSegmentToNode(Node& node);

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    uint32_t id() const { return m_id; }

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    Vector3f& position() { return m_position; }

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    Path& path() { return *m_path; }

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    std::vector<Unit*>& units() { return m_units; }

private:

    //! \brief Unique identifier.
    uint32_t              m_id;
    //! \brief
    uint32_t             m_color;
    //! \brief World position.
    Vector3f              m_position;
    //! \brief Path owning this node instance.
    Path                 *m_path = nullptr;
    //! \brief Segments owning this node instance.
    std::vector<Segment*> m_segments;
    //! \brief Units owning this node instance.
    std::vector<Unit*>    m_units;
};

// =============================================================================
//! \brief Class defining the segment of a Path. A segment is defined by two
//! Nodes and the Path owning it. Segment is the locomotion for Agents carrying
//! Resources.
// =============================================================================
class Segment
{
    friend Node;
    friend Path;

public:

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    Segment(Path& path, uint32_t id, Node& node1, Node& node2);

    // -------------------------------------------------------------------------
    //! \brief Used by Path::splitSegment()
    // -------------------------------------------------------------------------
    void changeNode2(Node& newNode2);

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    float length();

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    uint32_t id() const { return m_id; }

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    Node const& node1() const { return *m_node1; }

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    Node const& node2() const { return *m_node2; }

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    Vector3f const& position1() const { return m_node1->position(); }

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    Vector3f const& position2() const { return m_node2->position(); }

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    Path& path() { return *m_path; }

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    float length() const { return m_length; }

private:

    void updateLength();

private:

    //! \brief
    uint32_t             m_id;
    //! \brief
    uint32_t             m_color;
    //! \brief
    Node                *m_node1 = nullptr;
    //! \brief
    Node                *m_node2 = nullptr;
    //! \brief
    Path                *m_path = nullptr;
    //! \brief
    float                m_length = 0.0f;
};

// =============================================================================
//! \brief Nodes connected by Segments make up Paths make up Path Sets.
//! Curvy roads, power lines, water pipes, flight paths ...
//! Typically player created.
// =============================================================================
class Path
{
public:

    // -------------------------------------------------------------------------
    //! \brief Empty Path: no nodes, no segments.
    //! \param
    // -------------------------------------------------------------------------
    Path(std::string const& id, Box& box);

    // -------------------------------------------------------------------------
    //! \brief Create and store a new node given its world position.
    //! \return the newly created node.
    // -------------------------------------------------------------------------
    Node& addNode(Vector3f const& position);

    //TODO void removeNode(Node& node);

    // -------------------------------------------------------------------------
    //! \brief Create and store a new segment given two existing nodes.
    //! \return the newly created segment.
    // -------------------------------------------------------------------------
    Segment& addSegment(Node& p1, Node& p2);

    //TODO void RemoveSegment(SimSegment segment)

    // -------------------------------------------------------------------------
    //! \brief Split a segment into two sub segments linked by a newly created
    //! node (execept if the offset is set to one of the segment extremity)
    //!
    //! \param segment: the segment to split.
    //! \param offset: [0..1] the normalized length from node1 where to split
    //! the segment.
    //! \return the newly created position if offset = ]0..1[ or return the
    //! segment vertex if offset is 0 or 1.
    // -------------------------------------------------------------------------
    Node& splitSegment(Segment& segment, float offset);

    // -------------------------------------------------------------------------
    //! \brief Return the unique identifier
    // -------------------------------------------------------------------------
    std::string const& id() const { return m_id; }

    // -------------------------------------------------------------------------
    //! \brief Return the owner box.
    // -------------------------------------------------------------------------
    Box& box() const { return m_box; }

    // -------------------------------------------------------------------------
    //! \brief Return the list of nodes.
    // -------------------------------------------------------------------------
    std::vector<Node> const& nodes() const { return m_nodes; }

    // -------------------------------------------------------------------------
    //! \brief Return the list of Segments.
    // -------------------------------------------------------------------------
    std::vector<Segment> const& segments() const { return m_segments; }

private:

    //! \brief
    std::string          m_id;
    //! \brief The reference to the Box owner of this Path instance.
    Box                 &m_box;
    //! \brief
    std::vector<Node>    m_nodes;
    //! \brief
    std::vector<Segment> m_segments;
    //! \brief
    uint32_t             m_nextNodeId = 0u;
    //! \brief
    uint32_t             m_nextSegmentId = 0u;
};

#endif
