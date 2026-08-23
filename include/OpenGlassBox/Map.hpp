//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file Map.hpp
//! \brief Grid-backed resource layer shared by cities on the world map.

#ifndef OPEN_GLASSBOX_MAP_HPP
#define OPEN_GLASSBOX_MAP_HPP

#include "OpenGlassBox/MapCoordinatesInsideRadius.hpp"
#include "OpenGlassBox/MapRandomCoordinates.hpp"
#include "OpenGlassBox/MapRegion.hpp"
#include "OpenGlassBox/Rule.hpp"
#include "OpenGlassBox/Vector.hpp"
#include <algorithm>
#include <array>
#include <map>
#include <memory>
#include <unordered_map>

namespace ogb
{

class City;
class World;

//! \brief Collection of City, declared here because a Map runs its rules over
//! the region of each of them.
using Cities = std::map<std::string, std::unique_ptr<City>, std::less<>>;

//==============================================================================
//! \brief One layer of the environment: coal, oil, forest, but also air
//! pollution, land value or desirability. Every cell of the grid holds an
//! amount of that one thing, and rules read and write those amounts.
//!
//! A Map is what lets a rule ask a question about a place rather than about a
//! building: "is there water within three cells of here?" A building reads and
//! writes its neighbourhood through its footprint, which is a radius around the
//! cell it stands on.
//!
//! The grid is the one of the World, shared by every City and unbounded in the
//! four directions, which is why cell coordinates are signed. Storage is
//! sparse, by blocks of CHUNK_SIZE x CHUNK_SIZE cells allocated the first time
//! something is written into them: an empty map costs nothing, and a city can
//! be founded anywhere without deciding on a size beforehand.
//!
//! Each cell is capped at the capacity of the map type, so a resource cannot
//! pile up without bound in one place. A rule asking over a radius therefore
//! reads at most capacity times the number of cells covered, which is what
//! cellsInRadius() is for.
//!
//! Example:
//! \code
//! Map& water = city.addMap(simulation.script().getMapType("Water"));
//!
//! // Fill a pond by hand, then read what a building standing at (10,10) with a
//! // footprint of three cells would see.
//! water.setResource(10, 10, water.getCapacity());
//! uint32_t const seen = water.getResource(10, 10, 3u, city.region());
//! \endcode
//!
//! The matching script. The rule below turns water into grass, one cell at a
//! time, which is how a forest grows or pollution creeps:
//! \code
//! rules
//!     mapRule CreateGrass
//!         rate 20 minutes
//!         map Water remove 10 randomTilesPercent 90
//!         map Grass add 1
//!     end
//! end
//!
//! maps
//!     map Water color 0x0000FF capacity 100 rules [ ]
//!     map Grass color 0x00FF00 capacity 10 rules [ CreateGrass ]
//! end
//! \endcode
//==============================================================================
class Map
{
public:

    //! \brief Side of an allocation unit, in cells. A power of two, so that
    //! finding the block a cell belongs to is a shift rather than a division.
    static constexpr int32_t CHUNK_SIZE = 16;

    // -------------------------------------------------------------------------
    //! \brief An empty map: every cell reads as zero until written to, and
    //! nothing is allocated yet.
    //! \param[in] type recipe of the layer: its name, its colour, the cap of a
    //! cell and the rules to run. Kept by reference and has to outlive the Map.
    //! \param[in] world the world owning the grid the map is laid on. Kept by
    //! reference.
    // -------------------------------------------------------------------------
    Map(MapType const& type, World& world);

    // -------------------------------------------------------------------------
    //! \brief Write the amount held by one cell, capped by the capacity of the
    //! type.
    //!
    //! Writing zero into a cell of a block that was never touched allocates
    //! nothing, so clearing a map one cell at a time does not make it grow.
    //!
    //! \param[in] u, v coordinates of the cell, on the grid of the World.
    //! \param[in] amount what to store. Values above getCapacity() are clamped.
    // -------------------------------------------------------------------------
    void setResource(int32_t const u, int32_t const v, uint32_t amount)
    {
        if (amount > m_type.capacity)
            amount = m_type.capacity;

        // Do not allocate a block of cells just to write the value they
        // already read as.
        if (amount == 0u)
        {
            Chunk* const chunk = findChunk(u, v);
            if (chunk == nullptr)
                return;

            uint32_t& cell = chunk->cells[cellIndex(u, v)];
            chunk->total -= cell;
            cell = 0u;
            return;
        }

        Chunk& chunk = chunkFor(u, v);
        uint32_t& cell = chunk.cells[cellIndex(u, v)];
        chunk.total = chunk.total - cell + amount;
        cell = amount;
    }

