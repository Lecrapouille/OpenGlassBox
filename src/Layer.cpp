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

    for (auto const& [_, chunk] : m_chunks)
    {
        total += chunk.total;
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
        for (auto const& [_, city] : cities)
        {
            executeRule(*rule, *city);
        }
    }

    // Transport comes after the rules, so that what a factory produced this
    // period is carried by the next one. The other order would let one amount
    // cross several cells in a single tick.
    if (m_type.spreads() && (m_ticks % m_type.getPeriodTicks(perMinute) == 0u))
    {
        spreadAndFade();
    }
}

// -----------------------------------------------------------------------------
void Layer::spreadAndFade()
{
    collectBlocksToSpread();

    // Nothing anywhere on the grid: no amount to move and none to lose.
    if (m_previous.empty())
        return;

    // Every cell of every listed block is written from the copy, so the order
    // the cells are visited in changes nothing.
    for (Cell const& origin : m_spreadBlocks)
    {
        for (int32_t dv = 0; dv < CHUNK_SIZE; ++dv)
        {
            for (int32_t du = 0; du < CHUNK_SIZE; ++du)
            {
                Cell const cell{ origin.u + du, origin.v + dv };
                setResource(cell, nextAmount(cell));
            }
        }
    }
}

// -----------------------------------------------------------------------------
void Layer::collectBlocksToSpread()
{
    m_previous.clear();
    m_spreadBlocks.clear();

    for (auto const& [key, chunk] : m_chunks)
    {
        if (chunk.total == 0u)
            continue;

        m_previous[key] = chunk.cells;

        // A cell on the border of a block gives to a cell of the next block,
        // which may not exist yet: the pass has to write there too, and a write
        // of a non-zero amount allocates the block on its own.
        Cell const origin{ chunk.u0, chunk.v0 };
        m_spreadBlocks.push_back(origin);
        m_spreadBlocks.push_back(Cell{ origin.u - CHUNK_SIZE, origin.v });
        m_spreadBlocks.push_back(Cell{ origin.u + CHUNK_SIZE, origin.v });
        m_spreadBlocks.push_back(Cell{ origin.u, origin.v - CHUNK_SIZE });
        m_spreadBlocks.push_back(Cell{ origin.u, origin.v + CHUNK_SIZE });
    }

    // Two blocks side by side each name the other, so the list repeats itself.
    std::sort(m_spreadBlocks.begin(),
              m_spreadBlocks.end(),
              [](Cell const& a, Cell const& b)
              { return (a.v != b.v) ? (a.v < b.v) : (a.u < b.u); });
    m_spreadBlocks.erase(std::unique(m_spreadBlocks.begin(),
                                     m_spreadBlocks.end(),
                                     [](Cell const& a, Cell const& b)
                                     { return (a.u == b.u) && (a.v == b.v); }),
                         m_spreadBlocks.end());
}

// -----------------------------------------------------------------------------
uint32_t Layer::nextAmount(Cell const cell) const
{
    uint32_t const before = previousAmount(cell);

    // What the cell gives away, and what it loses on the way. The parser
    // refuses a sum above one hundred, so the two shares never exceed the
    // amount.
    uint64_t const leaving = uint64_t(before) * m_type.diffusion / 100u;
    uint64_t const share = leaving / 4u;
    uint64_t fading = uint64_t(before) * m_type.decay / 100u;

    // A share of a small amount rounds down to nothing, so a cell holding one
    // or two units would keep them for ever and the whole grid would settle
    // just above zero. A layer that loses always loses at least one unit, which
    // is what lets a threshold such as "layer Pollution less 1" ever pass
    // again.
    if ((m_type.decay != 0u) && (before != 0u) && (fading == 0u))
        fading = 1u;

    uint64_t const gone = (share * 4u) + fading;

    // The remainder of the division by four stays where it is, so a layer that
    // only diffuses neither creates nor loses anything.
    uint64_t amount = (before > gone) ? (uint64_t(before) - gone) : 0u;

    // What the four neighbours give this cell, each by the same rule.
    amount += uint64_t(previousAmount(Cell{ cell.u - 1, cell.v })) *
              m_type.diffusion / 100u / 4u;
    amount += uint64_t(previousAmount(Cell{ cell.u + 1, cell.v })) *
              m_type.diffusion / 100u / 4u;
    amount += uint64_t(previousAmount(Cell{ cell.u, cell.v - 1 })) *
              m_type.diffusion / 100u / 4u;
    amount += uint64_t(previousAmount(Cell{ cell.u, cell.v + 1 })) *
              m_type.diffusion / 100u / 4u;

    // setResource() clamps to the capacity of a cell. A cell already full
    // simply refuses what its neighbours push, which is what a saturated Layer
    // means.
    return (amount > uint64_t(Resource::MAX_CAPACITY)) ? Resource::MAX_CAPACITY
                                                       : uint32_t(amount);
}

// -----------------------------------------------------------------------------
uint32_t Layer::previousAmount(Cell const cell) const
{
    auto const it = m_previous.find(chunkKey(cell));
    return (it == m_previous.end()) ? 0u : it->second[cellIndex(cell)];
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
        m_randomCoordinates.init(region.sizeU,
                                 region.sizeV,
                                 rule.takePercent(region.getCellCount()));

        uint32_t du;
        uint32_t dv;
        while (m_randomCoordinates.next(du, dv))
        {
            m_context.cell =
                Cell{ region.u0 + int32_t(du), region.v0 + int32_t(dv) };
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
