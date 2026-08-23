//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file Vector.hpp
//! \brief Lightweight 3D vector type and the arithmetic the simulation needs.

#ifndef OPEN_GLASSBOX_VECTOR_HPP
#define OPEN_GLASSBOX_VECTOR_HPP

#include <cmath>
#include <ostream>

namespace ogb
{

//==============================================================================
//! \brief A point or a direction in world coordinates.
//!
//! Deliberately minimal: the simulation only ever adds, subtracts, scales and
//! measures, so pulling a linear algebra library in would buy nothing. The
//! third axis is carried around but nothing reads it yet, the world being flat.
//!
//! World coordinates are what the entities live in. The grid coordinates the
//! Maps and the Areas use are a division of them by SimulationConfig::
//! gridCellSize, and World does the conversion both ways.
//!
//! Example:
//! \code
//! ogb::Vector3f const from(0.0f, 0.0f, 0.0f);
//! ogb::Vector3f const to(30.0f, 40.0f, 0.0f);
//! float const length = ogb::magnitude(to - from); // 50
//! \endcode
//==============================================================================
struct Vector3f
{
    //--------------------------------------------------------------------------
    //! \brief Leaves the three components uninitialised, as a float does.
    //--------------------------------------------------------------------------
    Vector3f() = default;

    //--------------------------------------------------------------------------
    //! \brief \param[in] _x, _y, _z the three components, in world units.
    //--------------------------------------------------------------------------
    Vector3f(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}

    //--------------------------------------------------------------------------
    //! \brief Translate in place.
    //! \param[in] direction how far to move along each axis.
    //! \return this vector, moved.
    //--------------------------------------------------------------------------
    Vector3f& operator+=(const Vector3f& direction)
    {
        x += direction.x;
        y += direction.y;
        z += direction.z;
        return *this;
    }

    //! \brief East-west axis, in world units.
    float x;
    //! \brief North-south axis, in world units.
    float y;
    //! \brief Elevation, in world units. Carried but unused: the world is flat.
    float z;
};

//------------------------------------------------------------------------------
//! \brief Write a vector as "(x, y, z)", which is what a failing unit test
//! prints.
//------------------------------------------------------------------------------
static inline std::ostream& operator<<(std::ostream& os, Vector3f const& v)
{
    os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
    return os;
}

//------------------------------------------------------------------------------
//! \brief \return the sum of the two vectors, component by component.
//------------------------------------------------------------------------------
static inline Vector3f operator+(Vector3f const& v1, Vector3f const& v2)
{
    return Vector3f(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z);
}

//------------------------------------------------------------------------------
//! \brief \return the vector going from \c v2 to \c v1.
//------------------------------------------------------------------------------
static inline Vector3f operator-(Vector3f const& v1, Vector3f const& v2)
{
    return Vector3f(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z);
}

//------------------------------------------------------------------------------
//! \brief \return the vector stretched by \c scalar.
//------------------------------------------------------------------------------
static inline Vector3f operator*(Vector3f const& v1, float scalar)
{
    return Vector3f(v1.x * scalar, v1.y * scalar, v1.z * scalar);
}

//------------------------------------------------------------------------------
//! \brief \return the squared length of \c v. Comparing two of these is
//! comparing two lengths without paying for a square root.
//------------------------------------------------------------------------------
static inline float squaredMagnitude(Vector3f const& v)
{
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

//------------------------------------------------------------------------------
//! \brief \return the length of \c v, in world units.
//------------------------------------------------------------------------------
static inline float magnitude(Vector3f const& v)
{
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

} // namespace ogb

#endif