    // -------------------------------------------------------------------------
    //! \brief \param[in] u, v coordinates of the cell.
    //! \return the amount held by that cell. A cell that was never written to
    //! reads as zero, which is also the answer for a cell outside every
    //! allocated block.
    // -------------------------------------------------------------------------
    uint32_t getResource(int32_t const u, int32_t const v) const
    {
        Chunk const* const chunk = findChunk(u, v);
        return (chunk == nullptr) ? 0u : chunk->cells[cellIndex(u, v)];
    }

    // -------------------------------------------------------------------------
    //! \brief What a building with a footprint would see: the sum over its
    //! neighbourhood.
    //!
    //! \param[in] u, v coordinates of the centre cell.
    //! \param[in] radius how far the footprint reaches, as a taxicab distance:
    //! zero is the centre cell alone. See MapCoordinatesInsideRadius.
    //! \param[in] region the cells the calling City administers. Cells outside
    //! it are left out, so a city never reads over the shoulder of its
    //! neighbour.
    //! \return the sum, capped at the largest value a uint32_t holds.
    //! \note Not const: the walk over the neighbourhood reuses a cache held by
    //! the Map, so that a rule firing every tick does not allocate.
    // -------------------------------------------------------------------------
    uint32_t getResource(int32_t const u,
                         int32_t const v,
                         uint32_t radius,
                         MapRegion const& region)
    {
        // A rule of a layer stands on the cell it reads, and does so on every
        // cell of the region: worth not walking a one element diamond, and
        // worth being inlined into the caller.
        if (radius == 0u)
            return region.contains(u, v) ? getResource(u, v) : 0u;

        return getResourceInRadius(u, v, radius, region);
    }

    // -------------------------------------------------------------------------
    //! \brief How many cells the footprint of (u,v) covers inside the region.
    //!
    //! getResource() over a radius sums that many cells, so this is what turns
    //! the capacity of one cell into the largest value a rule may read. A rule
    //! comparing against a proportion needs both.
    //!
    //! \param[in] u, v coordinates of the centre cell.
    //! \param[in] radius how far the footprint reaches.
    //! \param[in] region the cells the calling City administers.
    //! \return the number of cells covered, at least zero when the centre
    //! itself lies outside the region.
    //! \note Not const, for the same reason as getResource().
    // -------------------------------------------------------------------------
    uint32_t cellsInRadius(int32_t const u,
                           int32_t const v,
                           uint32_t const radius,
                           MapRegion const& region)
    {
        if (radius == 0u)
            return region.contains(u, v) ? 1u : 0u;

        return countCellsInRadius(u, v, radius, region);
    }

    // -------------------------------------------------------------------------
    //! \brief \return the largest amount a single cell may hold, from the type.
    // -------------------------------------------------------------------------
    uint32_t getCapacity() const
    {
        return m_type.capacity;
    }

    // -------------------------------------------------------------------------
    //! \brief Add to one cell, up to the capacity of the type. What overflows
    //! is dropped rather than pushed to a neighbour.
    //! \param[in] u, v coordinates of the cell.
    //! \param[in] toAdd how much to add.
    // -------------------------------------------------------------------------
    void addResource(int32_t const u, int32_t const v, uint32_t const toAdd)
    {
        uint32_t amount = getResource(u, v);

        // Avoid integer overflow
        if (amount >= Resource::MAX_CAPACITY - toAdd)
            amount = Resource::MAX_CAPACITY;
        else
            amount += toAdd;

        setResource(u, v, amount);
    }

