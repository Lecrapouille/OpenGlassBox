//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file Layer.hpp
//! \brief Resource layer on the world grid. Cities share it.

#ifndef OPEN_GLASSBOX_LAYER_HPP
#define OPEN_GLASSBOX_LAYER_HPP

#include "OpenGlassBox/CellRegion.hpp"
#include "OpenGlassBox/CellsInRadius.hpp"
#include "OpenGlassBox/RandomCells.hpp"
#include "OpenGlassBox/Rule.hpp"
#include "OpenGlassBox/Vector.hpp"

#include <array>
#include <memory>
#include <unordered_map>

namespace ogb
{

class City;
class World;

//! \brief Map of City objects. A Layer runs rules over each city's region.
using Cities = std::map<std::string, std::unique_ptr<City>, std::less<>>;

//==============================================================================
//! \brief One environment layer: coal, oil, forest, air pollution, land value,
//! or desirability. Each grid cell holds an amount. Rules read and write those
//! amounts.
//!
//! A Layer lets a rule ask about a place, not a building.
//! Example: "Is there water within three cells?"
//! A building reads and writes through its footprint: a radius around its cell.
//!
//! The grid belongs to the World. Every City shares it.
//! Cell coordinates are signed. The grid has no fixed size.
//! Storage is sparse. Blocks of CHUNK_SIZE x CHUNK_SIZE cells allocate on first
//! write. An empty layer uses no memory. A city can start anywhere without a
//! preset size.
//!
//! Each cell is capped at the layer capacity.
//! A rule over a radius reads at most capacity times the cell count.
//! countCellsInRadius() returns that cell count.
//!
//! Example:
//! \code
//! Layer& water = city.addLayer(simulation.getRuleset().getLayerType("Water"));
//!
//! // Fill a pond. Read what a building at (10,10) with footprint 3 sees.
//! water.setResource({ 10, 10 }, water.getCellCapacity());
//! uint32_t const seen = water.getResource({ 10, 10 }, 3u, city.getRegion());
//! \endcode
//!
//! Matching script. This rule turns water into grass, one cell at a time.
//! Forests grow and pollution spreads the same way:
//! \code
//! rules
//!     layerRule CreateGrass
//!         rate 20 minutes
//!         layer Water remove 10 randomTilesPercent 90
//!         layer Grass add 1
//!     end
//! end
//!
//! layers
//!     layer Water color 0x0000FF capacity 100 rules [ ]
//!     layer Grass color 0x00FF00 capacity 10 rules [ CreateGrass ]
//! end
//! \endcode
//==============================================================================
class Layer
{
public:

    //! \brief Block size in cells. A power of two.
    //! Block lookup uses a shift, not division.
    static constexpr int32_t CHUNK_SIZE = 16;

    // -------------------------------------------------------------------------
    //! \brief Create an empty layer. Unwritten cells read as zero.
    //! \param[in] type Layer recipe: name, color, cell cap, and rules.
    //!     Kept by reference. Must outlive the Layer.
    //! \param[in] world World that owns the grid. Kept by reference.
    // -------------------------------------------------------------------------
    Layer(LayerType const& type, World& world);

    Layer(Layer const&) = delete;
    Layer& operator=(Layer const&) = delete;
    Layer(Layer&&) = delete;
    Layer& operator=(Layer&&) = delete;

    // -------------------------------------------------------------------------
    //! \brief Write the amount in one cell. Values above capacity are clamped.
    //!
    //! Writing zero does not allocate a new block.
    //! Clearing cell by cell does not grow memory use.
    //!
    //! \param[in] cell Cell on the World grid.
    //! \param[in] amount Value to store. Clamped to getCellCapacity().
    // -------------------------------------------------------------------------
    void setResource(Cell const cell, uint32_t amount)
    {
        if (amount > m_type.capacity)
            amount = m_type.capacity;

        // Do not allocate a block of cells just to write the value they
        // already read as.
        if (amount == 0u)
        {
            Chunk* const chunk = findChunk(cell);
            if (chunk == nullptr)
                return;

            uint32_t& stored = chunk->cells[cellIndex(cell)];
            chunk->total -= stored;
            stored = 0u;
            return;
        }

        Chunk& chunk = chunkFor(cell);
        uint32_t& stored = chunk.cells[cellIndex(cell)];
        chunk.total = chunk.total - stored + amount;
        stored = amount;
    }

