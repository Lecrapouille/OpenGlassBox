//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "OpenGlassBox/Path.hpp"
#include "OpenGlassBox/Unit.hpp"
#include <algorithm>
#include <cmath>

// =============================================================================
// NODE
// =============================================================================

// -----------------------------------------------------------------------------
namespace ogb
{

Node::Node(uint32_t id, Vector3f const& position)
    : m_id(id), m_position(position)
{
}

// -----------------------------------------------------------------------------
void Node::addUnit(Unit& unit)
{
    m_units.push_back(&unit);
}

// -----------------------------------------------------------------------------
void Node::removeUnit(Unit& unit)
{
    m_units.erase(std::remove(m_units.begin(), m_units.end(), &unit),
                  m_units.end());
}

// -----------------------------------------------------------------------------
void Node::translate(Vector3f const& direction)
{
    m_position += direction;
    for (auto& it : m_segments)
    {
        it->updateLength();
    }
}

// -----------------------------------------------------------------------------
// FIXME: raw pointer from unique_ptr
Segment* Node::findSegmentTo(Node const& node) const
{
    size_t i = m_segments.size();
    while (i--)
    {
        if (((m_segments[i]->m_from == &node) && (m_segments[i]->m_to == this)) ||
            ((m_segments[i]->m_to == &node) && (m_segments[i]->m_from == this)))
        {
            return m_segments[i];
        }
    }

    return nullptr;
}

// =============================================================================
// SEGMENT
// =============================================================================

// -----------------------------------------------------------------------------
Segment::Segment(uint32_t id, SegmentType const& type, Node& node1, Node& node2)
    : m_id(id), m_type(type), m_from(&node1), m_to(&node2)
{
    m_from->m_segments.push_back(this);
    m_to->m_segments.push_back(this);
    updateLength();
}

// -----------------------------------------------------------------------------
void Segment::updateLength()
{
    m_length = ogb::length(m_to->getPosition() - m_from->getPosition());

    // A type with a nonsensical speed would give an infinite or negative travel
    // time and poison the whole routing, so fall back on the length.
    m_t0 = (m_type.speed > 0.0f) ? (m_length / m_type.speed) : m_length;

    updateTravelTime();
}

// -----------------------------------------------------------------------------
void Segment::updateTravelTime()
{
    if (m_type.capacity <= 0.0f)
    {
        m_travelTime = m_t0;
        return;
    }

    float const x = m_flow / m_type.capacity;

    m_travelTime = m_t0 * (1.0f + 0.15f * std::pow(x, m_type.beta));
}

// -----------------------------------------------------------------------------
Vector3f Segment::getPositionAt(float offset) const
{
    return getFromPosition() +
           (getToPosition() - getFromPosition()) * offset;
}

// -----------------------------------------------------------------------------
void Segment::addUnit(Unit& unit)
{
    m_units.push_back(&unit);
}

// -----------------------------------------------------------------------------
void Segment::removeUnit(Unit& unit)
{
    m_units.erase(std::remove(m_units.begin(), m_units.end(), &unit),
                  m_units.end());
}

// -----------------------------------------------------------------------------
void Segment::addAgent()
{
    ++m_agentCount;
}

// -----------------------------------------------------------------------------
void Segment::removeAgent()
{
    if (m_agentCount > 0u)
    {
        --m_agentCount;
    }
}

// -----------------------------------------------------------------------------
void Segment::smoothFlow(float alpha)
{
    if (alpha > 1.0f)
    {
        alpha = 1.0f;
    }

    float const flow =
        (alpha <= 0.0f)
            ? float(m_agentCount)
            : ((1.0f - alpha) * m_flow + alpha * float(m_agentCount));

    // An empty street stays empty, and a city has far more of those than it
    // has busy ones. Skipping those spares the power the BPR function would
    // otherwise evaluate on every segment of the network on every tick.
    //
    // The threshold is on the change rather than on equality: the smoothing of
    // a street settling towards its traffic converges without ever reaching
    // it, and a hundredth of a vehicle moves no travel time worth recomputing.
    if (std::fabs(flow - m_flow) < FLOW_EPSILON)
        return;

    m_flow = flow;
    updateTravelTime();
}

// =============================================================================
// PATH
// =============================================================================

// -----------------------------------------------------------------------------
Path::Path(PathType const& type) : m_type(type) {}

// -----------------------------------------------------------------------------
Node& Path::addNode(Vector3f const& position)
{
    return addNode(m_nextNodeId, position);
}

// -----------------------------------------------------------------------------
Node& Path::addNode(uint32_t id, Vector3f const& position)
{
    Node* existing = findNode(id);
    if (existing != nullptr)
        return *existing;

    m_nodes.push_back(std::make_unique<Node>(id, position));
    m_nodes.back()->m_path = this;
    m_nodes.back()->m_index = uint32_t(m_nodes.size() - 1u);

    // Identifiers handed out from now on must not collide with the one just
    // reused, otherwise two nodes would answer to the same reference.
    if (id >= m_nextNodeId)
    {
        m_nextNodeId = id + 1u;
    }

    return *m_nodes.back();
}

// -----------------------------------------------------------------------------
Node* Path::findNode(uint32_t id) const
{
    for (auto const& it : m_nodes)
    {
        if (it->getId() == id)
            return it.get();
    }

    return nullptr;
}

// -----------------------------------------------------------------------------
// TODO: replace existing segment or allow multi-graph (== speedway)
Segment& Path::addSegment(SegmentType const& type, Node& p1, Node& p2)
{
    return addSegment(m_nextSegmentId, type, p1, p2);
}

// -----------------------------------------------------------------------------
Segment& Path::addSegment(uint32_t id, SegmentType const& type, Node& p1, Node& p2)
{
    Segment* existing = findSegment(id);
    if (existing != nullptr)
        return *existing;

    m_segments.push_back(std::make_unique<Segment>(id, type, p1, p2 /*, *this*/));
    m_maxFreeFlowSpeed = std::max(m_maxFreeFlowSpeed, type.speed);

    if (id >= m_nextSegmentId)
    {
        m_nextSegmentId = id + 1u;
    }

    return *m_segments.back();
}

// -----------------------------------------------------------------------------
Segment* Path::findSegment(uint32_t id) const
{
    for (auto const& it : m_segments)
    {
        if (it->getId() == id)
            return it.get();
    }

    return nullptr;
}

// -----------------------------------------------------------------------------
void Path::removeSegment(Segment& segment)
{
    auto detach = [&segment](std::vector<Segment*>& segments)
    { segments.erase(std::remove(segments.begin(), segments.end(), &segment), segments.end()); };
    detach(segment.m_from->m_segments);
    detach(segment.m_to->m_segments);

    m_segments.erase(std::remove_if(m_segments.begin(),
                                m_segments.end(),
                                [&segment](SegmentPtr const& it)
                                { return it.get() == &segment; }),
                 m_segments.end());

    updateMaxFreeFlowSpeed();
}

// -----------------------------------------------------------------------------
void Path::removeNode(Node& node)
{
    // Removing a segment mutates the very vector being walked, so keep taking
    // the first one until none is left.
    while (!node.m_segments.empty())
    {
        removeSegment(*node.m_segments.front());
    }

    m_nodes.erase(std::remove_if(m_nodes.begin(),
                                 m_nodes.end(),
                                 [&node](NodePtr const& it)
                                 { return it.get() == &node; }),
                  m_nodes.end());

    reindexNodes();
}

// -----------------------------------------------------------------------------
void Path::reindexNodes()
{
    // Linear, but demolishing a crossroads is something a player does, not
    // something a tick does, and the routers rely on the indices staying dense.
    uint32_t index = 0u;
    for (auto const& it : m_nodes)
    {
        it->m_index = index++;
    }
}

// -----------------------------------------------------------------------------
void Path::clear()
{
    // Segments first: they point at the nodes.
    m_segments.clear();
    m_nodes.clear();
    m_nextNodeId = 0u;
    m_nextSegmentId = 0u;
    m_maxFreeFlowSpeed = 1.0f;
}

// -----------------------------------------------------------------------------
void Path::updateMaxFreeFlowSpeed()
{
    // One is the floor rather than zero: the router divides a distance by this
    // speed, and an empty graph would otherwise give an infinite lower bound.
    float speed = 1.0f;
    for (auto const& it : m_segments)
    {
        speed = std::max(speed, it->m_type.speed);
    }

    m_maxFreeFlowSpeed = speed;
}

// -----------------------------------------------------------------------------
void Path::updateTrafficSmoothing(float alpha)
{
    for (auto const& segment : m_segments)
    {
        segment->smoothFlow(alpha);
    }
}

// -----------------------------------------------------------------------------
Node& Path::splitSegment(Segment& segment, float offset)
{
    if (offset <= 0.0f)
        return segment.getFrom();
    else if (offset >= 1.0f)
        return segment.getTo();

    Vector3f const cut = segment.getPositionAt(offset);

    Node& newNode = addNode(cut);
    addSegment(segment.m_type, newNode, segment.getTo());

    auto& segs = segment.m_to->m_segments;
    segs.erase(std::remove(segs.begin(), segs.end(), &segment));
    segment.m_to = &newNode;
    segment.m_to->m_segments.push_back(&segment);
    segment.updateLength();

    return newNode;
}

// -----------------------------------------------------------------------------
void Path::translate(Vector3f const& direction)
{
    for (auto const& it : m_nodes)
    {
        it->m_position += direction;
    }

    for (auto const& it : m_segments)
    {
        it->updateLength();
    }
}

} // namespace ogb