    // -------------------------------------------------------------------------
    //! \brief Add to the cells of a footprint.
    //!
    //! \param[in] u, v coordinates of the centre cell.
    //! \param[in] radius how far the footprint reaches.
    //! \param[in] region the cells the calling City administers. Cells outside
    //! it get nothing.
    //! \param[in] toAdd how much to hand out over the whole footprint when
    //! \c distributed is true, or to each cell of it when false.
    //! \param[in] distributed true to walk the cells in random order and take
    //! from \c toAdd as it goes, so that the footprint receives \c toAdd in
    //! total and the last cells may get nothing; false to give \c toAdd to
    //! every cell.
    //! \note Every cell is still capped at getCapacity(), so a saturated
    //! footprint swallows nothing and the remainder is dropped.
    // -------------------------------------------------------------------------
    void addResource(int32_t const u,
                     int32_t const v,
                     uint32_t const radius,
                     MapRegion const& region,
                     uint32_t const toAdd,
                     bool distributed = true)
    {
        if (radius == 0u)
        {
            if (region.contains(u, v))
                addResource(u, v, toAdd);
            return;
        }

        addResourceInRadius(u, v, radius, region, toAdd, distributed);
    }

    // -------------------------------------------------------------------------
    //! \brief Take from one cell, down to zero. What is missing is simply not
    //! taken: the cell never goes negative.
    //! \param[in] u, v coordinates of the cell.
    //! \param[in] toRemove how much to take.
    // -------------------------------------------------------------------------
    void removeResource(int32_t const u,
                        int32_t const v,
                        uint32_t const toRemove)
    {
        uint32_t const amount = getResource(u, v);

        setResource(u, v, (amount > toRemove) ? (amount - toRemove) : 0u);
    }

    // -------------------------------------------------------------------------
    //! \brief Take from the cells of a footprint. The mirror of
    //! addResource(int32_t, int32_t, uint32_t, MapRegion const&, uint32_t,
    //! bool) down to zero instead of up to the capacity.
    //!
    //! \param[in] u, v coordinates of the centre cell.
    //! \param[in] radius how far the footprint reaches.
    //! \param[in] region the cells the calling City administers.
    //! \param[in] toRemove how much to take over the whole footprint when
    //! \c distributed is true, or from each cell of it when false.
    //! \param[in] distributed see the matching parameter of addResource().
    // -------------------------------------------------------------------------
    void removeResource(int32_t const u,
                        int32_t const v,
                        uint32_t const radius,
                        MapRegion const& region,
                        uint32_t const toRemove,
                        bool distributed = true)
    {
        if (radius == 0u)
        {
            if (region.contains(u, v))
                removeResource(u, v, toRemove);
            return;
        }

        removeResourceInRadius(u, v, radius, region, toRemove, distributed);
    }

    // -------------------------------------------------------------------------
    //! \brief \param[in] u, v coordinates of the cell.
    //! \return the world position of its top-left corner, which is what the
    //! renderer draws a rectangle from.
    // -------------------------------------------------------------------------
    Vector3f getWorldPosition(int32_t const u, int32_t const v) const;

    // -------------------------------------------------------------------------
    //! \brief Run the rules of the type over the region of every City, and
    //! count one tick.
    //!
    //! A cell administered by two Cities is walked once for each of them, since
    //! a rule reads and writes on behalf of a city.
    //!
    //! \param[in] cities the cities of the World, each contributing its region.
    // -------------------------------------------------------------------------
    void executeRules(Cities const& cities);

    // -------------------------------------------------------------------------
    //! \brief \return the name of its type, such as "Water".
    // -------------------------------------------------------------------------
    std::string const& type() const
    {
        return m_type.name;
    }

    // -------------------------------------------------------------------------
    //! \brief \return the recipe itself, so that another City can be given a
    //! layer of the same kind without looking it up by name.
    // -------------------------------------------------------------------------
    MapType const& getMapType() const
    {
        return m_type;
    }

    // -------------------------------------------------------------------------
    //! \brief \return the colour of its type, as 0xRRGGBB, which the demo
    //! shades a cell with according to how full it is.
    // -------------------------------------------------------------------------
    uint32_t color() const
    {
        return m_type.color;
    }

    // -------------------------------------------------------------------------
    //! \brief \return the side of a cell, in world units. Comes from the World,
    //! so that every layer of a world lines up.
    // -------------------------------------------------------------------------
    float cellSize() const;

