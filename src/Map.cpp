//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "OpenGlassBox/Map.hpp"
#include "OpenGlassBox/City.hpp"
#include "OpenGlassBox/World.hpp"

#include <algorithm>

// -----------------------------------------------------------------------------
namespace ogb
{

Map::Map(MapType const& type, World& world) : m_type(type), m_world(world)
{
    m_context.map = this;
}

// -----------------------------------------------------------------------------
float Map::cellSize() const
{
    return m_world.cellSize();
}

// -----------------------------------------------------------------------------
Map::Chunk const* Map::lookupChunk(int64_t const key) const
{
    auto const it = m_chunks.find(key);

    m_cachedKey = key;
    m_cachedChunk = (it == m_chunks.end()) ? nullptr : &(it->second);
    m_cacheFilled = true;

    return m_cachedChunk;
}

// -----------------------------------------------------------------------------
Map::Chunk& Map::createChunk(int64_t const key,
                             int32_t const u,
                             int32_t const v)
{
    auto it = m_chunks.find(key);
    if (it == m_chunks.end())
    {
        it = m_chunks.emplace(key, Chunk{}).first;
        it->second.u0 = chunkOrigin(u);
        it->second.v0 = chunkOrigin(v);
    }

    // The table may just have grown, so a cached miss for this key is now
    // wrong.
    m_cachedKey = key;
    m_cachedChunk = &(it->second);
    m_cacheFilled = true;

    return it->second;
}

// -----------------------------------------------------------------------------
uint32_t Map::getResourceInRadius(int32_t const u,
                                  int32_t const v,
                                  uint32_t const radius,
                                  MapRegion const& region)
{
    uint32_t totalInsideRadius = 0u;
    int32_t x = u;
    int32_t y = v;

    m_coordinates.init(
        radius, x, y, region.u0, region.u1(), region.v0, region.v1(), false);

    while (m_coordinates.next(x, y))
        totalInsideRadius += getResource(x, y);

    return totalInsideRadius;
}

// -----------------------------------------------------------------------------
uint32_t Map::countCellsInRadius(int32_t const u,
                                 int32_t const v,
                                 uint32_t const radius,
                                 MapRegion const& region)
{
    uint32_t count = 0u;
    int32_t x = u;
    int32_t y = v;

    m_coordinates.init(
        radius, x, y, region.u0, region.u1(), region.v0, region.v1(), false);

    while (m_coordinates.next(x, y))
        ++count;

    return count;
}

// -----------------------------------------------------------------------------
void Map::addResourceInRadius(int32_t const u,
                              int32_t const v,
                              uint32_t const radius,
                              MapRegion const& region,
                              uint32_t const wanted,
                              bool const distributed)
{
    uint32_t toAdd = wanted;
    uint32_t remainingToAdd = wanted;
    int32_t x = u;
    int32_t y = v;

    m_coordinates.init(radius,
                       x,
                       y,
                       region.u0,
                       region.u1(),
                       region.v0,
                       region.v1(),
                       distributed);
    while ((remainingToAdd > 0u) && m_coordinates.next(x, y))
    {
        uint32_t amount = getResource(x, y);
        toAdd = std::min(m_type.capacity - amount, remainingToAdd);
        if (toAdd > 0u)
        {
            amount += toAdd;
            if (distributed)
            {
                remainingToAdd -= toAdd;
            }
            setResource(x, y, amount);
        }
    }
}

// -----------------------------------------------------------------------------
void Map::removeResourceInRadius(int32_t const u,
                                 int32_t const v,
                                 uint32_t const radius,
                                 MapRegion const& region,
                                 uint32_t const wanted,
                                 bool const distributed)
{
    uint32_t toRemove = wanted;
    uint32_t remainingToRemove = wanted;
    int32_t x = u;
    int32_t y = v;

    m_coordinates.init(radius,
                       x,
                       y,
                       region.u0,
                       region.u1(),
                       region.v0,
                       region.v1(),
                       distributed);
    while ((remainingToRemove > 0u) && m_coordinates.next(x, y))
    {
        uint32_t amount = getResource(x, y);
        toRemove = std::min(amount, remainingToRemove);
        if (toRemove > 0u)
        {
            amount -= toRemove;
            if (distributed)
            {
                remainingToRemove -= toRemove;
            }
            setResource(x, y, amount);
        }
    }
}

// -----------------------------------------------------------------------------
Vector3f Map::getWorldPosition(int32_t const u, int32_t const v) const
{
    return m_world.mapPosition2world(u, v);
}

// -----------------------------------------------------------------------------
uint64_t Map::totalResource() const
{
    uint64_t total = 0u;

    for (auto const& it : m_chunks)
    {
        total += it.second.total;
    }

    return total;
}

// -----------------------------------------------------------------------------
void Map::executeRules(Cities const& cities)
{
    ++m_ticks;

    uint32_t const perMinute = m_world.clock().ticksPerMinute();

    for (auto& rule : m_type.rules)
    {
        if (m_ticks % rule->periodTicks(perMinute) != 0u)
            continue;

        // The grid has no bounds of its own: what a rule walks is the region
        // administered by each City.
        for (auto const& it : cities)
        {
            executeRule(*rule, *(it.second));
        }
    }
}

// -----------------------------------------------------------------------------
void Map::executeRule(RuleMap& rule, City& city)
{
    MapRegion const& region = city.region();
    if (region.empty())
        return;

    m_context.city = &city;
    m_context.globals = &(city.globals());
    m_context.clock = &city.world().clock();

    if (rule.isRandom())
    {
        // A share of the cells, drawn over the whole region but handed out in
        // reading order, for the same reason the full sweep below reads in
        // that order.
        m_randomCoordinates.init(
            region.sizeU, region.sizeV, rule.percent(region.area()));

        uint32_t du;
        uint32_t dv;
        while (m_randomCoordinates.next(du, dv))
        {
            m_context.u = region.u0 + int32_t(du);
            m_context.v = region.v0 + int32_t(dv);
            rule.execute(m_context);
        }
    }
    else
    {
        // Row by row, and each row from left to right: the cells of a block
        // are stored in that order, so a run of sixteen columns is a run of
        // sixteen adjacent words, which is one cache line rather than sixteen.
        // A rule of a layer only ever touches the cell it stands on, so the
        // order the cells are visited in does not change the outcome.
        int32_t const u1 = region.u1();
        int32_t const v1 = region.v1();

        for (int32_t v = region.v0; v < v1; ++v)
        {
            m_context.v = v;
            for (int32_t u = region.u0; u < u1; ++u)
            {
                m_context.u = u;
                rule.execute(m_context);
            }
        }
    }
}

} // namespace ogb
