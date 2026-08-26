//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file MapRegion.hpp
//! \brief Axis-aligned rectangle of grid cells.

#ifndef OPEN_GLASSBOX_MAP_REGION_HPP
#define OPEN_GLASSBOX_MAP_REGION_HPP

#include <cstdint>

namespace ogb
{

//==============================================================================
//! \brief A rectangle of cells on the world grid, in grid coordinates.
//!
//! The grid is unbounded in the four directions, so anything that has to walk
//! cells has to be told which ones. A City hands out the region it administers,
//! and that is what bounds its Maps, its rules and the reach of its buildings.
//! An Area hands out the footprint the player painted, and that is what bounds
//! the buildings a zone may grow.
//!
//! Coordinates are signed, and the rectangle is half open: \c u0 is inside,
//! \c u1() is the first column past it, which is what makes a loop read like
//! any other.
//!
//! Example:
//! \code
//! ogb::MapRegion const& region = city.region();
//! for (int32_t v = region.v0; v < region.v1(); ++v)
//!     for (int32_t u = region.u0; u < region.u1(); ++u)
//!         total += map.getResource(u, v);
//! \endcode
//==============================================================================
struct MapRegion
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
    int32_t u1() const
    {
        return u0 + int32_t(sizeU);
    }

    //--------------------------------------------------------------------------
    //! \brief \return the row just past the last one, excluded.
    //--------------------------------------------------------------------------
    int32_t v1() const
    {
        return v0 + int32_t(sizeV);
    }

    //--------------------------------------------------------------------------
    //! \brief \param[in] u, v the cell to test.
    //! \return true when that cell belongs to the rectangle.
    //--------------------------------------------------------------------------
    bool contains(int32_t u, int32_t v) const
    {
        return (u >= u0) && (u < u1()) && (v >= v0) && (v < v1());
    }

    //--------------------------------------------------------------------------
    //! \brief Move a cell inside the rectangle, along each axis separately.
    //!
    //! What a rule reaching past the edge of the city reads: the cell of the
    //! rectangle nearest to the one it asked for, rather than nothing at all.
    //!
    //! \param[in,out] u, v the cell to bring back in. Left as is when it is
    //! already inside. Meaningless on an empty rectangle.
    //--------------------------------------------------------------------------
    void clamp(int32_t& u, int32_t& v) const
    {
        if (u < u0)
        {
            u = u0;
        }
        else if (u >= u1())
        {
            u = u1() - 1;
        }
        if (v < v0)
        {
            v = v0;
        }
        else if (v >= v1())
        {
            v = v1() - 1;
        }
    }

    //--------------------------------------------------------------------------
    //! \brief \return the number of cells. Wide enough to hold the product of
    //! two large sides without wrapping around.
    //--------------------------------------------------------------------------
    uint64_t area() const
    {
        return uint64_t(sizeU) * uint64_t(sizeV);
    }

    //--------------------------------------------------------------------------
    //! \brief \return true when the rectangle holds no cell at all, which is
    //! what a zone painted outside the city amounts to.
    //--------------------------------------------------------------------------
    bool empty() const
    {
        return (sizeU == 0u) || (sizeV == 0u);
    }
};

} // namespace ogb

#endif