    // -------------------------------------------------------------------------
    //! \brief \return the rules the layer runs, from its type. Read by the demo
    //! to show what a layer is doing.
    // -------------------------------------------------------------------------
    std::vector<RuleMap*> const& rules() const
    {
        return m_type.rules;
    }

    // -------------------------------------------------------------------------
    //! \brief \return how many ticks the layer has lived. A rule with a rate of
    //! twenty fires when this is a multiple of twenty.
    // -------------------------------------------------------------------------
    uint32_t ticks() const
    {
        return m_ticks;
    }

    // -------------------------------------------------------------------------
    //! \brief \return the sum over every allocated cell. Read from the cached
    //! total of each block, so it does not walk the grid.
    // -------------------------------------------------------------------------
    uint64_t totalResource() const;

    // -------------------------------------------------------------------------
    //! \brief \return how many blocks are allocated. Shown by the debug panel
    //! to make the memory cost of a layer visible: each block is
    //! CHUNK_SIZE x CHUNK_SIZE cells whether or not they hold anything.
    // -------------------------------------------------------------------------
    size_t allocatedChunks() const
    {
        return m_chunks.size();
    }

    // -------------------------------------------------------------------------
    //! \brief Call \c function(u, v, amount) for every cell holding something.
    //!
    //! This is how the renderer draws a layer without knowing where its cells
    //! are. Empty cells are skipped, so the cost follows what the layer holds
    //! rather than the size of the world.
    //!
    //! \param[in] function anything callable as f(int32_t, int32_t, uint32_t).
    // -------------------------------------------------------------------------
    template <class Function>
    void forEachCell(Function function) const
    {
        for (auto const& it : m_chunks)
        {
            forEachCellOfChunk(it.second, function);
        }
    }

    // -------------------------------------------------------------------------
    //! \brief Call \c function(u, v, side, mean) for every square of \c side
    //! cells inside a region that holds something, where \c mean is the average
    //! amount over the square.
    //!
    //! This is how a layer is drawn. A side of one is cell by cell; a larger
    //! one is a coarser picture of the same layer for a fraction of the
    //! rectangles, which is what keeps drawing a half million cell city
    //! affordable. The caller picks the side from the zoom: whatever keeps a
    //! square worth a pixel or two and keeps their number bounded.
    //!
    //! Blocks falling entirely outside the region are skipped without being
    //! read, so the cost follows what is on screen rather than the size of the
    //! world, and empty squares are skipped altogether.
    //!
    //! \param[in] region the cells to visit. An empty region visits nothing.
    //! \param[in] side how many cells a square spans, clamped into
    //! [1..CHUNK_SIZE] and rounded down to a divisor of CHUNK_SIZE so that a
    //! square never straddles two blocks.
    //! \param[in] function anything callable as
    //! f(int32_t, int32_t, int32_t, uint32_t).
    // -------------------------------------------------------------------------
    template <class Function>
    void forEachBlockInRegion(MapRegion const& region,
                              int32_t side,
                              Function function) const
    {
        if (region.empty())
            return;

        side = std::min(std::max(side, 1), CHUNK_SIZE);
        while ((CHUNK_SIZE % side) != 0)
            --side;

        for (auto const& it : m_chunks)
        {
            Chunk const& chunk = it.second;
            if (chunk.total == 0u)
                continue;
            if ((chunk.u0 + CHUNK_SIZE <= region.u0) ||
                (chunk.v0 + CHUNK_SIZE <= region.v0) ||
                (chunk.u0 >= region.u1()) || (chunk.v0 >= region.v1()))
            {
                continue;
            }

            forEachBlockOfChunk(chunk, region, side, function);
        }
    }

private:

    //==========================================================================
    //! \brief A square block of cells, allocated as a whole. The unit in which
    //! a sparse map grows.
    //==========================================================================
    struct Chunk
    {
        //! \brief Column of the cell at the top-left of the block, a multiple
        //! of CHUNK_SIZE.
        int32_t u0 = 0;

        //! \brief Row of the cell at the top-left of the block, a multiple of
        //! CHUNK_SIZE.
        int32_t v0 = 0;

