//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file Vector.hpp
//! \brief World position, grid cell, and the arithmetic the simulation needs.

#ifndef OPEN_GLASSBOX_VECTOR_HPP
#define OPEN_GLASSBOX_VECTOR_HPP

#include <cmath>
#include <cstdint>
#include <ostream>

namespace ogb
{

//==============================================================================
//! \brief A point or a direction in world coordinates.
//!
//! Deliberately small: the simulation only adds, subtracts, scales and
//! measures, so a linear algebra library would buy nothing. The third axis is
//! carried around but nothing reads it yet: the world is flat.
//!
//! Entities live in world coordinates. Layers and zones work in grid cells,
//! which are world coordinates divided by GridConfig::cellSize. World converts
//! both ways.
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
    //! \brief \param[in] _x, _y, _z the three components, in world units.
    //--------------------------------------------------------------------------
    Vector3f(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}

    //--------------------------------------------------------------------------
    //! \brief Move in place.
    //! \param[in] direction how far to move along each axis.
    //! \return this vector, moved.
    //--------------------------------------------------------------------------
    Vector3f& operator+=(Vector3f const& direction)
    {
        x += direction.x;
        y += direction.y;
        z += direction.z;
        return *this;
    }

    //--------------------------------------------------------------------------
    //! \brief Move in place, the other way.
    //! \param[in] direction how far to move along each axis.
    //! \return this vector, moved.
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
    //! \brief Elevation, in world units. Carried but unused: the world is flat.
    float z = 0.0f;
};

//==============================================================================
//! \brief One square of the world grid.
//!
//! The grid has no bounds and the coordinates are signed: every world position
//! falls in a cell, even outside every city. A city owns the rectangle of cells
//! given by its CellRegion.
//==============================================================================
struct Cell
{
    //! \brief Column, along the east-west axis.
    int32_t u = 0;
    //! \brief Row, along the north-south axis.
    int32_t v = 0;
};

//------------------------------------------------------------------------------
//! \brief \return true when the two cells are the same square.
//------------------------------------------------------------------------------
inline bool operator==(Cell const& c1, Cell const& c2)
{
    return (c1.u == c2.u) && (c1.v == c2.v);
}

//------------------------------------------------------------------------------
//! \brief \return true when the two cells are different squares.
//------------------------------------------------------------------------------
inline bool operator!=(Cell const& c1, Cell const& c2)
{
    return !(c1 == c2);
}

//------------------------------------------------------------------------------
//! \brief Write a cell as "[u, v]", which is what a failing unit test prints.
//------------------------------------------------------------------------------
inline std::ostream& operator<<(std::ostream& os, Cell const& cell)
{
    os << "[" << cell.u << ", " << cell.v << "]";
    return os;
}

//------------------------------------------------------------------------------
//! \brief Write a vector as "(x, y, z)", which is what a failing unit test
//! prints.
//------------------------------------------------------------------------------
inline std::ostream& operator<<(std::ostream& os, Vector3f const& v)
{
    os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
    return os;
}

//------------------------------------------------------------------------------
//! \brief \return the sum of the two vectors, component by component.
//------------------------------------------------------------------------------
inline Vector3f operator+(Vector3f const& v1, Vector3f const& v2)
{
    return Vector3f(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z);
}

//------------------------------------------------------------------------------
//! \brief \return the vector going from \c v2 to \c v1.
//------------------------------------------------------------------------------
inline Vector3f operator-(Vector3f const& v1, Vector3f const& v2)
{
    return Vector3f(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z);
}

//------------------------------------------------------------------------------
//! \brief \return the vector stretched by \c scalar.
//------------------------------------------------------------------------------
inline Vector3f operator*(Vector3f const& v1, float scalar)
{
    return Vector3f(v1.x * scalar, v1.y * scalar, v1.z * scalar);
}

//------------------------------------------------------------------------------
//! \brief \return true when the two vectors have exactly the same three
//! components.
//!
//! \note Exact equality on purpose: this answers "is it the very same
//! position", as when checking that a translation changed nothing. Two
//! positions worked out along different routes are never compared this way,
//! and a distance under some tolerance is length(v1 - v2) instead.
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
//! \brief \return true when the two vectors differ by at least one component.
//------------------------------------------------------------------------------
inline bool operator!=(Vector3f const& v1, Vector3f const& v2)
{
    return !(v1 == v2);
}

//------------------------------------------------------------------------------
//! \brief \return the squared length of \c v. Comparing two of these compares
//! two lengths without paying for a square root.
//------------------------------------------------------------------------------
inline float lengthSquared(Vector3f const& v)
{
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

//------------------------------------------------------------------------------
//! \brief \return the length of \c v, in world units.
//------------------------------------------------------------------------------
inline float length(Vector3f const& v)
{
    return std::sqrt(lengthSquared(v));
}

} // namespace ogb

#endif
