//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file CellRegion.hpp
//! \brief Rectangle of grid cells.

#ifndef OPEN_GLASSBOX_CELL_REGION_HPP
#define OPEN_GLASSBOX_CELL_REGION_HPP

#include "OpenGlassBox/Vector.hpp"

namespace ogb
{

//==============================================================================
//! \brief A rectangle of cells on the world grid.
//!
//! The grid has no bounds, so anything that walks cells has to be told which
//! ones. A city hands out the cells it owns, and that bounds its layers, its
//! rules and the reach of its buildings. A zone hands out the cells the player
//! painted, and that bounds the buildings a zone may grow.
//!
//! Coordinates are signed. The rectangle is half open: \c u0 is inside and
//! \c getMaxU() is the first column past it, which makes a loop read like any
//! other.
//!
//! Example:
//! \code
//! ogb::CellRegion const& region = city.getRegion();
//! for (int32_t v = region.v0; v < region.getMaxV(); ++v)
//!     for (int32_t u = region.u0; u < region.getMaxU(); ++u)
//!         total += layer.getResource({ u, v });
//! \endcode
//==============================================================================
struct CellRegion
{
    //! \brief Column of the first cell, included.
    int32_t u0 = 0;

    //! \brief Row of the first cell, included.
    int32_t v0 = 0;

    //! \brief Number of columns.
    uint32_t sizeU = 0u;

    //! \brief Number of rows.
    uint32_t sizeV = 0u;

    //--------------------------------------------------------------------------
    //! \brief \return the column just past the last one, excluded.
    //--------------------------------------------------------------------------
    [[nodiscard]] int32_t getMaxU() const
    {
        return u0 + int32_t(sizeU);
    }

    //--------------------------------------------------------------------------
    //! \brief \return the row just past the last one, excluded.
    //--------------------------------------------------------------------------
    [[nodiscard]] int32_t getMaxV() const
    {
        return v0 + int32_t(sizeV);
    }

    //--------------------------------------------------------------------------
    //! \brief \param[in] cell the cell to test.
    //! \return true when that cell belongs to the rectangle.
    //--------------------------------------------------------------------------
    bool contains(Cell cell) const
    {
        return (cell.u >= u0) && (cell.u < getMaxU()) && (cell.v >= v0) &&
               (cell.v < getMaxV());
    }

    //--------------------------------------------------------------------------
    //! \brief Bring a cell inside the rectangle, along each axis separately.
    //!
    //! This is what a rule reaching past the edge of the city reads: the cell
    //! of the rectangle nearest to the one it asked for, rather than nothing.
    //!
    //! \param[in] cell the cell to bring back in.
    //! \return the cell itself when it is already inside, the nearest one
    //! otherwise. Meaningless on an empty rectangle.
    //--------------------------------------------------------------------------
    Cell clamp(Cell cell) const
    {
        if (cell.u < u0)
        {
            cell.u = u0;
        }
        else if (cell.u >= getMaxU())
        {
            cell.u = getMaxU() - 1;
        }
        if (cell.v < v0)
        {
            cell.v = v0;
        }
        else if (cell.v >= getMaxV())
        {
            cell.v = getMaxV() - 1;
        }
        return cell;
    }

    //--------------------------------------------------------------------------
    //! \brief \return how many cells the rectangle holds. Wide enough for the
    //! product of two large sides.
    //--------------------------------------------------------------------------
    [[nodiscard]] uint64_t getCellCount() const
    {
        return uint64_t(sizeU) * uint64_t(sizeV);
    }

    //--------------------------------------------------------------------------
    //! \brief \return true when the rectangle holds no cell at all, which is
    //! what a zone painted outside the city amounts to.
    //--------------------------------------------------------------------------
    [[nodiscard]] bool isEmpty() const
    {
        return (sizeU == 0u) || (sizeV == 0u);
    }
};

} // namespace ogb

#endif