        //! \brief Sum of the cells, kept up to date on every write. Walking the
        //! cells to get it again would cost the whole grid, and the panels ask
        //! for it on every frame.
        uint64_t total = 0u;

        //! \brief The cells, row major: the cell (u0 + du, v0 + dv) is at the
        //! index dv * CHUNK_SIZE + du. Zero initialised.
        std::array<uint32_t, size_t(CHUNK_SIZE* CHUNK_SIZE)> cells{};
    };

    //--------------------------------------------------------------------------
    //! \brief Call f(u, v, amount) for the cells of one block holding
    //! something, skipping the empty ones.
    //! \param[in] chunk the block to walk.
    //! \param[in] function anything callable as f(int32_t, int32_t, uint32_t).
    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    //! \brief The part of forEachBlockInRegion() that walks one block, once it
    //! is known to overlap the region.
    //! \param[in] chunk the block to walk.
    //! \param[in] region the cells to keep. Cells of the block outside it are
    //! left out of the average rather than counted as empty.
    //! \param[in] side how many cells a square spans, a divisor of CHUNK_SIZE.
    //! \param[in] function anything callable as
    //! f(int32_t, int32_t, int32_t, uint32_t).
    //--------------------------------------------------------------------------
    template <class Function>
    static void forEachBlockOfChunk(Chunk const& chunk,
                                    MapRegion const& region,
                                    int32_t const side,
                                    Function function)
    {
        for (int32_t dv = 0; dv < CHUNK_SIZE; dv += side)
        {
            for (int32_t du = 0; du < CHUNK_SIZE; du += side)
            {
                uint64_t sum = 0u;
                uint32_t counted = 0u;
                sumSquare(chunk, region, du, dv, side, sum, counted);

                if ((sum != 0u) && (counted != 0u))
                {
                    function(chunk.u0 + du,
                             chunk.v0 + dv,
                             side,
                             uint32_t(sum / counted));
                }
            }
        }
    }

    //--------------------------------------------------------------------------
    //! \brief Add up the cells of one square of a block that lie inside a
    //! region.
    //! \param[in] chunk the block the square belongs to.
    //! \param[in] region the cells to keep.
    //! \param[in] du, dv top-left corner of the square, inside the block.
    //! \param[in] side how many cells the square spans.
    //! \param[out] sum what those cells hold, added up.
    //! \param[out] counted how many of them lay inside the region.
    //--------------------------------------------------------------------------
    static void sumSquare(Chunk const& chunk,
                          MapRegion const& region,
                          int32_t const du,
                          int32_t const dv,
                          int32_t const side,
                          uint64_t& sum,
                          uint32_t& counted)
    {
        for (int32_t v = dv; v < dv + side; ++v)
        {
            for (int32_t u = du; u < du + side; ++u)
            {
                if (!region.contains(chunk.u0 + u, chunk.v0 + v))
                    continue;
                sum += chunk.cells[size_t(v * CHUNK_SIZE + u)];
                ++counted;
            }
        }
    }

    template <class Function>
    static void forEachCellOfChunk(Chunk const& chunk, Function function)
    {
        for (int32_t dv = 0; dv < CHUNK_SIZE; ++dv)
        {
            for (int32_t du = 0; du < CHUNK_SIZE; ++du)
            {
                uint32_t const amount =
                    chunk.cells[size_t(dv * CHUNK_SIZE + du)];
                if (amount != 0u)
                {
                    function(chunk.u0 + du, chunk.v0 + dv, amount);
                }
            }
        }
    }

    // The three functions below sit on the hottest path of the whole library:
    // a rule of a layer addresses a cell several times over, on every cell of
    // the town, on every tick. They are defined here rather than in the source
    // file so that the caller can inline them, and they use masks rather than
    // divisions, which a side that is a power of two makes exact.
    static_assert((CHUNK_SIZE & (CHUNK_SIZE - 1)) == 0,
                  "the side of a block has to be a power of two");

    //! \brief Mask of the bits a coordinate uses inside its block.
    static constexpr int32_t CHUNK_MASK = CHUNK_SIZE - 1;

