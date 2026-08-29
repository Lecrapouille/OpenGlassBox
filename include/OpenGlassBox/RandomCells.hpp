//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file RandomCells.hpp
//! \brief Draw a share of the cells of a Layer, spread over the whole of it.

#ifndef OPEN_GLASSBOX_RANDOMCELLS_HPP
#define OPEN_GLASSBOX_RANDOMCELLS_HPP

#include <cstdint>

namespace ogb
{

//==============================================================================
//! \brief Hands out a given number of cells of a rectangle, drawn at random,
//! each at most once, in reading order.
//!
//! A layer rule that only acts on part of its cells has to pick which ones, and
//! taking the first ones in reading order would draw a front sweeping the layer
//! from one corner: grass would grow in stripes. What the rule wants is a
//! sample spread over the whole layer, and every subset of the size asked for has
//! to be equally likely.
//!
//! What this does not have to be is a sample handed out in a random *order*.
//! The rule is applied to each drawn cell on its own, so only the set matters,
//! and a set handed out in reading order is a set read along the rows of the
//! blocks the Layer stores its cells in. Handing them out shuffled instead costs
//! a cache miss on every cell, on a grid that is megabytes wide.
//!
//! So the rectangle is scanned once, in order, and each cell is taken with the
//! probability that makes the sample come out right: \c wanted out of the cells
//! that are left. That is selection sampling, and it draws exactly the number
//! asked for, uniformly, in one pass, holding nothing but a handful of
//! integers. The previous implementation held two vectors as large as the layer
//! and copied one into the other on every run.
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
//! The matching script, where the percentage is what makes only part of the layer
//! move on each run:
//! \code
//! layerRule CreateGrass
//!     rate 20 minutes
//!     layer Water remove 10 randomTilesPercent 90
//!     layer Grass add 1
//! end
//! \endcode
//!
//! \note The draw is not reproducible from one run of the program to the next:
//! the generator is seeded from the system.
//==============================================================================
class RandomCells
{
public:

    RandomCells();

    //--------------------------------------------------------------------------
    //! \brief Start a fresh draw over a rectangle of that size.
    //!
    //! Called at the start of every run of a layer rule. Allocates nothing, so a
    //! rule firing on every tick costs no memory traffic beyond the cells it
    //! actually visits. Whatever was left of a previous draw is forgotten.
    //!
    //! \param[in] layerSizeU number of columns.
    //! \param[in] layerSizeV number of rows.
    //! \param[in] wanted how many cells to hand out. Clamped to the number of
    //! cells the rectangle holds.
    //!
    //! \note A rectangle of more than four billion cells is scanned in part
    //! only. No grid that large fits in memory in the first place.
    //--------------------------------------------------------------------------
    void init(uint32_t layerSizeU, uint32_t layerSizeV, uint64_t wanted);

    // -------------------------------------------------------------------------
    //! \brief Hand out the next drawn cell.
    //! \param[out] u column of the cell, in [0..layerSizeU[. Zero when there is
    //! nothing left.
    //! \param[out] v row of the cell, in [0..layerSizeV[. Zero when there is
    //! nothing left.
    //! \return true when a cell was handed out, false once the whole sample has
    //! been handed out.
    // -------------------------------------------------------------------------
    bool next(uint32_t& u, uint32_t& v);

private:

    //--------------------------------------------------------------------------
    //! \brief \return the next number of the generator. A xorshift, which is a
    //! few instructions rather than the hundreds a Mersenne twister spends
    //! refilling its state, and far better than good enough to decide which
    //! cells grass grows on.
    //--------------------------------------------------------------------------
    uint32_t random();

    //--------------------------------------------------------------------------
    //! \brief \param[in] bound one past the largest value wanted, never zero.
    //! \return a number in [0..bound[, by the multiply and shift of Lemire:
    //! no division, and a bias too small to be worth a rejection loop.
    //--------------------------------------------------------------------------
    uint32_t random(uint32_t bound)
    {
        return uint32_t((uint64_t(random()) * uint64_t(bound)) >> 32);
    }

private:

    //! \brief State of the generator, never zero.
    uint64_t m_state = 0u;
    //! \brief Number of columns of the rectangle being scanned.
    uint32_t m_sizeU = 0u;
    //! \brief Column of the cell the scan has reached.
    uint32_t m_u = 0u;
    //! \brief Row of the cell the scan has reached.
    uint32_t m_v = 0u;
    //! \brief How many cells are still to be handed out.
    uint32_t m_wanted = 0u;
    //! \brief How many cells the scan has not reached yet, this one included.
    //! The draw takes the current cell with probability m_wanted / m_left,
    //! which is what makes every sample of the size asked for equally likely.
    uint32_t m_left = 0u;
};

} // namespace ogb

#endif
