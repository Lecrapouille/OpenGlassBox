#include "Core/Path.hpp"
#include "Core/Config.hpp"
#include "Core/Unit.hpp"

// =============================================================================
// NODE
// =============================================================================

// -----------------------------------------------------------------------------
Node::Node(uint32_t id, Vector3f const& position)
    : m_id(id), m_position(position)
{}

// -----------------------------------------------------------------------------
void Node::addUnit(Unit& unit)
{
    m_units.push_back(&unit);
}

// -----------------------------------------------------------------------------
void Node::getMapPosition(uint32_t gridSizeU, uint32_t gridSizeV, uint32_t& u, uint32_t& v)
{
    uint32_t x = uint32_t(m_position.x / config::GRID_SIZE);
    uint32_t y = uint32_t(m_position.y / config::GRID_SIZE);

    if (m_position.x <= 0.0f)
        u = 0u;
    else if (x >= gridSizeU)
        u = gridSizeU - 1u;
    else
        u = x;

    if (m_position.y <= 0.0f)
        v = 0u;
    else if (y >= gridSizeV)
        v = gridSizeV - 1u;
    else
        v = y;
}

// -----------------------------------------------------------------------------
void Node::setPosition(Vector3f const position)
{
    m_position = position;
    for (auto& it: m_ways)
    {
        it->updateLength();
    }
}

// -----------------------------------------------------------------------------
// FIXME: raw pointer from unique_ptr
Way* Node::getWayToNode(Node const& node)
{
    size_t i = m_ways.size();
    while (i--)
    {
        if (((m_ways[i]->m_from == &node) && (m_ways[i]->m_to == this)) ||
            ((m_ways[i]->m_to == &node) && (m_ways[i]->m_from == this)))
        {
            return m_ways[i];
        }
    }

    return nullptr;
}

// =============================================================================
// SEGMENT
// =============================================================================

// -----------------------------------------------------------------------------
Way::Way(uint32_t id, WayType const& type, Node& node1, Node& node2)
    : m_id(id),
      m_type(type),
      m_from(&node1),
      m_to(&node2)
{
    m_from->m_ways.push_back(this);
    m_to->m_ways.push_back(this);
    updateLength();
}

// -----------------------------------------------------------------------------
void Way::updateLength()
{
    m_length = (m_to->position() - m_from->position()).length();
}

// -----------------------------------------------------------------------------
void Way::changeNode2(Node& newNode2)
{
    auto& segs = m_to->m_ways;
    segs.erase(std::remove(segs.begin(), segs.end(), this));
    m_to = &newNode2;
    m_to->m_ways.push_back(this);
    updateLength();
}

// =============================================================================
// PATH
// =============================================================================

// -----------------------------------------------------------------------------
Path::Path(PathType const& type)
    : m_type(type)
{}

// -----------------------------------------------------------------------------
Node& Path::addNode(Vector3f const& position)
{
    m_nodes.push_back(std::make_unique<Node>(m_nextNodeId++, position/*, *this*/));
    return *m_nodes.back();
}

// -----------------------------------------------------------------------------
// TODO: replace existing segment or allow multi-graph (== speedway)
Way& Path::addWay(WayType const& type, Node& p1, Node& p2)
{
    m_ways.push_back(std::make_unique<Way>(m_nextWayId++, type, p1, p2/*, *this*/));
    return *m_ways.back();
}

// -----------------------------------------------------------------------------
Node& Path::splitWay(Way& segment, float offset)
{
    if (offset <= 0.0f)
        return segment.from();
    else if (offset >= 1.0f)
        return segment.to();

    Vector3f wordPosition = segment.position1()
           + (segment.position2() - segment.position1()) * offset;
    Node& newNode = addNode(wordPosition);

    addWay(segment.m_type, newNode, segment.to());
    segment.changeNode2(newNode);

    return newNode;
}