    //! \brief How far to shift a coordinate to get the number of its block.
    //! The base two logarithm of CHUNK_SIZE.
    static constexpr int32_t CHUNK_SHIFT = 4;
    static_assert((1 << CHUNK_SHIFT) == CHUNK_SIZE,
                  "CHUNK_SHIFT has to be the logarithm of CHUNK_SIZE");

    //--------------------------------------------------------------------------
    //! \brief Round a coordinate down to the origin of its block.
    //!
    //! Rounding is towards minus infinity rather than towards zero, so that the
    //! cells at minus one and at zero land in different blocks instead of
    //! sharing the one at the origin. Clearing the low bits of a two's
    //! complement integer does exactly that, and does it without a branch.
    //!
    //! \param[in] coordinate a column or a row.
    //! \return the matching multiple of CHUNK_SIZE.
    //--------------------------------------------------------------------------
    static int32_t chunkOrigin(int32_t const coordinate)
    {
        return coordinate & ~CHUNK_MASK;
    }

    //--------------------------------------------------------------------------
    //! \brief Pack the origin of the block holding a cell into one key, which
    //! is what the table of blocks is indexed by.
    //! \param[in] u, v coordinates of the cell.
    //! \return the key of its block.
    //--------------------------------------------------------------------------
    static int64_t chunkKey(int32_t const u, int32_t const v)
    {
        // Shifting a negative number right rounds towards minus infinity on
        // every compiler this builds with, and C++20 requires it, which is the
        // same rounding chunkOrigin() uses.
        int64_t const cu = u >> CHUNK_SHIFT;
        int64_t const cv = v >> CHUNK_SHIFT;

        return (cu << 32) ^ (cv & 0xFFFFFFFF);
    }

    //--------------------------------------------------------------------------
    //! \brief \param[in] u, v coordinates of the cell.
    //! \return where it sits inside Chunk::cells.
    //--------------------------------------------------------------------------
    static size_t cellIndex(int32_t const u, int32_t const v)
    {
        int32_t const du = u & CHUNK_MASK;
        int32_t const dv = v & CHUNK_MASK;

        return size_t(dv * CHUNK_SIZE + du);
    }

    //--------------------------------------------------------------------------
    //! \brief \param[in] u, v coordinates of the cell.
    //! \return the block holding it, or nullptr when nothing was ever written
    //! anywhere near it. Reading is allowed not to allocate.
    //!
    //! One rule attempt addresses the same cell three or four times over, and
    //! consecutive cells share a block sixteen times out of seventeen, so the
    //! last block found is remembered and the table is left alone.
    //--------------------------------------------------------------------------
    Chunk const* findChunk(int32_t const u, int32_t const v) const
    {
        int64_t const key = chunkKey(u, v);

        if (m_cacheFilled && (m_cachedKey == key))
            return m_cachedChunk;

        return lookupChunk(key);
    }

    //--------------------------------------------------------------------------
    //! \brief The writable counterpart of findChunk(), for the caller that
    //! clears a cell of a block it is not allowed to create.
    //! \param[in] u, v coordinates of the cell.
    //! \return the block holding it, or nullptr.
    //--------------------------------------------------------------------------
    Chunk* findChunk(int32_t const u, int32_t const v)
    {
        // This Map is not const here, so neither are the blocks it owns: only
        // the memoisation, which reading a cell has to fill too, made the
        // other overload the const one.
        return const_cast<Chunk*>(
            static_cast<Map const*>(this)->findChunk(u, v));
    }

    //--------------------------------------------------------------------------
    //! \brief \param[in] u, v coordinates of the cell.
    //! \return the block holding it, allocating and zero filling it when it did
    //! not exist yet.
    //--------------------------------------------------------------------------
    Chunk& chunkFor(int32_t const u, int32_t const v)
    {
        int64_t const key = chunkKey(u, v);

        if (m_cacheFilled && (m_cachedKey == key) && (m_cachedChunk != nullptr))
            return *findChunk(u, v);

        return createChunk(key, u, v);
    }

    //--------------------------------------------------------------------------
    //! \brief The part of findChunk() that has to read the table: the miss.
    //! \param[in] key key of the block, from chunkKey().
    //! \return the block, or nullptr when there is none. Fills the memo.
    //--------------------------------------------------------------------------
    Chunk const* lookupChunk(int64_t const key) const;

