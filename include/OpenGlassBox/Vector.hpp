//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file Vector.hpp
//! \brief World position, grid Cell, and basic math for the simulation.

#ifndef OPEN_GLASSBOX_VECTOR_HPP
#define OPEN_GLASSBOX_VECTOR_HPP

#include <cmath>
#include <cstdint>
#include <ostream>

namespace ogb
{

//==============================================================================
//! \brief A point or direction in world coordinates.
//!
//! Small on purpose: the simulation only adds, subtracts, scales, and measures.
//! A full linear algebra library is not needed. The z axis exists but is unused:
//! the world is flat.
//!
//! Entities use world coordinates. Layers and Zones use grid cells.
//! A cell is world position divided by GridConfig::cellSize. World converts both ways.
//!
//! Example:
//! \code
//! ogb::Vector3f const from(0.0f, 0.0f, 0.0f);
//! ogb::Vector3f const to(30.0f, 40.0f, 0.0f);
//! float const distance = ogb::length(to - from); // 50
//! \endcode
//==============================================================================
struct Vector3f
{
    //--------------------------------------------------------------------------
    //! \brief The origin.
    //--------------------------------------------------------------------------
    Vector3f() = default;

    //--------------------------------------------------------------------------
    //! \param[in] _x, _y, _z the three components, in world units.
    //--------------------------------------------------------------------------
    Vector3f(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}

    //--------------------------------------------------------------------------
    //! \brief Move this vector by \c direction.
    //! \return this vector after the move.
    //--------------------------------------------------------------------------
    Vector3f& operator+=(Vector3f const& direction)
    {
        x += direction.x;
        y += direction.y;
        z += direction.z;
        return *this;
    }

    //--------------------------------------------------------------------------
    //! \brief Move this vector by minus \c direction.
    //! \return this vector after the move.
    //--------------------------------------------------------------------------
    Vector3f& operator-=(Vector3f const& direction)
    {
        x -= direction.x;
        y -= direction.y;
        z -= direction.z;
        return *this;
    }

    //! \brief East-west axis, in world units.
    float x = 0.0f;
    //! \brief North-south axis, in world units.
    float y = 0.0f;
    //! \brief Elevation in world units. Unused: the world is flat.
    float z = 0.0f;
};

//==============================================================================
//! \brief One square on the world grid.
//!
//! The grid has no bounds. Coordinates are signed. Every world position maps to
//! a cell, even outside any city. A city owns the cells in its CellRegion.
//==============================================================================
struct Cell
{
    //! \brief Column, along the east-west axis.
    int32_t u = 0;
    //! \brief Row, along the north-south axis.
    int32_t v = 0;
};

//------------------------------------------------------------------------------
//! \return true if the two cells are the same square.
//------------------------------------------------------------------------------
inline bool operator==(Cell const& c1, Cell const& c2)
{
    return (c1.u == c2.u) && (c1.v == c2.v);
}

//------------------------------------------------------------------------------
//! \return true if the two cells are different squares.
//------------------------------------------------------------------------------
inline bool operator!=(Cell const& c1, Cell const& c2)
{
    return !(c1 == c2);
}

//------------------------------------------------------------------------------
//! \brief Write a cell as "[u, v]". Used in unit test output.
//------------------------------------------------------------------------------
inline std::ostream& operator<<(std::ostream& os, Cell const& cell)
{
    os << "[" << cell.u << ", " << cell.v << "]";
    return os;
}

//------------------------------------------------------------------------------
//! \brief Write a vector as "(x, y, z)". Used in unit test output.
//------------------------------------------------------------------------------
inline std::ostream& operator<<(std::ostream& os, Vector3f const& v)
{
    os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
    return os;
}

//------------------------------------------------------------------------------
//! \return the sum of the two vectors, component by component.
//------------------------------------------------------------------------------
inline Vector3f operator+(Vector3f const& v1, Vector3f const& v2)
{
    return Vector3f(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z);
}

//------------------------------------------------------------------------------
//! \return \c v1 minus \c v2.
//------------------------------------------------------------------------------
inline Vector3f operator-(Vector3f const& v1, Vector3f const& v2)
{
    return Vector3f(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z);
}

//------------------------------------------------------------------------------
//! \return \c v1 scaled by \c scalar.
//------------------------------------------------------------------------------
inline Vector3f operator*(Vector3f const& v1, float scalar)
{
    return Vector3f(v1.x * scalar, v1.y * scalar, v1.z * scalar);
}

//------------------------------------------------------------------------------
//! \return true if all three components are exactly equal.
//!
//! \note Uses exact equality on purpose. This checks "same position", for
//! example after a translation. Do not use this to compare positions from
//! different routes. Use length(v1 - v2) with a tolerance instead.
//------------------------------------------------------------------------------
#if defined(__GNUC__) || defined(__clang__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wfloat-equal"
#endif
inline bool operator==(Vector3f const& v1, Vector3f const& v2)
{
    return (v1.x == v2.x) && (v1.y == v2.y) && (v1.z == v2.z);
}
#if defined(__GNUC__) || defined(__clang__)
#    pragma GCC diagnostic pop
#endif

//------------------------------------------------------------------------------
//! \return true when the two vectors differ by at least one component.
//------------------------------------------------------------------------------
inline bool operator!=(Vector3f const& v1, Vector3f const& v2)
{
    return !(v1 == v2);
}

//------------------------------------------------------------------------------
//! \return the squared length of \c v.
//! Compare two squared lengths to compare distances without sqrt.
//------------------------------------------------------------------------------
inline float lengthSquared(Vector3f const& v)
{
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

//------------------------------------------------------------------------------
//! \return the length of \c v, in world units.
//------------------------------------------------------------------------------
inline float length(Vector3f const& v)
{
    return std::sqrt(lengthSquared(v));
}

} // namespace ogb

#endif
