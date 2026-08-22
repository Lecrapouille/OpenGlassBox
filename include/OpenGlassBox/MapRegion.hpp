//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#ifndef OPEN_GLASSBOX_MAP_REGION_HPP
#  define OPEN_GLASSBOX_MAP_REGION_HPP

#  include <cstdint>

namespace ogb {

//==============================================================================
//! \brief A rectangle of cells on the world grid, in cell coordinates.
//!
//! The world grid is unbounded, so anything that has to walk cells needs to be
//! told which ones. A City hands out the region it administers, and that is
//! what bounds the rules and the actions of its Units.
//==============================================================================
struct MapRegion
{
    //! \brief Coordinates of the first cell, included.
    int32_t u0 = 0;
    int32_t v0 = 0;
    //! \brief Number of cells along each axis.
    uint32_t sizeU = 0u;
    uint32_t sizeV = 0u;

    //--------------------------------------------------------------------------
    //! \brief Coordinate just past the last cell, excluded.
    //--------------------------------------------------------------------------
    int32_t u1() const { return u0 + int32_t(sizeU); }
    int32_t v1() const { return v0 + int32_t(sizeV); }

    //--------------------------------------------------------------------------
    //! \brief Whether the cell belongs to this region.
    //--------------------------------------------------------------------------
    bool contains(int32_t u, int32_t v) const
    {
        return (u >= u0) && (u < u1()) && (v >= v0) && (v < v1());
    }

    //--------------------------------------------------------------------------
    //! \brief The cell of the region closest to the given one.
    //--------------------------------------------------------------------------
    void clamp(int32_t& u, int32_t& v) const
    {
        if (u < u0) { u = u0; } else if (u >= u1()) { u = u1() - 1; }
        if (v < v0) { v = v0; } else if (v >= v1()) { v = v1() - 1; }
    }

    //--------------------------------------------------------------------------
    //! \brief Number of cells.
    //--------------------------------------------------------------------------
    uint64_t area() const { return uint64_t(sizeU) * uint64_t(sizeV); }

    bool empty() const { return (sizeU == 0u) || (sizeV == 0u); }
};

} // namespace ogb

#endif
