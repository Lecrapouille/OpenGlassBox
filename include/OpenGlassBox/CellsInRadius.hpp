//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file CellsInRadius.hpp
//! \brief Walk grid cells within reach of a building.

#ifndef OPEN_GLASSBOX_CELLSINRADIUS_HPP
#define OPEN_GLASSBOX_CELLSINRADIUS_HPP

#include <cstdint>
#include <map>
#include <vector>

namespace ogb
{

//==============================================================================
//! \brief Return grid cells within a given reach of a cell, clipped to the city
//! region.
//!
//! This is what \c layerRadius means for a building: how far its rules read
//! and write layers. The shape is a diamond (taxicab distance): a cell is kept
//! when the sum of the two axis distances is within the radius. A radius of one
//! gives the centre and four neighbours: { (0,0), (-1,0), (1,0), (0,-1),
//! (0,1) }.
//!
//! Offsets for each radius are computed once and cached for the whole program.
//! A large city asks this for every building on every tick. Only clipping and
//! translation to the centre run per call.
//!
//! The walk can start at a random offset instead of the first one. A rule that
//! adds one building's effect to a few cells needs this, or it would always hit
//! the same corner of the neighbourhood.
//!
//! Example:
//! \code
//! CellsInRadius around;
//! around.init(building.getLayerRadius(), building.getCell().u,
//! building.getCell().v,
//!             region.u0, region.getMaxU(), region.v0, region.getMaxV(),
//!             false);
//!
//! int32_t u, v;
//! while (around.next(u, v))
//! {
//!     layer.addResource({u, v}, 1u);
//! }
//! \endcode
//!
//! Matching script:
//! \code
//! building Work color 0x00AAFF layerRadius 3 rules [ ProduceGoods ]
//! \endcode
//==============================================================================
class CellsInRadius
{
public:

    //--------------------------------------------------------------------------
    //! \brief Offsets from the centre, two packed into one integer by
    //! compress().
    //--------------------------------------------------------------------------
    using RelativeCoordinates = std::vector<int32_t>;

    //--------------------------------------------------------------------------
    //! \brief Leaves everything unset. Call init() before next().
    //--------------------------------------------------------------------------
    CellsInRadius() = default;

    //--------------------------------------------------------------------------
    //! \brief Fix the random walk to a known sequence.
    //!
    //! Where a walk starts in the diamond decides which cell a Rule fills
    //! first, so it decides the state of the map. Without a fixed seed the
    //! sequence comes from the operating system and no run repeats another: a
    //! measurement of the ground after a few game hours then holds for one run
    //! only, which is a test that fails one time in ten and cannot be
    //! reproduced. Call this before a run that has to be reproducible.
    //!
    //! \param[in] seed the sequence to use.
    //--------------------------------------------------------------------------
    static void setSeed(uint32_t seed);

    //--------------------------------------------------------------------------
    //! \brief Start a walk around a cell.
    //!
    //! \param[in] radius reach in cells, as taxicab distance. Zero walks only
    //! the centre. Must not exceed MAX_RADIUS.
    //! \param[in] centerU column of the centre.
    //! \param[in] centerV row of the centre.
    //! \param[in] minU first allowed column, included.
    //! \param[in] maxU first column past the allowed range, excluded.
    //! \param[in] minV first allowed row, included.
    //! \param[in] maxV first row past the allowed range, excluded.
    //! \param[in] random true to start at a random cell in the diamond. Every
    //! cell is still returned exactly once; the walk wraps around.
    //--------------------------------------------------------------------------
    void init(uint32_t radius,
              int32_t centerU,
              int32_t centerV,
              int32_t minU,
              int32_t maxU,
              int32_t minV,
              int32_t maxV,
              bool random);

    //--------------------------------------------------------------------------
    //! \brief Return the next cell, skipping cells outside the bounds from
    //! init().
    //! \param[out] u column. Zero when nothing remains.
    //! \param[out] v row. Zero when nothing remains.
    //! \return true when a cell was returned, false when the diamond is done.
    //--------------------------------------------------------------------------
    bool next(int32_t& u, int32_t& v);

private:

    //--------------------------------------------------------------------------
    //! \brief Fill offsets for a diamond of that radius. Called once per
    //! radius; the result is cached by relativeCoordinates().
    //! \param[in] radius reach as taxicab distance.
    //! \param[out] coord the offsets, packed by compress().
    //--------------------------------------------------------------------------
    void createRelativeCoordinates(int32_t radius,
                                   RelativeCoordinates& coord) const;

    //--------------------------------------------------------------------------
    //! \brief Shared cache of offsets by radius.
    //! \param[in] radius reach to look up.
    //! \return offsets for that radius, empty on first call.
    //--------------------------------------------------------------------------
    static RelativeCoordinates& relativeCoordinates(uint32_t radius)
    {
        static std::map<uint32_t, RelativeCoordinates> coordinates;
        return coordinates[radius];
    }

    //--------------------------------------------------------------------------
    //! \brief Pack two small signed offsets into one integer.
    //! \param[in] u, v offsets from the centre, each within +/- MAX_RADIUS.
    //! \return the packed pair.
    //--------------------------------------------------------------------------
    static int32_t compress(int32_t u, int32_t v);

    //--------------------------------------------------------------------------
    //! \brief Undo compress().
    //! \param[in] val the packed pair.
    //! \param[out] u, v the two offsets.
    //--------------------------------------------------------------------------
    static void uncompress(int32_t val, int32_t& u, int32_t& v);

private:

    //! \brief Largest radius that fits in compress() packing.
    static constexpr int32_t MAX_RADIUS = 255;
    //! \brief Offsets for the current radius, owned by the cache.
    RelativeCoordinates* m_relativeCoord = nullptr;
    //! \brief Start index in the offsets, random or zero.
    uint32_t m_startingIndex;
    //! \brief How many offsets were returned or skipped so far.
    uint32_t m_offset;
    //! \brief Column of the centre.
    int32_t m_centerU;
    //! \brief Row of the centre.
    int32_t m_centerV;
    //! \brief First allowed column, included.
    int32_t m_minU;
    //! \brief First column past the allowed range, excluded.
    int32_t m_maxU;
    //! \brief First allowed row, included.
    int32_t m_minV;
    //! \brief First row past the allowed range, excluded.
    int32_t m_maxV;
};

} // namespace ogb

#endif