    // -------------------------------------------------------------------------
    //! \brief Read the amount in one cell.
    //! \param[in] cell Cell to read.
    //! \return Amount in that cell. Unwritten cells return zero.
    //!     Cells outside allocated blocks also return zero.
    // -------------------------------------------------------------------------
    [[nodiscard]] uint32_t getResource(Cell const cell) const
    {
        Chunk const* const chunk = findChunk(cell);
        return (chunk == nullptr) ? 0u : chunk->cells[cellIndex(cell)];
    }

    // -------------------------------------------------------------------------
    //! \brief Sum resources over a building footprint.
    //!
    //! \param[in] centre Cell at the footprint center.
    //! \param[in] radius Footprint reach as taxicab distance.
    //!     Zero means the center cell only. See CellsInRadius.
    //! \param[in] region Cells owned by the calling City.
    //!     Cells outside the region are skipped.
    //! \return Sum, capped at UINT32_MAX.
    // -------------------------------------------------------------------------
    [[nodiscard]] uint32_t getResource(Cell const centre,
                                       uint32_t radius,
                                       CellRegion const& region) const
    {
        // A rule of a layer stands on the cell it reads, and does so on every
        // cell of the region: worth not walking a one element diamond, and
        // worth being inlined into the caller.
        if (radius == 0u)
            return region.contains(centre) ? getResource(centre) : 0u;

        return sumInRadius(centre, radius, region);
    }

    // -------------------------------------------------------------------------
    //! \brief Count cells in a footprint inside a region.
    //!
    //! getResource() over a radius sums that many cells.
    //! This turns cell capacity into the max value a rule can read.
    //! Rules that compare proportions need both values.
    //!
    //! \param[in] centre Cell at the footprint center.
    //! \param[in] radius Footprint reach.
    //! \param[in] region Cells owned by the calling City.
    //! \return Cell count. Zero if the center lies outside the region.
    // -------------------------------------------------------------------------
    uint32_t countCellsInRadius(Cell const centre,
                                uint32_t const radius,
                                CellRegion const& region) const
    {
        if (radius == 0u)
            return region.contains(centre) ? 1u : 0u;

        return walkCellsInRadius(centre, radius, region);
    }

    // -------------------------------------------------------------------------
    //! \return Max amount one cell can hold, from the layer type.
    // -------------------------------------------------------------------------
    [[nodiscard]] uint32_t getCellCapacity() const
    {
        return m_type.capacity;
    }

    // -------------------------------------------------------------------------
    //! \brief Add to one cell, up to capacity. Overflow is dropped.
    //! \param[in] cell Target cell.
    //! \param[in] toAdd Amount to add.
    // -------------------------------------------------------------------------
    void addResource(Cell const cell, uint32_t const toAdd)
    {
        uint32_t amount = getResource(cell);

        // Avoid integer overflow
        if (amount >= Resource::MAX_CAPACITY - toAdd)
            amount = Resource::MAX_CAPACITY;
        else
            amount += toAdd;

        setResource(cell, amount);
    }

