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
namespace ogb {

Map::Map(MapType const& type, World& world)
    : m_type(type), m_world(world)
{
    m_context.map = this;
}

// -----------------------------------------------------------------------------
float Map::cellSize() const
{
    return m_world.cellSize();
}

// -----------------------------------------------------------------------------
int32_t Map::chunkOrigin(int32_t const coordinate)
{
    // Rounding towards zero would make the chunk straddling the origin twice as
    // wide, so round down instead.
    int32_t const origin = (coordinate / CHUNK_SIZE) * CHUNK_SIZE;
    return (coordinate < 0) && (origin != coordinate) ? origin - CHUNK_SIZE
                                                      : origin;
}

// -----------------------------------------------------------------------------
int64_t Map::chunkKey(int32_t const u, int32_t const v)
{
    int64_t const cu = chunkOrigin(u) / CHUNK_SIZE;
    int64_t const cv = chunkOrigin(v) / CHUNK_SIZE;

    return (cu << 32) ^ (cv & 0xFFFFFFFF);
}

// -----------------------------------------------------------------------------
size_t Map::cellIndex(int32_t const u, int32_t const v)
{
    int32_t const du = u - chunkOrigin(u);
    int32_t const dv = v - chunkOrigin(v);

    return size_t(dv * CHUNK_SIZE + du);
}

// -----------------------------------------------------------------------------
Map::Chunk const* Map::findChunk(int32_t const u, int32_t const v) const
{
    auto const it = m_chunks.find(chunkKey(u, v));
    return (it == m_chunks.end()) ? nullptr : &(it->second);
}

// -----------------------------------------------------------------------------
Map::Chunk& Map::chunkFor(int32_t const u, int32_t const v)
{
    int64_t const key = chunkKey(u, v);

    auto const it = m_chunks.find(key);
    if (it != m_chunks.end())
        return it->second;

    Chunk& chunk = m_chunks[key];
    chunk.u0 = chunkOrigin(u);
    chunk.v0 = chunkOrigin(v);

    return chunk;
}

// -----------------------------------------------------------------------------
void Map::setResource(int32_t const u, int32_t const v, uint32_t amount)
{
    if (amount > m_type.capacity)
        amount = m_type.capacity;

    // Do not allocate a block of cells just to write the value they already
    // read as.
    if (amount == 0u)
    {
        auto const it = m_chunks.find(chunkKey(u, v));
        if (it == m_chunks.end())
            return;

        uint32_t& cell = it->second.cells[cellIndex(u, v)];
        it->second.total -= cell;
        cell = 0u;
        return;
    }

    Chunk& chunk = chunkFor(u, v);
    uint32_t& cell = chunk.cells[cellIndex(u, v)];
    chunk.total = chunk.total - cell + amount;
    cell = amount;
}

// -----------------------------------------------------------------------------
uint32_t Map::getResource(int32_t const u, int32_t const v) const
{
    Chunk const* const chunk = findChunk(u, v);
    return (chunk == nullptr) ? 0u : chunk->cells[cellIndex(u, v)];
}

// -----------------------------------------------------------------------------
uint32_t Map::getResource(int32_t const u, int32_t const v, uint32_t radius,
                          MapRegion const& region)
{
    // A map rule reads the cell it stands on, and it does so for every cell of
    // the region: worth not walking a one element circle to get there.
    if (radius == 0u)
        return region.contains(u, v) ? getResource(u, v) : 0u;

    uint32_t totalInsideRadius = 0u;
    int32_t x = u;
    int32_t y = v;

    m_coordinates.init(radius, x, y, region.u0, region.u1(), region.v0,
                       region.v1(), false);

    while (m_coordinates.next(x, y))
        totalInsideRadius += getResource(x, y);

    return totalInsideRadius;
}

// -----------------------------------------------------------------------------
uint32_t Map::cellsInRadius(int32_t const u, int32_t const v,
                            uint32_t const radius, MapRegion const& region)
{
    if (radius == 0u)
        return region.contains(u, v) ? 1u : 0u;

    uint32_t count = 0u;
    int32_t x = u;
    int32_t y = v;

    m_coordinates.init(radius, x, y, region.u0, region.u1(), region.v0,
                       region.v1(), false);

    while (m_coordinates.next(x, y))
        ++count;

    return count;
}

// -----------------------------------------------------------------------------
void Map::addResource(int32_t const u, int32_t const v, uint32_t toAdd)
{
    uint32_t amount = getResource(u, v);

    // Avoid integer overflow
    if (amount >= Resource::MAX_CAPACITY - toAdd)
        amount = Resource::MAX_CAPACITY;
    else
        amount += toAdd;

    setResource(u, v, amount);
}

// -----------------------------------------------------------------------------
void Map::addResource(int32_t const u, int32_t const v, uint32_t const radius,
                      MapRegion const& region, uint32_t toAdd, bool distributed)
{
    if (radius == 0u)
    {
        if (region.contains(u, v))
            addResource(u, v, toAdd);
        return;
    }

    uint32_t remainingToAdd = toAdd;
    int32_t x = u;
    int32_t y = v;

    m_coordinates.init(radius, x, y, region.u0, region.u1(), region.v0,
                       region.v1(), distributed);
    while ((remainingToAdd > 0u) && m_coordinates.next(x, y))
    {
        uint32_t amount = getResource(x, y);
        toAdd = std::min(m_type.capacity - amount, remainingToAdd);
        if (toAdd > 0u)
        {
            amount += toAdd;
            if (distributed) { remainingToAdd -= toAdd; }
            setResource(x, y, amount);
        }
    }
}

// -----------------------------------------------------------------------------
void Map::removeResource(int32_t const u, int32_t const v, uint32_t toRemove)
{
    uint32_t amount = getResource(u, v);

    if (amount > toRemove)
        amount -= toRemove;
    else
        amount = 0u;

    setResource(u, v, amount);
}

// -----------------------------------------------------------------------------
void Map::removeResource(int32_t const u, int32_t const v, uint32_t radius,
                         MapRegion const& region, uint32_t toRemove,
                         bool distributed)
{
    if (radius == 0u)
    {
        if (region.contains(u, v))
            removeResource(u, v, toRemove);
        return;
    }

    uint32_t remainingToRemove = toRemove;
    int32_t x = u;
    int32_t y = v;

    m_coordinates.init(radius, x, y, region.u0, region.u1(), region.v0,
                       region.v1(), distributed);
    while ((remainingToRemove > 0u) && m_coordinates.next(x, y))
    {
        uint32_t amount = getResource(x, y);
        toRemove = std::min(amount, remainingToRemove);
        if (toRemove > 0u)
        {
            amount -= toRemove;
            if (distributed) { remainingToRemove -= toRemove; }
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

    for (auto const& it: m_chunks)
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

    for (auto& rule: m_type.rules)
    {
        if (m_ticks % rule->periodTicks(perMinute) != 0u)
            continue;

        // The grid has no bounds of its own: what a rule walks is the region
        // administered by each City.
        for (auto const& it: cities)
        {
            executeRule(*rule, *(it.second));
        }
    }
}

// -----------------------------------------------------------------------------
void Map::executeRule(RuleMap& rule, City& city)
{
    MapRegion const region = city.region();
    if (region.empty())
        return;

    m_context.city = &city;
    m_context.globals = &(city.globals());
    m_context.clock = &city.world().clock();

    if (rule.isRandom())
    {
        m_randomCoordinates.init(region.sizeU, region.sizeV);
        uint32_t tilesAmount = rule.percent(uint32_t(region.area()));
        while (tilesAmount--)
        {
            uint32_t du, dv;
            if (m_randomCoordinates.next(du, dv))
            {
                m_context.u = region.u0 + int32_t(du);
                m_context.v = region.v0 + int32_t(dv);
                rule.execute(m_context);
            }
        }
    }
    else
    {
        uint32_t du = region.sizeU;
        while (du--)
        {
            m_context.u = region.u0 + int32_t(du);
            uint32_t dv = region.sizeV;
            while (dv--)
            {
                m_context.v = region.v0 + int32_t(dv);
                rule.execute(m_context);
            }
        }
    }
}

} // namespace ogb
