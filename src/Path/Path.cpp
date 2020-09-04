#include "Path.hpp"
#include "Map.hpp"
#include <algorithm>

#define GRID_SIZE 100.0f

Node::Node(Path& path, uint32_t id, Vector3f const& position)
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
    float y = worldPos.z / Map::GRID_SIZE;

    if (x < 0.0f)
        u = 0u;
    else if (x >= m_path->box.gridSizeX)
        u = m_path->box.gridSizeX - 1u;
    else
        u = uint32_t(x);

    if (y < 0.0f)
        v = 0u;
    else if (y >= m_path->box.gridSizeY)
        v = m_path->box.gridSizeY - 1u;
    else
        y = uint32_t(y);
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

Segment::Segment(Path& path, uint32_t id, Node& node1, Node& node2)
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
    std::remove(m_node2->m_segments.begin(),
                m_node2->m_segments.end(),
                this);
    m_node2 = &newNode2;
    m_node2->m_segments.push_back(this);
    updateLength();
}

Path::Path(std::string const& id, Box& box)
    : m_id(id),
      m_box(box)
{}

Node& Path::addNode(Vector3f const& position)
{
    m_nodes.push_back(Node(*this, m_nextNodeId++, position));
    return m_nodes.back();
}

Segment& Path::addSegment(Node& p1, Node& p2)
{
    m_segments.push_back(Segment(*this, m_nextSegmentId++, p1, p2));
    return m_segments.back();
}

Node& Path::splitSegment(Segment& segment, float offset)
{
    if (offset <= 0.0f)
        return segment.node1();
    else if (offset >= 1.0f)
        return segment.node2();

    Vector3f wordPosition = segment.position1()
           + (segment.position2() - segment.position1()) * offset;
    Node& newNode = addNode(wordPosition);

    addSegment(segment.id, newNode, segment.node2());
    segment.changeNode2(newNode);

    return newNode;
}