    // -------------------------------------------------------------------------
    //! \brief Add resources to cells in a footprint.
    //!
    //! \param[in] centre Cell at the footprint center.
    //! \param[in] radius Footprint reach.
    //! \param[in] region Cells owned by the calling City.
    //!     Cells outside get nothing.
    //! \param[in] toAdd When \c distributed is true, total amount for the
    //! footprint.
    //!     When false, amount for each cell.
    //! \param[in] distributed If true, walk cells in random order and take from
    //! \c toAdd.
    //!     The footprint receives \c toAdd in total. Last cells may get
    //!     nothing. If false, give \c toAdd to every cell.
    //! \note Each cell is capped at getCellCapacity().
    //!     A full footprint absorbs nothing. The rest is dropped.
    // -------------------------------------------------------------------------
    void addResource(Cell const centre,
                     uint32_t const radius,
                     CellRegion const& region,
                     uint32_t const toAdd,
                     bool distributed = true)
    {
        if (radius == 0u)
        {
            if (region.contains(centre))
                addResource(centre, toAdd);
            return;
        }

        addResourceInRadius(centre, radius, region, toAdd, distributed);
    }

    // -------------------------------------------------------------------------
    //! \brief Remove from one cell, down to zero. The cell never goes negative.
    //! \param[in] cell Target cell.
    //! \param[in] toRemove Amount to remove.
    // -------------------------------------------------------------------------
    void removeResource(Cell const cell, uint32_t const toRemove)
    {
        uint32_t const amount = getResource(cell);

        setResource(cell, (amount > toRemove) ? (amount - toRemove) : 0u);
    }

    // -------------------------------------------------------------------------
    //! \brief Remove resources from cells in a footprint.
    //! Mirror of addResource(Cell, uint32_t, CellRegion const&, uint32_t,
    //! bool). Removes down to zero instead of adding up to capacity.
    //!
    //! \param[in] centre Cell at the footprint center.
    //! \param[in] radius Footprint reach.
    //! \param[in] region Cells owned by the calling City.
    //! \param[in] toRemove When \c distributed is true, total amount to take.
    //!     When false, amount from each cell.
    //! \param[in] distributed Same meaning as in addResource().
    // -------------------------------------------------------------------------
    void removeResource(Cell const centre,
                        uint32_t const radius,
                        CellRegion const& region,
                        uint32_t const toRemove,
                        bool distributed = true)
    {
        if (radius == 0u)
        {
            if (region.contains(centre))
                removeResource(centre, toRemove);
            return;
        }

        removeResourceInRadius(centre, radius, region, toRemove, distributed);
    }

    // -------------------------------------------------------------------------
    //! \brief Convert a cell to world coordinates.
    //! \param[in] cell Cell to locate.
    //! \return World position of the cell's top-left corner.
    //!     The renderer draws a rectangle from this point.
    // -------------------------------------------------------------------------
    Vector3f cellToWorld(Cell cell) const;

    // -------------------------------------------------------------------------
    //! \brief Run the layer's rules over every City's region. Increment the
    //! tick count.
    //!
    //! A cell shared by two Cities is walked once per City.
    //! Rules read and write on behalf of each city.
    //!
    //! \param[in] cities Cities in the World. Each contributes its region.
    // -------------------------------------------------------------------------
    void executeRules(Cities const& cities);

    // -------------------------------------------------------------------------
    //! \return Layer type name, such as "Water".
    // -------------------------------------------------------------------------
    [[nodiscard]] Name const& getTypeName() const
    {
        return m_type.name;
    }

    // -------------------------------------------------------------------------
    //! \return The layer type recipe.
    //!     Use it to add the same layer kind to another City without a name
    //!     lookup.
    // -------------------------------------------------------------------------
    [[nodiscard]] LayerType const& getType() const
    {
        return m_type;
    }

    // -------------------------------------------------------------------------
    //! \return Layer color as 0xRRGGBB.
    //!     The demo shades each cell by how full it is.
    // -------------------------------------------------------------------------
    [[nodiscard]] uint32_t getColor() const
    {
        return m_type.color;
    }

    // -------------------------------------------------------------------------
    //! \return Cell side length in world units. Comes from the World.
    //!     All layers in a world align.
    // -------------------------------------------------------------------------
    [[nodiscard]] float getCellSize() const;

