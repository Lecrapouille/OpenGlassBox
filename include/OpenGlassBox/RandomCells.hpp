//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file RandomCells.hpp
//! \brief Pick a share of layer cells at random, spread over the whole layer.

#ifndef OPEN_GLASSBOX_RANDOMCELLS_HPP
#define OPEN_GLASSBOX_RANDOMCELLS_HPP

#include <cstdint>

namespace ogb
{

//==============================================================================
//! \brief Pick a fixed number of cells from a rectangle, each at most once, in
//! reading order.
//!
//! A layer rule that acts on part of the cells must choose which ones. Taking
//! the first cells in order would sweep from one corner and create stripes. The
//! rule needs a sample spread over the layer. Every subset of the requested
//! size must be equally likely.
//!
//! The order of the sample does not matter. The rule runs on each cell alone,
//! so only the set matters. Reading order follows the rows of the layer blocks.
//! Shuffling would cause a cache miss on every cell on a large grid.
//!
//! The rectangle is scanned once in order. Each cell is taken with the
//! probability that gives the right sample size: \c wanted out of the cells
//! left. This is selection sampling. It picks exactly the requested count in
//! one pass and stores only a few integers.
//!
//! Example:
//! \code
//! RandomCells walk;
//! walk.init(sizeU, sizeV, rule.percent(uint64_t(sizeU) * sizeV));
//!
//! uint32_t u, v;
//! while (walk.next(u, v))
//! {
//!     // apply the rule on the cell (u, v)
//! }
//! \endcode
//!
//! Matching script:
//! \code
//! layerRule CreateGrass
//!     rate 20 minutes
//!     layer Water remove 10 randomTilesPercent 90
//!     layer Grass add 1
//! end
//! \endcode
//!
//! \note The draw is not reproducible across program runs: the generator uses
//! the system clock.
//==============================================================================
class RandomCells
{
public:

    RandomCells();

    //--------------------------------------------------------------------------
    //! \brief Start a new draw over a rectangle of that size.
    //!
    //! Called at the start of every layer rule run. Allocates nothing. A rule
    //! on every tick pays no extra memory beyond the cells it visits. Any
    //! previous draw is forgotten.
    //!
    //! \param[in] layerSizeU number of columns.
    //! \param[in] layerSizeV number of rows.
    //! \param[in] wanted how many cells to return. Clamped to the rectangle
    //! size.
    //!
    //! \note A rectangle with more than four billion cells is only partly
    //! scanned. No grid that large fits in memory.
    //--------------------------------------------------------------------------
    void init(uint32_t layerSizeU, uint32_t layerSizeV, uint64_t wanted);

    // -------------------------------------------------------------------------
    //! \brief Return the next picked cell.
    //! \param[out] u column, in [0..layerSizeU[. Zero when nothing remains.
    //! \param[out] v row, in [0..layerSizeV[. Zero when nothing remains.
    //! \return true when a cell was returned, false when the sample is done.
    // -------------------------------------------------------------------------
    bool next(uint32_t& u, uint32_t& v);

private:

    //--------------------------------------------------------------------------
    //! \return the next random number. Uses xorshift: fast and enough for
    //! picking grass cells.
    //--------------------------------------------------------------------------
    uint32_t random();

    //--------------------------------------------------------------------------
    //! \param[in] bound one past the largest value, never zero.
    //! \return a number in [0..bound[ using Lemire multiply-and-shift: no
    //! division.
    //--------------------------------------------------------------------------
    uint32_t random(uint32_t bound)
    {
        return uint32_t((uint64_t(random()) * uint64_t(bound)) >> 32);
    }

private:

    //! \brief Generator state, never zero.
    uint64_t m_state = 0u;
    //! \brief Number of columns in the rectangle.
    uint32_t m_sizeU = 0u;
    //! \brief Current column in the scan.
    uint32_t m_u = 0u;
    //! \brief Current row in the scan.
    uint32_t m_v = 0u;
    //! \brief How many cells remain to return.
    uint32_t m_wanted = 0u;
    //! \brief How many cells remain in the scan, including the current one.
    //! The current cell is taken with probability m_wanted / m_left.
    uint32_t m_left = 0u;
};

} // namespace ogb

#endif
