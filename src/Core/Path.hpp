#ifndef PATH_HPP
#  define PATH_HPP

#  include "Core/Types.hpp"
#  include "Core/Vector.hpp"
#  include "Core/Unique.hpp"
#  include <deque>
#  include <map>

class Way;
class Path;
class Unit;

// =============================================================================
//! \brief Class defining the extremity of arcs constituing a path. This
//! class can be seen as nodes of a graph (named Path). This class is not a
//! basic structure holding position but it holds more information such the list
//! of neighbor Ways (graph arcs) and Units (houses, buildings) refering to it.
//! Units consumn and output Agent carrying Resources along a Path and Node are
//! origin and destination for Agents.
// =============================================================================
class Node
{
    friend Way;
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
    //! \brief Helper fonction calling
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
    //! \brief Return the world position.
    // -------------------------------------------------------------------------
    Vector3f const& position() const { return m_position; }

    // -------------------------------------------------------------------------
    //! \brief Change the position of the Node.
    // -------------------------------------------------------------------------
    void setPosition(Vector3f const position);

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
    //! \brief Getter/Setter of node color (global color)
    // -------------------------------------------------------------------------
    uint32_t& color()
    {
        static uint32_t c = 0xAAAAAA;
        return c;
    }

private:

    // -------------------------------------------------------------------------
    //! \brief A Maps are simple uniform size grids. This method return the
    //! indices on the map (maps have all the same pavement).
    // -------------------------------------------------------------------------
    void getMapPosition(uint32_t gridSizeU, uint32_t gridSizeV, uint32_t& u, uint32_t& v);

private:

    //! \brief Unique identifier.
    uint32_t              m_id;
    //! \brief World position.
    Vector3f              m_position;
    //! \brief Ways owning this node instance.
    std::vector<Way*>     m_ways;
    //! \brief Units owning this node instance.
    std::vector<Unit*>    m_units;
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

    // -------------------------------------------------------------------------
    //! \brief Initialized the state of the Way.
    //! \param[in] id: unique identifier.
    //! \param[in] type: const reference of a given type of Way also refered
    //! internally. The refered instance shall not be deleted before this Way
    //! instance is destroyed.
    //! \param[in] from: The node of origin.
    //! \param[in] to: The node of destination.
    // -------------------------------------------------------------------------
    Way(uint32_t id, WayType const& type, Node& from, Node& to);

    // -------------------------------------------------------------------------
    //! \brief Used by Path::splitWay()
    // -------------------------------------------------------------------------
    void changeNode2(Node& newNode2);

    // -------------------------------------------------------------------------
    //! \brief Return the unique identifier.
    // -------------------------------------------------------------------------
    uint32_t id() const { return m_id; }

    // -------------------------------------------------------------------------
    //! \brief Return the origin node.
    // -------------------------------------------------------------------------
    Node& from() { return *m_from; }

    // -------------------------------------------------------------------------
    //! \brief Return the destination node.
    // -------------------------------------------------------------------------
    Node& to() { return *m_to; }

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
    float length() const { return m_length; }

    // -------------------------------------------------------------------------
    //! \brief Getter: return the type of Way.
    // -------------------------------------------------------------------------
    std::string const& type() const { return m_type.name; }

    // -------------------------------------------------------------------------
    //! \briefG etter: return the color of the Way.
    // -------------------------------------------------------------------------
    uint32_t color() const { return m_type.color; }

private:

    // -------------------------------------------------------------------------
    //! \brief Constructor that can only be called by friend class.
    // -------------------------------------------------------------------------
    Way() = default;

    // -------------------------------------------------------------------------
    //! \brief Compute the length of the segment.
    // -------------------------------------------------------------------------
    void updateLength();

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
    float              m_length = 0.0f;
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
    //! \param[in] type: const reference of a given type of Path also refered
    //! internally. The refered instance shall not be deleted before this Path
    //! instance is destroyed.
    // -------------------------------------------------------------------------
    Path(PathType const& type);

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
    Way& addWay(WayType const& type, Node& p1, Node& p2);

    //TODO void RemoveWay(SimWay segment)

    // -------------------------------------------------------------------------
    //! \brief Split a segment into two sub arcs linked by a newly created
    //! node (execept if the offset is set to one of the segment extremity)
    //!
    //! \param segment: the segment to split.
    //! \param offset: [0..1] the normalized length from from where to split
    //! the segment.
    //! \return the newly created position if offset = ]0..1[ or return the
    //! segment vertex if offset is 0 or 1.
    // -------------------------------------------------------------------------
    Node& splitWay(Way& segment, float offset);

    // -------------------------------------------------------------------------
    //! \brief Getter: return the type of Path.
    // -------------------------------------------------------------------------
    std::string const& type() const { return m_type.name; }

    // -------------------------------------------------------------------------
    //! \brief Return the list of nodes.
    // -------------------------------------------------------------------------
    Nodes const& nodes() const { return m_nodes; }

    // -------------------------------------------------------------------------
    //! \brief Return the list of Ways.
    // -------------------------------------------------------------------------
    Ways const& ways() const { return m_ways; }

private:

    PathType const& m_type;
    //! \brief Holde nodes. Do not use vector<> to avoid references to be
    //! invalidated.
    Nodes           m_nodes;
    //! \brief Hold arcs. Do not use vector<> to avoid references to be
    //! invalidated.
    Ways           m_ways;
    //! \brief
    uint32_t       m_nextNodeId = 0u;
    //! \brief
    uint32_t       m_nextWayId = 0u;
};

using Paths = std::map<std::string, std::unique_ptr<Path>>;

#endif