    // -------------------------------------------------------------------------
    //! \return Rules attached to this layer, from its type.
    //!     The demo reads them to show layer activity.
    // -------------------------------------------------------------------------
    [[nodiscard]] std::vector<RuleLayer*> const& getRules() const
    {
        return m_type.rules;
    }

    // -------------------------------------------------------------------------
    //! \return Tick count for this layer.
    //!     A rule with rate 20 fires when this is a multiple of 20.
    // -------------------------------------------------------------------------
    [[nodiscard]] uint32_t getTicks() const
    {
        return m_ticks;
    }

    // -------------------------------------------------------------------------
    //! \return Sum of all allocated cells.
    //!     Uses cached block totals. Does not walk the grid.
    // -------------------------------------------------------------------------
    [[nodiscard]] uint64_t getTotalResource() const;

    // -------------------------------------------------------------------------
    //! \return Number of allocated blocks.
    //!     Shown in the debug panel to expose memory cost.
    //!     Each block is CHUNK_SIZE x CHUNK_SIZE cells.
    // -------------------------------------------------------------------------
    [[nodiscard]] size_t getBlockCount() const
    {
        return m_chunks.size();
    }

    // -------------------------------------------------------------------------
    //! \brief Call \c function(u, v, amount) for every non-empty cell.
    //!
    //! The renderer uses this to draw a layer without knowing cell locations.
    //! Empty cells are skipped. Cost follows stored data, not world size.
    //!
    //! \param[in] function Callable as f(int32_t, int32_t, uint32_t).
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
    //! \brief Call \c function(u, v, side, mean) for each non-empty square in a
    //! region.
    //! \c mean is the average amount over \c side x \c side cells.
    //!
    //! The renderer uses this to draw a layer.
    //! side 1 draws cell by cell. A larger side draws a coarser view with fewer
    //! rectangles. This keeps large cities affordable to draw.
    //! The caller picks \c side from the zoom level.
    //!
    //! Blocks outside the region are skipped.
    //! Empty squares are skipped.
    //! Cost follows visible data, not world size.
    //!
    //! \param[in] region Cells to visit. An empty region visits nothing.
    //! \param[in] side Square size in cells. Clamped to [1..CHUNK_SIZE].
    //!     Rounded down to a divisor of CHUNK_SIZE.
    //!     A square never spans two blocks.
    //! \param[in] function Callable as f(int32_t, int32_t, int32_t, uint32_t).
    // -------------------------------------------------------------------------
    template <class Function>
    void forEachBlockInRegion(CellRegion const& region,
                              int32_t side,
                              Function function) const
    {
        if (region.isEmpty())
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
                (chunk.u0 >= region.getMaxU()) ||
                (chunk.v0 >= region.getMaxV()))
            {
                continue;
            }

            forEachBlockOfChunk(chunk, region, side, function);
        }
    }

