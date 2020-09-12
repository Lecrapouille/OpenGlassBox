#include "Core/Path.hpp"
#include "Core/Config.hpp"
//#include "Core/City.hpp"
#include "Core/Unit.hpp"
#include "Core/Unique.hpp"
#include <algorithm>

Node::Node(uint32_t id, Vector3f const& position)
    : m_id(id),
      m_position(position)
{}

void Node::addUnit(Unit& unit)
{
    m_units.push_back(&unit);
}

void Node::getMapPosition(uint32_t gridSizeX, uint32_t gridSizeY, uint32_t& u, uint32_t& v)
{
    uint32_t x = uint32_t(m_position.x / config::GRID_SIZE);
    uint32_t y = uint32_t(m_position.y / config::GRID_SIZE);

    if (m_position.x <= 0.0f)
        u = 0u;
    else if (x >= gridSizeX)
        u = gridSizeX - 1u;
    else
        u = x;

    if (m_position.y <= 0.0f)
        v = 0u;
    else if (y >= gridSizeY)
        v = gridSizeY - 1u;
    else
        v = y;
}

// FIXME: raw pointer from unique_ptr
Segment* Node::getSegmentToNode(Node& node)
{
    size_t i = m_segments.size();
    while (i--)
    {
        if (((m_segments[i]->m_node1 == &node) && (m_segments[i]->m_node2 == this)) ||
            ((m_segments[i]->m_node2 == &node) && (m_segments[i]->m_node1 == this)))
        {
            return m_segments[i];
        }
    }

    return nullptr;
}

Segment::Segment(uint32_t id, Node& node1, Node& node2)
    : m_id(id),
      m_node1(&node1),
      m_node2(&node2)
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

Path::Path(std::string const& id)
    : m_id(id)
{}

Node& Path::addNode(Vector3f const& position)
{
    m_nodes.push_back(std::make_unique<Node>(m_nextNodeId++, position/*, *this*/));
    return *m_nodes.back();
}

// TODO: replace existing segment or allow multi-graph (== speedway)
Segment& Path::addSegment(Node& p1, Node& p2)
{
    m_segments.push_back(std::make_unique<Segment>(m_nextSegmentId++, p1, p2/*, *this*/));
    return *m_segments.back();
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

    addSegment(newNode, segment.node2());
    segment.changeNode2(newNode);

    return newNode;
}
