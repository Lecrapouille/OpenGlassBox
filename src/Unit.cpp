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
namespace ogb {

void Unit::bind(City& city)
{
    m_context.unit = this;
    m_context.city = &city;
    m_context.globals = &(city.globals());
    m_context.locals = &m_resources;
    m_context.radius = m_type.radius;
    m_context.clock = &city.world().clock();
    city.world2mapPosition(m_position, m_context.u, m_context.v);
}

// -----------------------------------------------------------------------------
Unit::Unit(UnitType const& type, Node& node, City& city)
    : m_type(type),
      m_position(node.position()),
      m_node(&node),
      m_resources(type.resources)
{
    m_node->addUnit(*this);
    bind(city);
}

// -----------------------------------------------------------------------------
Unit::Unit(UnitType const& type, Way& way, float offset, City& city)
    : m_type(type),
      m_way(&way),
      m_offset(offset),
      m_resources(type.resources)
{
    if (m_offset < 0.0f)
        m_offset = 0.0f;
    else if (m_offset > 1.0f)
        m_offset = 1.0f;

    m_position = m_way->positionAt(m_offset);
    m_way->addUnit(*this);
    bind(city);
}

// -----------------------------------------------------------------------------
Unit::Unit(UnitType const& type, Vector3f const& position, City& city)
    : m_type(type),
      m_position(position),
      m_resources(type.resources)
{
    bind(city);
}

// -----------------------------------------------------------------------------
void Unit::desynchronise()
{
    if (m_context.city == nullptr)
        return;

    uint32_t const perMinute = (m_context.clock != nullptr)
                               ? m_context.clock->ticksPerMinute()
                               : 1u;
    uint32_t const spread = std::max(1u, perMinute * 60u);

    // A cheap integer hash, so that consecutive identifiers do not come out as
    // consecutive phases.
    uint32_t hash = m_context.city->config().randomSeed ^ (m_id * 2654435761u);
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
    if (m_way != nullptr)
    {
        m_way->removeUnit(*this);
        m_way = nullptr;
    }
}

// -----------------------------------------------------------------------------
bool Unit::hasWays() const
{
    if (m_node != nullptr)
        return m_node->hasWays();
    return m_way != nullptr;
}

// -----------------------------------------------------------------------------
Node* Unit::accessNode() const
{
    if (m_node != nullptr)
        return m_node;
    if (m_way == nullptr)
        return nullptr;
    return (m_offset <= 0.5f) ? &m_way->from() : &m_way->to();
}

// -----------------------------------------------------------------------------
Path* Unit::path() const
{
    if (m_node != nullptr)
        return m_node->path();
    if (m_way != nullptr)
        return m_way->from().path();
    return nullptr;
}

// -----------------------------------------------------------------------------
void Unit::translate(Vector3f const& direction)
{
    // A Unit sitting on a Node or a Way follows it: the City translates the
    // Path, which moves the Nodes, and the Unit reads the new position. One
    // that was given a footprint of its own keeps it and shifts with the City.
    if (m_placed)
        m_position += direction;
    else if (m_node != nullptr)
        m_position = m_node->position();
    else if (m_way != nullptr)
        m_position = m_way->positionAt(m_offset);
    else
        m_position += direction;

    refreshMapPosition();
}

// -----------------------------------------------------------------------------
void Unit::placeAt(Vector3f const& position)
{
    m_position = position;
    m_placed = true;
    refreshMapPosition();
}

// -----------------------------------------------------------------------------
void Unit::reanchor(Way& way, float offset)
{
    detach();

    m_way = &way;
    m_offset = std::min(1.0f, std::max(0.0f, offset));
    m_way->addUnit(*this);

    if (!m_placed)
    {
        m_position = m_way->positionAt(m_offset);
        refreshMapPosition();
    }
}

// -----------------------------------------------------------------------------
void Unit::refreshMapPosition()
{
    if (m_context.city != nullptr)
        m_context.city->world2mapPosition(m_position, m_context.u, m_context.v);
}

// -----------------------------------------------------------------------------
void Unit::executeRules()
{
    m_ticks += 1u;
    if (m_context.city != nullptr)
        m_context.clock = &m_context.city->world().clock();

    uint32_t const perMinute = (m_context.clock != nullptr)
                               ? m_context.clock->ticksPerMinute()
                               : 1u;

    size_t i = m_type.rules.size();
    while (i--)
    {
        if (m_ticks % m_type.rules[i]->periodTicks(perMinute) == 0u)
        {
            m_type.rules[i]->execute(m_context);
        }
    }
}

// -----------------------------------------------------------------------------
bool Unit::accepts(std::string const& searchTarget, Resources const& resourcesToTryToAdd)
{
    return (m_resources.canAddSomeResources(resourcesToTryToAdd)) &&
            ((find(m_type.targets.begin(), m_type.targets.end(), searchTarget)
              != m_type.targets.end()));
}

} // namespace ogb