private:

    //==========================================================================
    //! \brief Square block of cells. The sparse layer allocates these as
    //! blocks.
    //==========================================================================
    struct Chunk
    {
        //! \brief Column of the top-left cell. A multiple of CHUNK_SIZE.
        int32_t u0 = 0;

        //! \brief Row of the top-left cell. A multiple of CHUNK_SIZE.
        int32_t v0 = 0;

        //! \brief Sum of all cells. Updated on every write.
        //!     Panels request this every frame. Recomputing would walk the
        //!     grid.
        uint64_t total = 0u;

        //! \brief Cells in row-major order.
        //!     Cell (u0 + du, v0 + dv) is at index dv * CHUNK_SIZE + du.
        //!     Zero-initialized.
        std::array<uint32_t, size_t(CHUNK_SIZE* CHUNK_SIZE)> cells{};
    };

    //--------------------------------------------------------------------------
    //! \brief Call f(u, v, amount) for non-empty cells in one block.
    //! \param[in] chunk Block to walk.
    //! \param[in] function Callable as f(int32_t, int32_t, uint32_t).
    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    //! \brief Walk one block for forEachBlockInRegion(), after overlap check.
    //! \param[in] chunk Block to walk.
    //! \param[in] region Cells to keep. Block cells outside are excluded from
    //! the average.
    //! \param[in] side Square size in cells. A divisor of CHUNK_SIZE.
    //! \param[in] function Callable as f(int32_t, int32_t, int32_t, uint32_t).
    //--------------------------------------------------------------------------
    template <class Function>
    static void forEachBlockOfChunk(Chunk const& chunk,
                                    CellRegion const& region,
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
    //! \brief Sum cells in one square that lie inside a region.
    //! \param[in] chunk Block containing the square.
    //! \param[in] region Cells to include.
    //! \param[in] du, dv Top-left corner of the square inside the block.
    //! \param[in] side Square size in cells.
    //! \param[out] sum Total amount in included cells.
    //! \param[out] counted Number of included cells.
    //--------------------------------------------------------------------------
    static void sumSquare(Chunk const& chunk,
                          CellRegion const& region,
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
                if (!region.contains(Cell{ chunk.u0 + u, chunk.v0 + v }))
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
    // the city, on every tick. They are defined here rather than in the source
    // file so that the caller can inline them, and they use masks rather than
    // divisions, which a side that is a power of two makes exact.
    static_assert((CHUNK_SIZE & (CHUNK_SIZE - 1)) == 0,
                  "the side of a block has to be a power of two");

    //! \brief Bit mask for coordinates inside a block.
    static constexpr int32_t CHUNK_MASK = CHUNK_SIZE - 1;

    //! \brief Shift to get a block index from a coordinate.
    //!     Equal to log2(CHUNK_SIZE).
    static constexpr int32_t CHUNK_SHIFT = 4;
    static_assert((1 << CHUNK_SHIFT) == CHUNK_SIZE,
                  "CHUNK_SHIFT has to be the logarithm of CHUNK_SIZE");

    //--------------------------------------------------------------------------
    //! \brief Round a coordinate down to its block origin.
    //!
    //! Rounds toward negative infinity, not zero.
    //! Cells at -1 and 0 land in different blocks.
    //! Clearing low bits of a two's complement integer does this without a
    //! branch.
    //!
    //! \param[in] coordinate Column or row.
    //! \return Nearest multiple of CHUNK_SIZE at or below the coordinate.
    //--------------------------------------------------------------------------
    static int32_t chunkOrigin(int32_t const coordinate)
    {
        return coordinate & ~CHUNK_MASK;
    }

    //--------------------------------------------------------------------------
    //! \brief Pack a block origin into one lookup key.
    //! \param[in] cell Cell to locate.
    //! \return Key for its block.
    //--------------------------------------------------------------------------
    static int64_t chunkKey(Cell const cell)
    {
        // Shifting a negative number right rounds towards minus infinity on
        // every compiler this builds with, and C++20 requires it, which is the
        // same rounding chunkOrigin() uses.
        int64_t const cu = cell.u >> CHUNK_SHIFT;
        int64_t const cv = cell.v >> CHUNK_SHIFT;

        return (cu << 32) ^ (cv & 0xFFFFFFFF);
    }

    //--------------------------------------------------------------------------
    //! \brief Index of a cell inside Chunk::cells.
    //! \param[in] cell Cell to locate.
    //! \return Index in Chunk::cells.
    //--------------------------------------------------------------------------
    static size_t cellIndex(Cell const cell)
    {
        int32_t const du = cell.u & CHUNK_MASK;
        int32_t const dv = cell.v & CHUNK_MASK;

        return size_t(dv * CHUNK_SIZE + du);
    }

    //--------------------------------------------------------------------------
    //! \brief Find the block for a cell. Read-only. Does not allocate.
    //! \param[in] cell Cell to locate.
    //! \return Block pointer, or nullptr if nothing was ever written nearby.
    //!
    //! Caches the last lookup. One rule may read the same cell several times.
    //! Neighboring cells often share a block.
    //--------------------------------------------------------------------------
    [[nodiscard]] Chunk const* findChunk(Cell const cell) const
    {
        int64_t const key = chunkKey(cell);

        if (m_cacheFilled && (m_cachedKey == key))
            return m_cachedChunk;

        return lookupChunk(key);
    }

    //--------------------------------------------------------------------------
    //! \brief Writable findChunk() for callers that clear cells without
    //! creating blocks.
    //! \param[in] cell Cell to locate.
    //! \return Block pointer, or nullptr.
    //--------------------------------------------------------------------------
    [[nodiscard]] Chunk* findChunk(Cell const cell)
    {
        // This Layer is not const here, so neither are the blocks it owns: only
        // the memoisation, which reading a cell has to fill too, made the
        // other overload the const one.
        return const_cast<Chunk*>(
            static_cast<Layer const*>(this)->findChunk(cell));
    }

    //--------------------------------------------------------------------------
    //! \brief Find or create the block for a cell.
    //! \param[in] cell Cell to locate.
    //! \return Block reference. Allocates and zero-fills a new block if needed.
    //--------------------------------------------------------------------------
    Chunk& chunkFor(Cell const cell)
    {
        int64_t const key = chunkKey(cell);

        if (m_cacheFilled && (m_cachedKey == key) && (m_cachedChunk != nullptr))
            return *findChunk(cell);

        return createChunk(key, cell);
    }

    //--------------------------------------------------------------------------
    //! \brief Table lookup on cache miss.
    //! \param[in] key Block key from chunkKey().
    //! \return Block pointer, or nullptr. Updates the cache.
    //--------------------------------------------------------------------------
    Chunk const* lookupChunk(int64_t const key) const;

    //--------------------------------------------------------------------------
    //! \brief Table lookup or create on cache miss.
    //! \param[in] key Block key from chunkKey().
    //! \param[in] cell Any cell in the block. Gives the origin for a new block.
    //! \return Block reference. Updates the cache.
    //--------------------------------------------------------------------------
    Chunk& createChunk(int64_t const key, Cell const cell);

    //--------------------------------------------------------------------------
    //! \brief Sum resources over a multi-cell footprint. Called by
    //! getResource(). Same parameters and result.
    //! \note Const but uses mutable CellsInRadius to avoid allocation per rule
    //! call.
    //--------------------------------------------------------------------------
    uint32_t sumInRadius(Cell const centre,
                         uint32_t const radius,
                         CellRegion const& region) const;

    //--------------------------------------------------------------------------
    //! \brief Count cells over a multi-cell footprint. Called by
    //! countCellsInRadius(). Same parameters and result.
    //--------------------------------------------------------------------------
    uint32_t walkCellsInRadius(Cell const centre,
                               uint32_t const radius,
                               CellRegion const& region) const;

    //--------------------------------------------------------------------------
    //! \brief Add resources over a multi-cell footprint. Called by
    //! addResource(). Same parameters.
    //--------------------------------------------------------------------------
    void addResourceInRadius(Cell const centre,
                             uint32_t const radius,
                             CellRegion const& region,
                             uint32_t const toAdd,
                             bool const distributed);

    //--------------------------------------------------------------------------
    //! \brief Remove resources over a multi-cell footprint. Called by
    //! removeResource(). Same parameters.
    //--------------------------------------------------------------------------
    void removeResourceInRadius(Cell const centre,
                                uint32_t const radius,
                                CellRegion const& region,
                                uint32_t const toRemove,
                                bool const distributed);

    //--------------------------------------------------------------------------
    //! \brief Run one rule over one City's region, cell by cell.
    //! \param[in] rule Rule to run.
    //! \param[in] city City whose region bounds the walk.
    //!     The rule reads and writes on behalf of this city.
    //--------------------------------------------------------------------------
    void executeRule(RuleLayer& rule, City& city);

    //--------------------------------------------------------------------------
    //! \brief Move a share of every cell to its neighbours and lose another
    //! share.
    //!
    //! See LayerType::diffusion and LayerType::decay. Called by executeRules()
    //! on the layer period, and only when LayerType::spreads() is true.
    //!
    //! The pass reads the state before it and writes the state after it, which
    //! is why it copies the amounts first. Reading the grid it is writing would
    //! carry an amount several blocks in one pass, in the direction the sweep
    //! happens to run.
    //!
    //! The pass ignores the City regions. Smoke crosses a city border, and the
    //! grid belongs to the World, not to a City.
    //--------------------------------------------------------------------------
    void spreadAndFade();

    //--------------------------------------------------------------------------
    //! \brief Copy the amounts spreadAndFade() reads, and list the blocks it
    //! writes to.
    //!
    //! Empty blocks are left out of the copy: they read as zero anyway, and a
    //! large grid holds many of them. The blocks around a used one are listed
    //! all the same, because a cell on a border gives to a cell of the next
    //! block, which may not exist yet.
    //!
    //! Fills m_previous and m_spreadBlocks.
    //--------------------------------------------------------------------------
    void collectBlocksToSpread();

    //--------------------------------------------------------------------------
    //! \brief The amount one cell holds once the current spreadAndFade() pass
    //! ends.
    //! \param[in] cell Cell to compute.
    //! \return What the cell keeps plus what its four neighbours give it.
    //--------------------------------------------------------------------------
    [[nodiscard]] uint32_t nextAmount(Cell const cell) const;

    //--------------------------------------------------------------------------
    //! \brief The amount one cell held before the current spreadAndFade() pass.
    //! \param[in] cell Cell to read.
    //! \return The copied amount. Zero outside the copied blocks.
    //--------------------------------------------------------------------------
    [[nodiscard]] uint32_t previousAmount(Cell const cell) const;

private:

    //! \brief Layer type recipe. Shared by all layers of this kind.
    LayerType const& m_type;

    //! \brief World that owns the grid. Provides cell size.
    World& m_world;

    //! \brief Rule execution context: city, cell, layer.
    //!     Reused across thousands of cells per tick.
    RuleContext m_context;

    //! \brief Sparse cell storage. Blocks allocate on demand. Keyed by
    //! chunkKey().
    std::unordered_map<int64_t, Chunk> m_chunks;

    //! \brief Cells as they stood before the current spreadAndFade() pass.
    //!     Holds the non-empty blocks only. Kept between passes so that a layer
    //!     which diffuses does not allocate on every period. Empty on a layer
    //!     that does not diffuse.
    std::unordered_map<int64_t,
                       std::array<uint32_t, size_t(CHUNK_SIZE* CHUNK_SIZE)>>
        m_previous;

    //! \brief Blocks one spreadAndFade() pass has to write: the non-empty ones
    //!     and the neighbours an amount may reach. Kept between passes to reuse
    //!     its memory.
    std::vector<Cell> m_spreadBlocks;

    //! \brief Key from the last block lookup. Valid when m_cacheFilled is true.
    mutable int64_t m_cachedKey = 0;

    //! \brief Block from the last lookup, or nullptr.
    //!     Blocks are never removed. Map pointers stay valid for the Layer
    //!     lifetime.
    mutable Chunk const* m_cachedChunk = nullptr;

    //! \brief Reusable footprint walker. Avoids allocation on reads.
    //!     Mutable so reads stay const.
    mutable CellsInRadius m_coordinates;

    //! \brief Reusable random-order footprint walker for uneven distribution.
    RandomCells m_randomCoordinates;

    // The two small members come last, together, so that neither leaves a gap
    // in front of the eight-byte members above.

    //! \brief Tick count. Rule rates compare against this value.
    uint32_t m_ticks = 0u;

    //! \brief True when the cache holds a valid entry.
    //!     Grid sweeps reuse the same block for many consecutive cells.
    mutable bool m_cacheFilled = false;
};

//! \brief Map of layers in a World, keyed by name. The World owns them.
using Layers = std::map<std::string, std::unique_ptr<Layer>, std::less<>>;

} // namespace ogb

#endif
