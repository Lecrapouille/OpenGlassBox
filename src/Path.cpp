#include "Path.hpp"
#include "Map.hpp"
#include "City.hpp"
#include <algorithm>

Node::Node(uint32_t id, Vector3f const& position, Path& path)
    : m_id(id),
      m_position(position),
      m_path(&path)
{}

void Node::addUnit(Unit& unit)
{
    m_units.push_back(&unit);
}

void Node::getMapPosition(uint32_t& u, uint32_t& v)
{
    Vector3f worldPos = m_position;

    float x = worldPos.x / Map::GRID_SIZE;
    float y = worldPos.y / Map::GRID_SIZE;

    if (x < 0.0f)
        u = 0u;
    else if (uint32_t(x) >= m_path->city().gridSizeX())
        u = m_path->city().gridSizeX() - 1u;
    else
        u = uint32_t(x);

    if (y < 0.0f)
        v = 0u;
    else if (uint32_t(y) >= m_path->city().gridSizeY())
        v = m_path->city().gridSizeY() - 1u;
    else
        v = uint32_t(y);
}

Segment* Node::getSegmentToNode(Node& node)
{
    size_t i = m_segments.size();
    while (i--)
    {
        if ((m_segments[i]->m_node1 == &node) ||
            (m_segments[i]->m_node2 == &node))
        {
            return m_segments[i];
        }
    }

    return nullptr;
}

Segment::Segment(uint32_t id, Node& node1, Node& node2, Path& path)
    : m_id(id),
      m_node1(&node1),
      m_node2(&node2),
      m_path(&path)
{
    m_node1->m_segments.push_back(this);
    m_node2->m_segments.push_back(this);
    updateLength();
}

void Segment::updateLength()
{
    m_length = (m_node2->position() - m_node1->position()).length();
}

void Segment::changeNode2(Node& newNode2)
{
    auto& segs = m_node2->m_segments;
    segs.erase(std::remove(segs.begin(), segs.end(), this));
    m_node2 = &newNode2;
    m_node2->m_segments.push_back(this);
    updateLength();
}

Path::Path(std::string const& id, City& city)
    : m_id(id),
      m_city(city)
{}

// FIXME: can be invalidated
Node& Path::addNode(Vector3f const& position)
{
    m_nodes.push_back(Node(m_nextNodeId++, position, *this));
    return m_nodes.back();
}

// FIXME: can be invalidated
Segment& Path::addSegment(Node& p1, Node& p2)
{
    m_segments.push_back(Segment(m_nextSegmentId++, p1, p2, *this));
    return m_segments.back();
}

// FIXME: can be invalidated
Node& Path::splitSegment(Segment& segment, float offset)
{
    if (offset <= 0.0f)
        return segment.node1();
    else if (offset >= 1.0f)
        return segment.node2();

    Vector3f wordPosition = segment.position1()
           + (segment.position2() - segment.position1()) * offset;
    Node& newNode = addNode(wordPosition);

    addSegment(newNode, segment.node2());
    segment.changeNode2(newNode);

    return newNode;
}
