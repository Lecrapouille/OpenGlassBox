//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file Map.hpp
//! \brief Grid-backed resource layer shared by cities on the world map.


#ifndef OPEN_GLASSBOX_MAP_HPP
#  define OPEN_GLASSBOX_MAP_HPP

#  include "OpenGlassBox/MapCoordinatesInsideRadius.hpp"
#  include "OpenGlassBox/MapRandomCoordinates.hpp"
#  include "OpenGlassBox/MapRegion.hpp"
#  include "OpenGlassBox/Rule.hpp"
#  include "OpenGlassBox/Vector.hpp"
#  include <array>
#  include <map>
#  include <memory>
#  include <unordered_map>

namespace ogb {

class City;
class World;

//! \brief Collection of City, declared here because a Map runs its rules over
//! the region of each of them.
using Cities = std::map<std::string, std::unique_ptr<City>>;

//==============================================================================
//! \brief A Map represents a single type of resource in the environment (coal,
//! oil, forest but also air pollution, land value, desirability ...). Each cell
//! holds an amount of that resource, bounded by the capacity of the map type.
//! Units interact with maps through their footprint.
//!
//! The grid is that of the World, shared by every City and unbounded in the
//! four directions: cell coordinates are signed. Storage is sparse, by chunks
//! of CHUNK_SIZE x CHUNK_SIZE cells allocated the first time something is
//! written into them, so an empty map costs nothing and a city can be founded
//! anywhere without deciding a size in advance.
//==============================================================================
class Map
{
public:

    //! \brief Side of an allocation unit, in cells.
    static constexpr int32_t CHUNK_SIZE = 16;

    // -------------------------------------------------------------------------
    //! \brief Create an empty map. Every cell reads as zero until written to.
    // -------------------------------------------------------------------------
    Map(MapType const& type, World& world);
    VIRTUAL ~Map() = default;

    // -------------------------------------------------------------------------
    //! \brief Change the amount of resource at the cell (u,v), capped by the
    //! capacity of the map type. Writing zero into a cell of a chunk that was
    //! never touched allocates nothing.
    // -------------------------------------------------------------------------
    void setResource(int32_t const u, int32_t const v, uint32_t amount);

    // -------------------------------------------------------------------------
    //! \brief Get the amount of resource at the cell (u,v). Cells that were
    //! never written to read as zero.
    // -------------------------------------------------------------------------
    uint32_t getResource(int32_t const u, int32_t const v) const;

    // -------------------------------------------------------------------------
    //! \brief Sum of the resource held by the cells within the radius of (u,v)
    //! and inside the given region.
    // -------------------------------------------------------------------------
    uint32_t getResource(int32_t const u, int32_t const v, uint32_t radius,
                         MapRegion const& region);

    // -------------------------------------------------------------------------
    //! \brief Maximum amount a single cell may hold.
    // -------------------------------------------------------------------------
    uint32_t getCapacity() const { return m_type.capacity; }

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    void addResource(int32_t const u, int32_t const v, uint32_t toAdd);

    // -------------------------------------------------------------------------
    //! \brief Distribute the amount resource toAdd to the cells inside a circle
    //! and inside the given region.
    //!
    //! \param[in] distributed: if set to true cells are randomized and each
    //! distribution makes toAdd reduced. Therefore maybe not all cells are feed.
    //! If set to false, each cell gets the same amount of resource.
    //! \note Amount of resource for each cell are constrained by the global
    //! capacity of the map.
    // -------------------------------------------------------------------------
    void addResource(int32_t const u, int32_t const v, uint32_t const radius,
                     MapRegion const& region, uint32_t const toAdd,
                     bool distributed = true);

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    void removeResource(int32_t const u, int32_t const v, uint32_t toRemove);

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    void removeResource(int32_t const u, int32_t const v, uint32_t radius,
                        MapRegion const& region, uint32_t toRemove,
                        bool distributed = true);

    // -------------------------------------------------------------------------
    //! \brief Position, in world units, of the top-left corner of the cell.
    // -------------------------------------------------------------------------
    Vector3f getWorldPosition(int32_t const u, int32_t const v) const;

    // -------------------------------------------------------------------------
    //! \brief Run the rules of the map type over the region of every City, and
    //! count one tick. A cell is walked once per City that administers it.
    // -------------------------------------------------------------------------
    VIRTUAL void executeRules(Cities const& cities);

    // -------------------------------------------------------------------------
    //! \brief Getter: return the type of Map.
    // -------------------------------------------------------------------------
    std::string const& type() const { return m_type.name; }

    // -------------------------------------------------------------------------
    //! \brief Getter: return the type of Map.
    // -------------------------------------------------------------------------
    MapType const& getMapType() const { return m_type; }

    // -------------------------------------------------------------------------
    //! \brief Return the color for the renderer.
    // -------------------------------------------------------------------------
    uint32_t const& color() const { return m_type.color; }

    // -------------------------------------------------------------------------
    //! \brief Return the length of the side of a cell, in world units.
    // -------------------------------------------------------------------------
    float cellSize() const;

    // -------------------------------------------------------------------------
    //! \brief Rules this Map runs, from its type.
    // -------------------------------------------------------------------------
    std::vector<RuleMap*> const& rules() const { return m_type.rules; }

