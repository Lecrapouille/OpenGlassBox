#include "Path.hpp"
#include "Unit.hpp"
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

void Node::getMapPosition(int32_t& x, int32_t& y)
{
    Vector3f worldPos = m_position;

    x = int32_t(worldPos.x / /*Map::*/GRID_SIZE);
    y = int32_t(worldPos.z / /*Map::*/GRID_SIZE);
    /*
    if (x < 0)
        x = 0;
    else if (x >= m_path->box.gridSizeX)
        x = m_path->box.gridSizeX - 1;

    if (y < 0)
        y = 0;
    else if (y >= m_path->box.gridSizeY)
        y = m_path->box.gridSizeY - 1;
    */
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

void Segment::changePoint2(Node& newPoint2)
{
    std::remove(m_node2->m_segments.begin(),
                m_node2->m_segments.end(),
                this);
    m_node2 = &newPoint2;
    m_node2->m_segments.push_back(this);
    updateLength();
}

Path::Path(std::string const& id)
    : m_id(id)
{}

Node& Path::addNode(Vector3f const& position)
{
    m_nodes.push_back(Node(*this, m_nextNodeId++, position));
    // pathListener.OnNodeAdded(this, m_nodes.back());
    return m_nodes.back();
}

Segment& Path::addSegment(Node& p1, Node& p2)
{
    m_segments.push_back(Segment(*this, m_nextSegmentId++, p1, p2));
    // pathListener.OnSegmentAdded(this, m_segments.back());
    return m_segments.back();
}
