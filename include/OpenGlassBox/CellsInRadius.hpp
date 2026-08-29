//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file CellsInRadius.hpp
//! \brief Walk the grid cells within reach of a building.

#ifndef OPEN_GLASSBOX_CELLSINRADIUS_HPP
#define OPEN_GLASSBOX_CELLSINRADIUS_HPP

#include <cstdint>
#include <map>
#include <vector>

namespace ogb
{

//==============================================================================
//! \brief Hands out the grid cells within a given reach of a given cell,
//! clipped to the region of the City.
//!
//! This is what \c layerRadius means for a building: how far around itself its
//! rules read and write the Layers. The shape is a diamond rather than a disc,
//! cells being kept when the sum of the two distances is within the radius,
//! which is the taxicab distance. A radius of one gives the cell itself and its
//! four neighbours: { (0,0), (-1,0), (1,0), (0,-1), (0,1) }.
//!
//! The offsets of a given radius are computed once and cached for the whole
//! program, keyed by radius, because a large city asks this question for every
//! building at every tick. Only the clipping and the translation to the centre
//! are paid per call.
//!
//! Walking can start at a random offset rather than at the first one, which is
//! what a rule adding a single unit of something to a limited number of cells
//! needs: starting at the first cell every time would always feed the same
//! corner of the neighbourhood.
//!
//! Example:
//! \code
//! CellsInRadius around;
//! around.init(unit.getLayerRadius(), unit.getCell().u, unit.getCell().v,
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
//! The matching script, where \c layerRadius is the reach walked here:
//! \code
//! unit Work color 0x00AAFF layerRadius 3 rules [ ProduceGoods ]
//! \endcode
//==============================================================================
class CellsInRadius
{
public:

    //--------------------------------------------------------------------------
    //! \brief Offsets to the centre, packed two to an integer by compress().
    //--------------------------------------------------------------------------
    using RelativeCoordinates = std::vector<int32_t>;

    //--------------------------------------------------------------------------
    //! \brief Leaves everything uninitialised: call init() before next().
    //--------------------------------------------------------------------------
    CellsInRadius() = default;

    //--------------------------------------------------------------------------
    //! \brief Start a walk around a cell.
    //!
    //! \param[in] radius reach, in cells, as a taxicab distance. Zero walks the
    //! centre alone. Must not exceed MAX_RADIUS.
    //! \param[in] centerU column of the centre.
    //! \param[in] centerV row of the centre.
    //! \param[in] minU first column allowed, included.
    //! \param[in] maxU first column past the ones allowed, excluded.
    //! \param[in] minV first row allowed, included.
    //! \param[in] maxV first row past the ones allowed, excluded.
    //! \param[in] random true to begin at a random cell of the diamond instead
    //! of the first one. Every cell is still handed out exactly once: the walk
    //! wraps around.
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
    //! \brief Hand out the next cell of the walk, skipping the ones clipped
    //! away by the bounds given to init().
    //! \param[out] u column of the cell. Zero when there is nothing left.
    //! \param[out] v row of the cell. Zero when there is nothing left.
    //! \return true when a cell was handed out, false once the whole diamond
    //! has been walked.
    //--------------------------------------------------------------------------
    bool next(int32_t& u, int32_t& v);

private:

    //--------------------------------------------------------------------------
    //! \brief Fill in the offsets of a diamond of that radius. Called once per
    //! radius, the answer being cached by relativeCoordinates().
    //! \param[in] radius reach, as a taxicab distance.
    //! \param[out] coord the offsets, packed by compress().
    //--------------------------------------------------------------------------
    void createRelativeCoordinates(int32_t radius, RelativeCoordinates& coord);

    //--------------------------------------------------------------------------
    //! \brief The cache of offsets, keyed by radius, shared by the whole
    //! program. A radius is asked for again at every tick, by every building.
    //! \param[in] radius reach to look up.
    //! \return the offsets of that radius, empty on the first call.
    //--------------------------------------------------------------------------
    static RelativeCoordinates& relativeCoordinates(uint32_t radius)
    {
        static std::map<uint32_t, RelativeCoordinates> coordinates;
        return coordinates[radius];
    }

    //--------------------------------------------------------------------------
    //! \brief Pack two small signed numbers into one, so that the cache is a
    //! flat vector of integers rather than a vector of pairs.
    //! \param[in] u, v offsets to the centre, each within +/- MAX_RADIUS.
    //! \return the two of them shifted into one integer.
    //--------------------------------------------------------------------------
    static int32_t compress(int32_t u, int32_t v);

    //--------------------------------------------------------------------------
    //! \brief Undo compress().
    //! \param[in] val the packed pair.
    //! \param[out] u, v the two offsets.
    //--------------------------------------------------------------------------
    static void uncompress(int32_t val, int32_t& u, int32_t& v);

private:

    //! \brief Largest reach that fits in the packing of compress().
    static constexpr int32_t MAX_RADIUS = 255;
    //! \brief Offsets of the current radius, owned by the cache.
    RelativeCoordinates* m_relativeCoord = nullptr;
    //! \brief Where in the offsets the walk begins, drawn at random or zero.
    uint32_t m_startingIndex;
    //! \brief How many offsets have been handed out or skipped so far.
    uint32_t m_offset;
    //! \brief Column of the centre.
    int32_t m_centerU;
    //! \brief Row of the centre.
    int32_t m_centerV;
    //! \brief First column allowed, included.
    int32_t m_minU;
    //! \brief First column past the ones allowed, excluded.
    int32_t m_maxU;
    //! \brief First row allowed, included.
    int32_t m_minV;
    //! \brief First row past the ones allowed, excluded.
    int32_t m_maxV;
};

} // namespace ogb

#endif
