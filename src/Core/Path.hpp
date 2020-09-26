#ifndef PATH_HPP
#  define PATH_HPP

#  include "Core/Vector.hpp"
#  include "Core/Unique.hpp"
#  include <deque>
#  include <vector>
#  include <cstdint>
#  include <string>
#  include <memory>
#  include <map>

class Node;
class Segment;
class Path;
class Unit;

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
    //! \param[in] id: a unique id used to reference the node.
    //! \param[in] position: the world position.
    // -------------------------------------------------------------------------
    Node(uint32_t id, Vector3f const& position/*, Path& path*/);

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
    void getMapPosition(uint32_t gridSizeU, uint32_t gridSizeV, uint32_t& u, uint32_t& v);

    // -------------------------------------------------------------------------
    //! \brief Helper fonction calling
    //! getMapPosition(uint32_t, uint32_t, uint32_t&, uint32_t&) from class T
    //! that implements gridSizeU() and gridSizeV() ie City, Map.
    // -------------------------------------------------------------------------
    template<class T>
    void getMapPosition(T const& x, uint32_t& u, uint32_t& v)
    {
        getMapPosition(x.gridSizeU(), x.gridSizeV(), u, v);
    }

    // -------------------------------------------------------------------------
    //! \brief Return the first segment in which the given node belongs to.
    //! \note to get the full list, call segments().
    //! \param[in] node: the neighbor node.
    //! \return the address of the segment where extremity points are node and
    //! this instance. Return nullptr if the node was not a neighbor.
    // -------------------------------------------------------------------------
    Segment* getSegmentToNode(Node const& node);

    // -------------------------------------------------------------------------
    //! \brief Return the unique identifier.
    // -------------------------------------------------------------------------
    uint32_t id() const { return m_id; }

    // -------------------------------------------------------------------------
    //! \brief Return the world position.
    // -------------------------------------------------------------------------
    Vector3f& position() { return m_position; }

    // -------------------------------------------------------------------------
    //! \brief Const getter of Segments hold by this instance.
    // -------------------------------------------------------------------------
    std::vector<Segment*> const& segments() const { return m_segments; }

    // -------------------------------------------------------------------------
    //! \brief Getter of Units hold by this instance.
    // -------------------------------------------------------------------------
    std::vector<Unit*>& units() { return m_units; }

    // -------------------------------------------------------------------------
    //! \brief Getter the nth Unit
    // -------------------------------------------------------------------------
    Unit& unit(uint32_t const nth) { return *m_units[nth]; }

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
    //! \brief Segments owning this node instance.
    std::vector<Segment*> m_segments;
    //! \brief Units owning this node instance.
    std::vector<Unit*>    m_units;
};

using NodePtr = std::unique_ptr<Node>;
using Nodes = std::deque<NodePtr>;

//==========================================================================
//! \brief Type of Segments (people, worker ...).
//! Class constructed during the parsing of simulation scripts.
//! Examples:
//!  - segment Dirt color 0xAAAAAA
//==========================================================================
class SegmentType
{
public:

    SegmentType(SegmentType const&) = default;

    SegmentType(std::string const& name, uint32_t color)
        : m_name(name), m_color(color)
    {}

    SegmentType(std::string const& name)
        : m_name(name), m_color(0xFFFFFF)
    {}

    std::string m_name;
    uint32_t    m_color;
};

// =============================================================================
//! \brief Class defining the segment of a Path. A segment is defined by two
//! Nodes and the Path owning it. Segment is the locomotion for Agents carrying
//! Resources.
// =============================================================================
class Segment: private SegmentType
{
    friend Node;
    friend Path;

public:

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    Segment(uint32_t id, SegmentType const& type, Node& node1, Node& node2);

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    Segment() = default;

    // -------------------------------------------------------------------------
    //! \brief Used by Path::splitSegment()
    // -------------------------------------------------------------------------
    void changeNode2(Node& newNode2);

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    uint32_t id() const { return m_id; }

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    Node& node1() { return *m_node1; }

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    Node& node2() { return *m_node2; }

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
    float length() const { return m_length; }

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    std::string const& name() const { return m_name; }

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    uint32_t color() const { return m_color; }

private:

    void updateLength();

private:

    //! \brief Unique identifier
    uint32_t             m_id;
    //! \brief First vertex
    Node                *m_node1 = nullptr;
    //! \brief Second vertex
    Node                *m_node2 = nullptr;
    //! \brief Cache the computation of the length
    float                m_length = 0.0f;
};

using SegmentPtr = std::unique_ptr<Segment>;
using Segments = std::deque<SegmentPtr>;

//==========================================================================
//! \brief Type of Segments (people, worker ...).
//! Class constructed during the parsing of simulation scripts.
//! Examples:
//!  - path Road color 0xAAAAAA
//==========================================================================
class PathType
{
public:

    PathType(PathType const&) = default;

    //PathType(std::string const& name, uint32_t color)
    //    : m_name(name), m_color(color)
    //{}

    PathType(std::string const& name)
        : m_name(name)
    {}

    std::string m_name;
    //uint32_t  m_color;
};

// =============================================================================
//! \brief Nodes connected by Segments make up Paths make up Path Sets.
//! Curvy roads, power lines, water pipes, flight paths ...
//! Typically player created.
// =============================================================================
class Path: private PathType
{
public:

    // -------------------------------------------------------------------------
    //! \brief Empty Path: no nodes, no segments.
    //! \param
    // -------------------------------------------------------------------------
    Path(PathType const& type/*, City& city*/);

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
    Segment& addSegment(SegmentType const& type, Node& p1, Node& p2);

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
    std::string const& name() const { return m_name; }

    // -------------------------------------------------------------------------
    //! \brief Return the owner city.
    // -------------------------------------------------------------------------
    //City& city() const { return m_city; }

    // -------------------------------------------------------------------------
    //! \brief Return the list of nodes.
    // -------------------------------------------------------------------------
    Nodes const& nodes() const { return m_nodes; }

    // -------------------------------------------------------------------------
    //! \brief Return the list of Segments.
    // -------------------------------------------------------------------------
    Segments const& segments() const { return m_segments; }

private:

    //! \brief The reference to the City owning this Path instance.
    //City &m_city;
    //! \brief Holde nodes. Do not use vector<> to avoid references to be
    //! invalidated.
    Nodes m_nodes;
    //! \brief Holde segments. Do not use vector<> to avoid references to be
    //! invalidated.
    Segments m_segments;
    //! \brief
    uint32_t m_nextNodeId = 0u;
    //! \brief
    uint32_t m_nextSegmentId = 0u;
};

using Paths = std::map<std::string, std::unique_ptr<Path>>;

#endif