    // -------------------------------------------------------------------------
    //! \brief Number of ticks elapsed, which drives the rate of the rules.
    // -------------------------------------------------------------------------
    uint32_t ticks() const { return m_ticks; }

    // -------------------------------------------------------------------------
    //! \brief Sum of the resource held by every allocated cell.
    // -------------------------------------------------------------------------
    uint64_t totalResource() const;

    // -------------------------------------------------------------------------
    //! \brief Number of chunks currently allocated. Shown by the debugger to
    //! make the cost of a map visible.
    // -------------------------------------------------------------------------
    size_t allocatedChunks() const { return m_chunks.size(); }

    // -------------------------------------------------------------------------
    //! \brief Call the given function for every cell holding a non zero amount,
    //! as f(u, v, amount). This is how the renderer draws a map without knowing
    //! where its cells are.
    // -------------------------------------------------------------------------
    template<class Function>
    void forEachCell(Function function) const
    {
        for (auto const& it: m_chunks)
        {
            forEachCellOfChunk(it.second, function);
        }
    }

    // -------------------------------------------------------------------------
    //! \brief Same as forEachCell, restricted to the cells inside the region.
    //! Blocks that fall entirely outside are skipped without being read, which
    //! is what keeps the cost of drawing a map proportional to what is on
    //! screen rather than to the size of the world.
    // -------------------------------------------------------------------------
    template<class Function>
    void forEachCellInRegion(MapRegion const& region, Function function) const
    {
        if (region.empty())
            return;

        for (auto const& it: m_chunks)
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

            forEachCellOfChunk(chunk, [&](int32_t u, int32_t v, uint32_t amount) {
                if (region.contains(u, v))
                    function(u, v, amount);
            });
        }
    }

    // -------------------------------------------------------------------------
    //! \brief Call the given function for every allocated block holding
    //! something, as f(u0, v0, side, mean), where mean is the average amount
    //! over the block. Zoomed out, a cell is a fraction of a pixel and the
    //! renderer draws blocks instead: two hundred and fifty six times fewer
    //! rectangles for the same picture.
    // -------------------------------------------------------------------------
    template<class Function>
    void forEachChunk(Function function) const
    {
        for (auto const& it: m_chunks)
        {
            Chunk const& chunk = it.second;
            if (chunk.total == 0u)
                continue;
            function(chunk.u0, chunk.v0, CHUNK_SIZE,
                     uint32_t(chunk.total / uint64_t(CHUNK_SIZE * CHUNK_SIZE)));
        }
    }

private:

    //==========================================================================
    //! \brief A square block of cells, allocated as a whole.
    //==========================================================================
    struct Chunk
    {
        //! \brief Coordinates of the cell at the top-left of the chunk.
        int32_t u0 = 0;
        int32_t v0 = 0;
        //! \brief Sum of the cells, kept up to date by setResource. Walking the
        //! cells to get it again would cost the whole grid, and the panels ask
        //! for it on every frame.
        uint64_t total = 0u;
        std::array<uint32_t, size_t(CHUNK_SIZE * CHUNK_SIZE)> cells{};
    };

    //--------------------------------------------------------------------------
    //! \brief Call f(u, v, amount) for the non empty cells of one block.
    //--------------------------------------------------------------------------
    template<class Function>
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

    //--------------------------------------------------------------------------
    //! \brief Pack the coordinates of a chunk into a single key. Division is
    //! arithmetic, not truncating, so that negative coordinates land in the
    //! chunk below rather than sharing the one at zero.
    //--------------------------------------------------------------------------
    static int64_t chunkKey(int32_t const u, int32_t const v);
    static int32_t chunkOrigin(int32_t const coordinate);
    static size_t cellIndex(int32_t const u, int32_t const v);

    //--------------------------------------------------------------------------
    //! \brief The chunk holding the cell, or nullptr when it was never written.
    //--------------------------------------------------------------------------
    Chunk const* findChunk(int32_t const u, int32_t const v) const;

    //--------------------------------------------------------------------------
    //! \brief The chunk holding the cell, allocated if needed.
    //--------------------------------------------------------------------------
    Chunk& chunkFor(int32_t const u, int32_t const v);

    //--------------------------------------------------------------------------
    //! \brief Run one rule over the region of one city.
    //--------------------------------------------------------------------------
    void executeRule(RuleMap& rule, City& city);

private:

    MapType const& m_type;
    //! \brief The world owning the grid this map is laid on.
    World&         m_world;
    //! \brief Structure holding all information needed to execute simulation
    //! rules.
    RuleContext    m_context;
    //! \brief Frequency for running rules.
    uint32_t       m_ticks = 0u;
    //! \brief Cells, by blocks allocated on demand.
    std::unordered_map<int64_t, Chunk> m_chunks;
    //! \brief Cache coordinates within a position and radius.
    MapCoordinatesInsideRadius m_coordinates;
    //! \brief Cache random coordinates.
    MapRandomCoordinates       m_randomCoordinates;
};

using Maps = std::map<std::string, std::unique_ptr<Map>>;

} // namespace ogb

#endif
