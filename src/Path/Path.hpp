#ifndef PATH_HPP
#  define PATH_HPP

#  include "Resource.hpp"
#  include "Vector.hpp"
#  include <vector>
#  include <cstdint>
#  include <string>

class Node;
class Segment;
class Path;
class Unit;

class Node
{
    friend Segment;
    friend Path;

public:

    Node() = default;
    Node(Path& path, uint32_t id, Vector3f const& position);
    Node(Node const&) = default;
    void addUnit(Unit& unit);
    void getMapPosition(int32_t& x, int32_t& y);
    Segment* getSegmentToNode(Node& node);
    uint32_t id() const { return m_id; }
    Vector3f& position() { return m_position; }
    Path& path() { return *m_path; }
    std::vector<Unit*>& units() { return m_units; }

private:

    uint32_t              m_id;
    Vector3f              m_position;
    Path                 *m_path;
    std::vector<Segment*> m_segments;
    std::vector<Unit*>    m_units;
};

class Segment
{
    friend Node;
    friend Path;

public:

    Segment(Path& path, uint32_t id, Node& node1, Node& node2);
    void changePoint2(Node& newPoint2);
    float length();
    uint32_t id() const { return m_id; }
    Node& node1() { return *m_node1; }
    Node& node2() { return *m_node2; }
    Vector3f& position1() { return m_node1->position(); }
    Vector3f& position2() { return m_node2->position(); }
    Path& path() { return *m_path; }
    float length() const { return m_length; }

private:

    void updateLength();

private:

    uint32_t             m_id;
    uint32_t             m_color;
    Node                *m_node1 = nullptr;
    Node                *m_node2 = nullptr;
    Path                *m_path = nullptr;
    float                m_length = 0.0f;
};

class Path
{
public:

    Path(std::string const& id);
    Node& addNode(Vector3f const& position);
    Segment& addSegment(Node& p1, Node& p2);
    std::string const& id() const { return m_id; }

private:

    std::string          m_id;
    std::vector<Node>    m_nodes;
    std::vector<Segment> m_segments;
    uint32_t             m_nextNodeId = 0u;
    uint32_t             m_nextSegmentId = 0u;
};

#endif
