//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "OpenGlassBox/Building.hpp"
#include "OpenGlassBox/City.hpp"
#include "OpenGlassBox/World.hpp"
#include <algorithm>

// -----------------------------------------------------------------------------
namespace ogb
{

void Building::bind(City& city)
{
    m_context.building = this;
    m_context.city = &city;
    m_context.globals = &(city.getGlobals());
    m_context.locals = &m_resources;
    m_context.radius = m_type.radius;
    m_context.clock = &city.getClock();
    m_context.cell = city.worldToCell(m_position);
}

// -----------------------------------------------------------------------------
Building::Building(BuildingType const& type, Node& node, City& city)
    : Entity(0u, type, node.getPosition()),
      m_resources(type.resources),
      m_node(&node)
{
    m_node->addBuilding(*this);
    bind(city);
}

// -----------------------------------------------------------------------------
Building::Building(BuildingType const& type, Segment& segment, float offset, City& city)
    : Entity(0u, type, {}),
      m_resources(type.resources),
      m_segment(&segment),
      m_offset(offset)
{
    if (m_offset < 0.0f)
        m_offset = 0.0f;
    else if (m_offset > 1.0f)
        m_offset = 1.0f;

    m_position = m_segment->getPositionAt(m_offset);
    m_segment->addBuilding(*this);
    bind(city);
}

// -----------------------------------------------------------------------------
Building::Building(BuildingType const& type, Vector3f const& position, City& city)
    : Entity(0u, type, position), m_resources(type.resources)
{
    bind(city);
}

// -----------------------------------------------------------------------------
void Building::spreadRuleStart()
{
    if (m_context.city == nullptr)
        return;

    uint32_t const perMinute =
        (m_context.clock != nullptr) ? m_context.clock->getTicksPerMinute() : 1u;
    uint32_t const spread = std::max(1u, perMinute * 60u);

    // A cheap integer hash, so that consecutive identifiers do not come out as
    // consecutive phases. The id is folded down to 32 bits on purpose: the
    // mixing constants below are 32-bit ones, and a city never holds enough
    // buildings for the high half to carry anything.
    uint32_t hash = m_context.city->getConfig().randomSeed ^
                    (uint32_t(m_id) * 2654435761u);
    hash ^= hash >> 15;
    hash *= 2246822519u;
    hash ^= hash >> 13;

    m_ticks = hash % spread;
}

// -----------------------------------------------------------------------------
Building::~Building()
{
    detach();
}

// -----------------------------------------------------------------------------
void Building::detach()
{
    if (m_node != nullptr)
    {
        m_node->removeBuilding(*this);
        m_node = nullptr;
    }
    if (m_segment != nullptr)
    {
        m_segment->removeBuilding(*this);
        m_segment = nullptr;
    }
}

// -----------------------------------------------------------------------------
bool Building::hasSegments() const
{
    if (m_node != nullptr)
        return m_node->hasSegments();
    return m_segment != nullptr;
}

// -----------------------------------------------------------------------------
Node* Building::getAccessNode() const
{
    if (m_node != nullptr)
        return m_node;
    if (m_segment == nullptr)
        return nullptr;
    return (m_offset <= 0.5f) ? &m_segment->getFrom() : &m_segment->getTo();
}

// -----------------------------------------------------------------------------
Path* Building::getPath() const
{
    if (m_node != nullptr)
        return m_node->getPath();
    if (m_segment != nullptr)
        return m_segment->getFrom().getPath();
    return nullptr;
}

// -----------------------------------------------------------------------------
void Building::translate(Vector3f const& direction)
{
    // A Building sitting on a Node or a Segment follows it: the City translates the
    // Path, which moves the Nodes, and the Building reads the new position. One
    // that was given a footprint of its own keeps it and shifts with the City.
    if (m_placed || ((m_node == nullptr) && (m_segment == nullptr)))
    {
        m_position += direction;
        updateCell();
        return;
    }

    followAnchor();
}

// -----------------------------------------------------------------------------
void Building::followAnchor()
{
    if (m_placed)
        return;

    if (m_node != nullptr)
        m_position = m_node->getPosition();
    else if (m_segment != nullptr)
        m_position = m_segment->getPositionAt(m_offset);
    else
        return;

    updateCell();
}

// -----------------------------------------------------------------------------
void Building::setPosition(Vector3f const& position)
{
    m_position = position;
    m_placed = true;
    updateCell();
}

// -----------------------------------------------------------------------------
void Building::moveOntoSegment(Segment& segment, float offset)
{
    detach();

    m_segment = &segment;
    m_offset = std::min(1.0f, std::max(0.0f, offset));
    m_segment->addBuilding(*this);

    if (!m_placed)
    {
        m_position = m_segment->getPositionAt(m_offset);
        updateCell();
    }
}

// -----------------------------------------------------------------------------
void Building::updateCell()
{
    if (m_context.city != nullptr)
        m_context.cell = m_context.city->worldToCell(m_position);
}

// -----------------------------------------------------------------------------
void Building::executeRules()
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
OpeningHours Building::getOpeningHours() const
{
    OpeningHours hours;
    for (RuleBuilding const* rule : m_type.rules)
    {
        if (rule != nullptr)
            hours.add(*rule);
    }
    return hours;
}

// -----------------------------------------------------------------------------
bool Building::accepts(Name const& searchTarget,
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
