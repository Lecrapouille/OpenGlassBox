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
//! The grid has no bounds. Code that walks cells needs a region.
//! A city returns the cells it owns. This limits its Layers, Rules, and Buildings.
//! A Zone returns the cells the player painted. This limits where Buildings may grow.
//!
//! Coordinates are signed. The rectangle is half-open: \c u0 is included and
//! \c getMaxU() is excluded. Loops use the usual \c u < getMaxU() pattern.
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
    //! \brief First column, included.
    int32_t u0 = 0;

    //! \brief First row, included.
    int32_t v0 = 0;

    //! \brief Number of columns.
    uint32_t sizeU = 0u;

    //! \brief Number of rows.
    uint32_t sizeV = 0u;

    //--------------------------------------------------------------------------
    //! \return the first column past the rectangle, excluded.
    //--------------------------------------------------------------------------
    [[nodiscard]] int32_t getMaxU() const
    {
        return u0 + int32_t(sizeU);
    }

    //--------------------------------------------------------------------------
    //! \return the first row past the rectangle, excluded.
    //--------------------------------------------------------------------------
    [[nodiscard]] int32_t getMaxV() const
    {
        return v0 + int32_t(sizeV);
    }

    //--------------------------------------------------------------------------
    //! \param[in] cell the cell to test.
    //! \return true if the cell is inside the rectangle.
    //--------------------------------------------------------------------------
    bool contains(Cell cell) const
    {
        return (cell.u >= u0) && (cell.u < getMaxU()) && (cell.v >= v0) &&
               (cell.v < getMaxV());
    }

    //--------------------------------------------------------------------------
    //! \brief Move a cell inside the rectangle, axis by axis.
    //!
    //! Used when a Rule reaches past the city edge. Returns the nearest cell
    //! inside the rectangle, not an empty result.
    //!
    //! \param[in] cell the cell to clamp.
    //! \return the cell if already inside, else the nearest cell inside.
    //! Result is undefined for an empty rectangle.
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
    //! \return the number of cells in the rectangle.
    //--------------------------------------------------------------------------
    [[nodiscard]] uint64_t getCellCount() const
    {
        return uint64_t(sizeU) * uint64_t(sizeV);
    }

    //--------------------------------------------------------------------------
    //! \return true if the rectangle has no cells.
    //! Happens when a Zone is painted outside the city.
    //--------------------------------------------------------------------------
    [[nodiscard]] bool isEmpty() const
    {
        return (sizeU == 0u) || (sizeV == 0u);
    }
};

} // namespace ogb

#endif
