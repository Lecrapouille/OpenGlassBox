#ifndef VECTOR_HPP
#define VECTOR_HPP

#include <cmath>

struct Vector3f
{
    Vector3f() = default;
    Vector3f(float _x, float _y, float _z)
        : x(_x), y(_y), z(_z)
    {}

    float length()
    {
        return sqrtf(x * x + y * y + z * z);
    }

    float x, y, z;
};

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

#endif