    //--------------------------------------------------------------------------
    //! \brief The part of chunkFor() that has to read or grow the table.
    //! \param[in] key key of the block, from chunkKey().
    //! \param[in] u, v coordinates of a cell of the block, which give its
    //! origin when it has to be created.
    //! \return the block. Fills the memo.
    //--------------------------------------------------------------------------
    Chunk& createChunk(int64_t const key, int32_t const u, int32_t const v);

    //--------------------------------------------------------------------------
    //! \brief The part of getResource() that walks a footprint of more than one
    //! cell. Same parameters and same result.
    //--------------------------------------------------------------------------
    uint32_t getResourceInRadius(int32_t const u,
                                 int32_t const v,
                                 uint32_t const radius,
                                 MapRegion const& region);

    //--------------------------------------------------------------------------
    //! \brief The part of cellsInRadius() that walks a footprint of more than
    //! one cell. Same parameters and same result.
    //--------------------------------------------------------------------------
    uint32_t countCellsInRadius(int32_t const u,
                                int32_t const v,
                                uint32_t const radius,
                                MapRegion const& region);

    //--------------------------------------------------------------------------
    //! \brief The part of addResource() that walks a footprint of more than one
    //! cell. Same parameters.
    //--------------------------------------------------------------------------
    void addResourceInRadius(int32_t const u,
                             int32_t const v,
                             uint32_t const radius,
                             MapRegion const& region,
                             uint32_t const toAdd,
                             bool const distributed);

    //--------------------------------------------------------------------------
    //! \brief The part of removeResource() that walks a footprint of more than
    //! one cell. Same parameters.
    //--------------------------------------------------------------------------
    void removeResourceInRadius(int32_t const u,
                                int32_t const v,
                                uint32_t const radius,
                                MapRegion const& region,
                                uint32_t const toRemove,
                                bool const distributed);

    //--------------------------------------------------------------------------
    //! \brief Run one rule over the region of one city, cell by cell.
    //! \param[in] rule the rule to run.
    //! \param[in] city whose region bounds the walk and on whose behalf the
    //! rule reads and writes.
    //--------------------------------------------------------------------------
    void executeRule(RuleMap& rule, City& city);

private:

    //! \brief Recipe of the layer, shared with every layer of that kind.
    MapType const& m_type;

    //! \brief The world owning the grid the layer is laid on. Where the size of
    //! a cell comes from.
    World& m_world;

    //! \brief Everything a rule needs while it runs: the city, the cell, the
    //! layer. Held here rather than built on each call, since a rule fires over
    //! thousands of cells per tick.
    RuleContext m_context;

    //! \brief How many ticks the layer has lived, which is what the rate of a
    //! rule is counted against.
    uint32_t m_ticks = 0u;

    //! \brief The cells, by blocks allocated on demand and keyed by chunkKey().
    std::unordered_map<int64_t, Chunk> m_chunks;

    //! \brief Key of the block the last lookup landed in. Meaningless until
    //! m_cacheFilled.
    mutable int64_t m_cachedKey = 0;

    //! \brief The block that key names, or nullptr when there is none. Blocks
    //! are never removed and \c std::unordered_map keeps its elements put
    //! across a rehash, so a pointer stays good for the life of the Map.
    mutable Chunk const* m_cachedChunk = nullptr;

    //! \brief Whether the two above hold an answer. A rule sweeping the grid
    //! walks a whole row of a block before leaving it, so remembering the last
    //! one answers fifteen lookups out of sixteen without hashing.
    mutable bool m_cacheFilled = false;

    //! \brief Reusable walk over the cells of a footprint. Held here so that
    //! reading a neighbourhood does not allocate.
    MapCoordinatesInsideRadius m_coordinates;

    //! \brief Reusable walk over the cells of a footprint in random order, used
    //! when handing out a resource unevenly.
    MapRandomCoordinates m_randomCoordinates;
};

//! \brief The layers of a World, by name, which owns them.
using Maps = std::map<std::string, std::unique_ptr<Map>, std::less<>>;

} // namespace ogb

#endif
