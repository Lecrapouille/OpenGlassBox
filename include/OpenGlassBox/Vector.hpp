//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#ifndef OPEN_GLASSBOX_VECTOR_HPP
#  define OPEN_GLASSBOX_VECTOR_HPP

#  include <cmath>
#  include <iostream>

//==============================================================================
//! \brief Quick and dirty implementation of 3D vector.
//==============================================================================
struct Vector3f
{
    Vector3f() = default;
    Vector3f(float _x, float _y, float _z)
        : x(_x), y(_y), z(_z)
    {}

    Vector3f& operator +=(const Vector3f& direction)
    {
        x += direction.x;
        y += direction.y;
        z += direction.z;
        return *this;
    }

    float x, y, z;
};

static inline std::ostream& operator<<(std::ostream& os, Vector3f const& v)
{
    os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
    return os;
}

static inline Vector3f operator+(Vector3f const& v1, Vector3f const& v2)
{
    return Vector3f(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z);
}

static inline Vector3f operator-(Vector3f const& v1, Vector3f const& v2)
{
    return Vector3f(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z);
}

static inline Vector3f operator*(Vector3f const& v1, float scalar)
{
    return Vector3f(v1.x * scalar, v1.y * scalar, v1.z * scalar);
}

static inline float squaredMagnitude(Vector3f const& v)
{
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

static inline float magnitude(Vector3f const& v)
{
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

#endif
