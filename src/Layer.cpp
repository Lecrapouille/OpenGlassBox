//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "OpenGlassBox/Layer.hpp"
#include "OpenGlassBox/City.hpp"
#include "OpenGlassBox/World.hpp"

#include <algorithm>

// -----------------------------------------------------------------------------
namespace ogb
{

Layer::Layer(LayerType const& type, World& world) : m_type(type), m_world(world)
{
    m_context.layer = this;
}

// -----------------------------------------------------------------------------
float Layer::getCellSize() const
{
    return m_world.getCellSize();
}

// -----------------------------------------------------------------------------
Layer::Chunk const* Layer::lookupChunk(int64_t const key) const
{
    auto const it = m_chunks.find(key);

    m_cachedKey = key;
    m_cachedChunk = (it == m_chunks.end()) ? nullptr : &(it->second);
    m_cacheFilled = true;

    return m_cachedChunk;
}

// -----------------------------------------------------------------------------
Layer::Chunk& Layer::createChunk(int64_t const key, Cell const cell)
{
    auto it = m_chunks.find(key);
    if (it == m_chunks.end())
    {
        it = m_chunks.emplace(key, Chunk{}).first;
        it->second.u0 = chunkOrigin(cell.u);
        it->second.v0 = chunkOrigin(cell.v);
    }

    // The table may just have grown, so a cached miss for this key is now
    // wrong.
    m_cachedKey = key;
    m_cachedChunk = &(it->second);
    m_cacheFilled = true;

    return it->second;
}

// -----------------------------------------------------------------------------
uint32_t Layer::sumInRadius(Cell const centre,
                          uint32_t const radius,
                          CellRegion const& region) const
{
    uint32_t totalInsideRadius = 0u;
    int32_t x = centre.u;
    int32_t y = centre.v;

    m_coordinates.init(radius,
                       x,
                       y,
                       region.u0,
                       region.getMaxU(),
                       region.v0,
                       region.getMaxV(),
                       false);

    while (m_coordinates.next(x, y))
        totalInsideRadius += getResource(Cell{ x, y });

    return totalInsideRadius;
}

// -----------------------------------------------------------------------------
uint32_t Layer::walkCellsInRadius(Cell const centre,
                                uint32_t const radius,
                                CellRegion const& region) const
{
    uint32_t count = 0u;
    int32_t x = centre.u;
    int32_t y = centre.v;

    m_coordinates.init(radius,
                       x,
                       y,
                       region.u0,
                       region.getMaxU(),
                       region.v0,
                       region.getMaxV(),
                       false);

    while (m_coordinates.next(x, y))
        ++count;

    return count;
}

// -----------------------------------------------------------------------------
void Layer::addResourceInRadius(Cell const centre,
                              uint32_t const radius,
                              CellRegion const& region,
                              uint32_t const wanted,
                              bool const distributed)
{
    uint32_t toAdd = wanted;
    uint32_t remainingToAdd = wanted;
    int32_t x = centre.u;
    int32_t y = centre.v;

    m_coordinates.init(radius,
                       x,
                       y,
                       region.u0,
                       region.getMaxU(),
                       region.v0,
                       region.getMaxV(),
                       distributed);
    while ((remainingToAdd > 0u) && m_coordinates.next(x, y))
    {
        Cell const cell{ x, y };
        uint32_t amount = getResource(cell);
        toAdd = std::min(m_type.capacity - amount, remainingToAdd);
        if (toAdd > 0u)
        {
            amount += toAdd;
            if (distributed)
            {
                remainingToAdd -= toAdd;
            }
            setResource(cell, amount);
        }
    }
}

// -----------------------------------------------------------------------------
void Layer::removeResourceInRadius(Cell const centre,
                                 uint32_t const radius,
                                 CellRegion const& region,
                                 uint32_t const wanted,
                                 bool const distributed)
{
    uint32_t toRemove = wanted;
    uint32_t remainingToRemove = wanted;
    int32_t x = centre.u;
    int32_t y = centre.v;

    m_coordinates.init(radius,
                       x,
                       y,
                       region.u0,
                       region.getMaxU(),
                       region.v0,
                       region.getMaxV(),
                       distributed);
    while ((remainingToRemove > 0u) && m_coordinates.next(x, y))
    {
        Cell const cell{ x, y };
        uint32_t amount = getResource(cell);
        toRemove = std::min(amount, remainingToRemove);
        if (toRemove > 0u)
        {
            amount -= toRemove;
            if (distributed)
            {
                remainingToRemove -= toRemove;
            }
            setResource(cell, amount);
        }
    }
}

// -----------------------------------------------------------------------------
Vector3f Layer::cellToWorld(Cell const cell) const
{
    return m_world.cellToWorld(cell);
}

// -----------------------------------------------------------------------------
uint64_t Layer::getTotalResource() const
{
    uint64_t total = 0u;

    for (auto const& it : m_chunks)
    {
        total += it.second.total;
    }

    return total;
}

// -----------------------------------------------------------------------------
void Layer::executeRules(Cities const& cities)
{
    ++m_ticks;

    uint32_t const perMinute = m_world.getClock().getTicksPerMinute();

    for (auto& rule : m_type.rules)
    {
        if (m_ticks % rule->getPeriodTicks(perMinute) != 0u)
            continue;

        // The grid has no bounds of its own: what a rule walks is the cells
        // owned by each city.
        for (auto const& it : cities)
        {
            executeRule(*rule, *(it.second));
        }
    }
}

// -----------------------------------------------------------------------------
void Layer::executeRule(RuleLayer& rule, City& city)
{
    CellRegion const& region = city.getRegion();
    if (region.isEmpty())
        return;

    m_context.city = &city;
    m_context.globals = &(city.getGlobals());
    m_context.clock = &city.getClock();

    if (rule.isRandom())
    {
        // A share of the cells, drawn over the whole region but handed out in
        // reading order, for the same reason the full sweep below reads in
        // that order.
        m_randomCoordinates.init(
            region.sizeU, region.sizeV, rule.takePercent(region.getCellCount()));

        uint32_t du;
        uint32_t dv;
        while (m_randomCoordinates.next(du, dv))
        {
            m_context.cell = Cell{ region.u0 + int32_t(du),
                                   region.v0 + int32_t(dv) };
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
        int32_t const maxU = region.getMaxU();
        int32_t const maxV = region.getMaxV();

        for (int32_t v = region.v0; v < maxV; ++v)
        {
            m_context.cell.v = v;
            for (int32_t u = region.u0; u < maxU; ++u)
            {
                m_context.cell.u = u;
                rule.execute(m_context);
            }
        }
    }
}

} // namespace ogb
