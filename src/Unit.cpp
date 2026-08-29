//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "OpenGlassBox/Unit.hpp"
#include "OpenGlassBox/City.hpp"
#include "OpenGlassBox/World.hpp"
#include <algorithm>

// -----------------------------------------------------------------------------
namespace ogb
{

void Unit::bind(City& city)
{
    m_context.unit = this;
    m_context.city = &city;
    m_context.globals = &(city.getGlobals());
    m_context.locals = &m_resources;
    m_context.radius = m_type.radius;
    m_context.clock = &city.getClock();
    m_context.cell = city.worldToCell(m_position);
}

// -----------------------------------------------------------------------------
Unit::Unit(UnitType const& type, Node& node, City& city)
    : Entity(0u, type, node.getPosition()),
      m_node(&node),
      m_resources(type.resources)
{
    m_node->addUnit(*this);
    bind(city);
}

// -----------------------------------------------------------------------------
Unit::Unit(UnitType const& type, Segment& segment, float offset, City& city)
    : Entity(0u, type, {}),
      m_segment(&segment),
      m_offset(offset),
      m_resources(type.resources)
{
    if (m_offset < 0.0f)
        m_offset = 0.0f;
    else if (m_offset > 1.0f)
        m_offset = 1.0f;

    m_position = m_segment->getPositionAt(m_offset);
    m_segment->addUnit(*this);
    bind(city);
}

// -----------------------------------------------------------------------------
Unit::Unit(UnitType const& type, Vector3f const& position, City& city)
    : Entity(0u, type, position), m_resources(type.resources)
{
    bind(city);
}

// -----------------------------------------------------------------------------
void Unit::spreadRuleStart()
{
    if (m_context.city == nullptr)
        return;

    uint32_t const perMinute =
        (m_context.clock != nullptr) ? m_context.clock->getTicksPerMinute() : 1u;
    uint32_t const spread = std::max(1u, perMinute * 60u);

    // A cheap integer hash, so that consecutive identifiers do not come out as
    // consecutive phases.
    uint32_t hash = m_context.city->getConfig().randomSeed ^ (m_id * 2654435761u);
    hash ^= hash >> 15;
    hash *= 2246822519u;
    hash ^= hash >> 13;

    m_ticks = hash % spread;
}

// -----------------------------------------------------------------------------
Unit::~Unit()
{
    detach();
}

// -----------------------------------------------------------------------------
void Unit::detach()
{
    if (m_node != nullptr)
    {
        m_node->removeUnit(*this);
        m_node = nullptr;
    }
    if (m_segment != nullptr)
    {
        m_segment->removeUnit(*this);
        m_segment = nullptr;
    }
}

// -----------------------------------------------------------------------------
bool Unit::hasSegments() const
{
    if (m_node != nullptr)
        return m_node->hasSegments();
    return m_segment != nullptr;
}

// -----------------------------------------------------------------------------
Node* Unit::getAccessNode() const
{
    if (m_node != nullptr)
        return m_node;
    if (m_segment == nullptr)
        return nullptr;
    return (m_offset <= 0.5f) ? &m_segment->getFrom() : &m_segment->getTo();
}

// -----------------------------------------------------------------------------
Path* Unit::getPath() const
{
    if (m_node != nullptr)
        return m_node->getPath();
    if (m_segment != nullptr)
        return m_segment->getFrom().getPath();
    return nullptr;
}

// -----------------------------------------------------------------------------
void Unit::translate(Vector3f const& direction)
{
    // A Unit sitting on a Node or a Segment follows it: the City translates the
    // Path, which moves the Nodes, and the Unit reads the new position. One
    // that was given a footprint of its own keeps it and shifts with the City.
    if (m_placed)
        m_position += direction;
    else if (m_node != nullptr)
        m_position = m_node->getPosition();
    else if (m_segment != nullptr)
        m_position = m_segment->getPositionAt(m_offset);
    else
        m_position += direction;

    updateCell();
}

// -----------------------------------------------------------------------------
void Unit::setPosition(Vector3f const& position)
{
    m_position = position;
    m_placed = true;
    updateCell();
}

// -----------------------------------------------------------------------------
void Unit::moveOntoSegment(Segment& segment, float offset)
{
    detach();

    m_segment = &segment;
    m_offset = std::min(1.0f, std::max(0.0f, offset));
    m_segment->addUnit(*this);

    if (!m_placed)
    {
        m_position = m_segment->getPositionAt(m_offset);
        updateCell();
    }
}

// -----------------------------------------------------------------------------
void Unit::updateCell()
{
    if (m_context.city != nullptr)
        m_context.cell = m_context.city->worldToCell(m_position);
}

// -----------------------------------------------------------------------------
void Unit::executeRules()
{
    m_ticks += 1u;
    if (m_context.city != nullptr)
        m_context.clock = &m_context.city->getClock();

    uint32_t const perMinute =
        (m_context.clock != nullptr) ? m_context.clock->getTicksPerMinute() : 1u;

    size_t i = m_type.rules.size();
    while (i--)
    {
        if (m_ticks % m_type.rules[i]->getPeriodTicks(perMinute) == 0u)
        {
            m_type.rules[i]->execute(m_context);
        }
    }
}

// -----------------------------------------------------------------------------
OpeningHours Unit::getOpeningHours() const
{
    OpeningHours hours;
    for (RuleUnit const* rule : m_type.rules)
    {
        if (rule != nullptr)
            hours.add(*rule);
    }
    return hours;
}

// -----------------------------------------------------------------------------
bool Unit::accepts(Name const& searchTarget,
                   Resources const& resourcesToTryToAdd)
{
    // Asked of every building the router walks past, so the name test comes
    // first: it is four bytes against four bytes, whereas the room test walks
    // two lists of stocks.
    if (find(m_type.targets.begin(), m_type.targets.end(), searchTarget) ==
        m_type.targets.end())
        return false;

    return m_resources.canAddAny(resourcesToTryToAdd, m_inbound);
}

} // namespace ogb
