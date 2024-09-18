// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <limits>

namespace cer
{
struct Vector2;
struct Vector3;

/**
 * Represents a floating-point (single-precision) 4D vector.
 *
 * @ingroup Math
 */
struct Vector4
{
    /**
     * Creates a 4D vector with all of its components being zero.
     */
    constexpr Vector4();

    /**
     * Creates a 4D vector by splatting a single value to all of its components.
     *
     * @param xyzw The value to set for the X, Y, Z and W component
     */
    constexpr explicit Vector4(float xyzw);

    /**
     * Creates a 4D vector from two separate components.
     *
     * @param x The X component
     * @param y The Y component
     * @param z The Z component
     * @param w The W component
     */
    constexpr Vector4(float x, float y, float z, float w);

    /**
     * Creates a 4D vector from two 2D vectors.
     *
     * @param xy The 2D vector that represents the X and Y components
     * @param zw The 2D vector that represents the Z and W components
     */
    constexpr Vector4(Vector2 xy, Vector2 zw);

    /**
     * Creates a 4D vector from a 2D vector and two numbers.
     *
     * @param xy The 2D vector that represents the X and Y components
     * @param z The Z component
     * @param w The W component
     */
    constexpr Vector4(Vector2 xy, float z, float w);

    /**
     * Creates a 4D vector from a 3D vector and a number.
     *
     * @param xyz The 3D vector that represents the X, Y and Z components
     * @param w The W component
     */
    constexpr Vector4(Vector3 xyz, float w);

    /** Default comparison */
    bool operator==(const Vector4&) const = default;

    /** Default comparison */
    bool operator!=(const Vector4&) const = default;

    /** The value of the X component */
    float x = 0.0f;

    /** The value of the Y component */
    float y = 0.0f;

    /** The value of the Z component */
    float z = 0.0f;

    /** The value of the W component */
    float w = 0.0f;
};

/**
 * Calculates the length of a 4D vector.
 *
 * @ingroup Math
 */
float length(const Vector4& vector);

/**
 * Calculates the squared length of a 4D vector.
 *
 * @ingroup Math
 */
float length_squared(const Vector4& vector);

/**
 * Calculates the normalized version of a 4D vector.
 *
 * @ingroup Math
 */
Vector4 normalize(const Vector4& vector);

/**
 * Calculates the rounded version of a 4D vector.
 *
 * @ingroup Math
 */
Vector4 round(const Vector4& vector);

/**
 * Calculates a version of a 4D vector with all of its components being their
 * absolute values.
 *
 * @ingroup Math
 */
Vector4 abs(const Vector4& vector);

/**
 * Calculates the sine of a 4D vector's elements, specified in radians.
 *
 * @ingroup Math
 */
Vector4 sin(const Vector4& vector);

/**
 * Calculates the cosine of a 4D vector's elements, specified in radians.
 *
 * @ingroup Math
 */
Vector4 cos(const Vector4& vector);

/**
 * Calculates the tangent of a 4D vector's elements, specified in radians.
 *
 * @ingroup Math
 */
Vector4 tan(const Vector4& vector);

/**
 * Calculates the value of `base` raised to the power `exp` (4D vector).
 *
 * @ingroup Math
 */
Vector4 pow(const Vector4& x, const Vector4& y);

/**
 * Rounds a 4D vector's elements down to their nearest integers.
 *
 * @param value The vector to round down
 *
 * @ingroup Math
 */
Vector4 floor(const Vector4& value);

/**
 * Rounds a 4D vector's elements up to their nearest integers.
 *
 * @param value The vector to round up
 *
 * @ingroup Math
 */
Vector4 ceiling(const Vector4& value);

/**
 * Calculates a random 4D vector.
 *
 * @ingroup Math
 */
Vector4 random_vector4(float min = 0.0f, float max = 1.0f);

/**
 * Calculates the dot product of two 4D vectors.
 *
 * @ingroup Math
 */
float dot(const Vector4& lhs, const Vector4& rhs);

/**
 * Calculates the distance between two 4D vectors.
 *
 * @param lhs The start vector
 * @param rhs The destination vector
 *
 * @ingroup Math
 */
float distance(const Vector4& lhs, const Vector4& rhs);

/**
 * Calculates the squared distance between two 4D vectors.
 *
 * @param lhs The start vector
 * @param rhs The destination vector
 *
 * @ingroup Math
 */
float distance_squared(const Vector4& lhs, const Vector4& rhs);

/**
 * Performs a linear interpolation from one 4D vector to another.
 *
 * @param start The start vector
 * @param end The destination vector
 * @param t The interpolation factor, in the range `[0.0 .. 1.0]`
 *
 * @ingroup Math
 */
Vector4 lerp(const Vector4& start, const Vector4& end, float t);

/**
 * Performs a smoothstep interpolation from one 4D vector to another.
 *
 * @param start The start vector
 * @param end The destination vector
 * @param t The interpolation factor, in the range `[0.0 .. 1.0]`
 *
 * @ingroup Math
 */
Vector4 smoothstep(const Vector4& start, const Vector4& end, float t);

/**
 * Clamps a 4D vector into a specific range.
 *
 * @param value The value to clamp
 * @param min The minimum value of the range
 * @param max The maximum value of the range
 *
 * @ingroup Math
 */
Vector4 clamp(const Vector4& value, const Vector4& min, const Vector4& max);

/**
 * Gets a value indicating whether all components of a 4D vector are exactly
 * equal to zero.
 *
 * @param vector The vector to test
 *
 * @ingroup Math
 */
bool is_zero(const Vector4& vector);

/**
 * Gets a value indicating whether two 4D vectors are equal within a specific
 * threshold.
 *
 * @param lhs The first vector to compare
 * @param rhs The second vector to compare
 * @param threshold The threshold within which both vectors count as equal to one
 * another. The comparison is performed one a per-component basis.
 *
 * @ingroup Math
 */
static bool are_equal_within(const Vector4& lhs,
                             const Vector4& rhs,
                             float          threshold = std::numeric_limits<float>::epsilon());

/**
 * Calculates the smaller of two 4D vectors.
 * The comparison is performed on a per-component basis.
 *
 * @param lhs The first vector to compare
 * @param rhs The second vector to compare
 *
 * @code{.cpp}
 * Vector4 smaller_vector = cer::min(Vector4{1, -2, 3, -1}, Vector4{-2, 3, 4, 5});
 * // smaller_vector is Vector4{-2, -2, 3, -1}.
 * @endcode
 *
 * @ingroup Math
 */
Vector4 min(const Vector4& lhs, const Vector4& rhs);

/**
 * Calculates the larger of two 4D vectors.
 * The comparison is performed on a per-component basis.
 *
 * @param lhs The first vector to compare
 * @param rhs The second vector to compare
 *
 * @code{.cpp}
 * Vector4 larger_vector = cer::max(Vector4{1, -2, 3, -1}, Vector4{-2, 3, 4, 5});
 * // larger_vector is Vector4{1, 3, 4, 5}.
 * @endcode
 *
 * @ingroup Math
 */
Vector4 max(const Vector4& lhs, const Vector4& rhs);

/**
 * Adds two 4D vectors.
 *
 * @ingroup Math
 */
Vector4 operator+(const Vector4& lhs, const Vector4& rhs);

/**
 * Subtracts two 4D vectors.
 *
 * @ingroup Math
 */
Vector4 operator-(const Vector4& lhs, const Vector4& rhs);

/**
 * Multiplies two 4D vectors.
 *
 * @ingroup Math
 */
Vector4 operator*(const Vector4& lhs, const Vector4& rhs);

/**
 * Multiplies a 4D vector by a number.
 *
 * @ingroup Math
 */
Vector4 operator*(const Vector4& lhs, float rhs);

/**
 * Multiplies a 4D vector by a number.
 *
 * @ingroup Math
 */
Vector4 operator*(float lhs, const Vector4& rhs);

/**
 * Divides a 4D vector by another 4D vector.
 *
 * @ingroup Math
 */
Vector4 operator/(const Vector4& lhs, const Vector4& rhs);

/**
 * Divides a 4D vector by a number.
 *
 * @ingroup Math
 */
Vector4 operator/(const Vector4& lhs, float rhs);

/**
 * Adds a 4D vector to another 4D vector.
 *
 * @ingroup Math
 */
Vector4& operator+=(Vector4& vector, const Vector4& rhs);

/**
 * Subtracts a 4D vector from another 4D vector.
 *
 * @ingroup Math
 */
Vector4& operator-=(Vector4& vector, const Vector4& rhs);

/**
 * Scales a 4D vector by another 4D vector.
 *
 * @ingroup Math
 */
Vector4& operator*=(Vector4& vector, const Vector4& rhs);

/**
 * Scales a 4D vector by a number.
 *
 * @ingroup Math
 */
Vector4& operator*=(Vector4& vector, float rhs);

/**
 * Divides a 4D vector by another 4D vector.
 *
 * @ingroup Math
 */
Vector4& operator/=(Vector4& vector, const Vector4& rhs);

/**
 * Divides a 4D vector by a number.
 *
 * @ingroup Math
 */
Vector4& operator/=(Vector4& vector, float rhs);

/**
 * Negates a 4D vector.
 *
 * @ingroup Math
 */
Vector4 operator-(const Vector4& value);
} // namespace cer

template <>
class std::numeric_limits<cer::Vector4>
{
  public:
    static constexpr cer::Vector4 min() noexcept
    {
        return cer::Vector4(std::numeric_limits<float>::min());
    }

    static constexpr cer::Vector4 lowest() noexcept
    {
        return cer::Vector4(std::numeric_limits<float>::lowest());
    }

    static constexpr cer::Vector4 max() noexcept
    {
        return cer::Vector4(std::numeric_limits<float>::max());
    }

    static constexpr cer::Vector4 epsilon() noexcept
    {
        return cer::Vector4(std::numeric_limits<float>::epsilon());
    }

    static constexpr cer::Vector4 round_error() noexcept
    {
        return cer::Vector4(std::numeric_limits<float>::round_error());
    }

    static constexpr cer::Vector4 infinity() noexcept
    {
        return cer::Vector4(std::numeric_limits<float>::infinity());
    }
};

#include <cerlib/Math.hpp>
#include <cerlib/Vector2.hpp>
#include <cerlib/Vector3.hpp>

namespace cer
{
constexpr Vector4::Vector4() = default;

constexpr Vector4::Vector4(float xyzw)
    : x(xyzw)
    , y(xyzw)
    , z(xyzw)
    , w(xyzw)
{
}

constexpr Vector4::Vector4(float x, float y, float z, float w)
    : x(x)
    , y(y)
    , z(z)
    , w(w)
{
}

constexpr Vector4::Vector4(Vector2 xy, Vector2 zw)
    : x(xy.x)
    , y(xy.y)
    , z(zw.x)
    , w(zw.y)
{
}

constexpr Vector4::Vector4(Vector2 xy, float z, float w)
    : x(xy.x)
    , y(xy.y)
    , z(z)
    , w(w)
{
}

constexpr Vector4::Vector4(Vector3 xyz, float w)
    : x(xyz.x)
    , y(xyz.y)
    , z(xyz.z)
    , w(w)
{
}
} // namespace cer

inline float cer::length(const Vector4& vector)
{
    return std::sqrt(length_squared(vector));
}

inline float cer::length_squared(const Vector4& vector)
{
    return vector.x * vector.x + vector.y * vector.y + vector.z * vector.z + vector.w * vector.w;
}

inline cer::Vector4 cer::normalize(const Vector4& vector)
{
    const float len = length(vector);
    return is_zero(len) ? Vector4() : vector / len;
}

inline cer::Vector4 cer::round(const Vector4& vector)
{
    return {
        round(vector.x),
        round(vector.y),
        round(vector.z),
        round(vector.w),
    };
}

inline cer::Vector4 cer::abs(const Vector4& vector)
{
    return {
        abs(vector.x),
        abs(vector.y),
        abs(vector.z),
        abs(vector.w),
    };
}

inline cer::Vector4 cer::sin(const Vector4& vector)
{
    return {
        sin(vector.x),
        sin(vector.y),
        sin(vector.z),
        sin(vector.w),
    };
}

inline cer::Vector4 cer::cos(const Vector4& vector)
{
    return {
        cos(vector.x),
        cos(vector.y),
        cos(vector.z),
        cos(vector.w),
    };
}

inline cer::Vector4 cer::tan(const Vector4& vector)
{
    return {
        tan(vector.x),
        tan(vector.y),
        tan(vector.z),
        tan(vector.w),
    };
}

inline cer::Vector4 cer::pow(const Vector4& x, const Vector4& y)
{
    return {
        pow(x.x, y.x),
        pow(x.y, y.y),
        pow(x.z, y.z),
        pow(x.w, y.w),
    };
}

inline cer::Vector4 cer::floor(const Vector4& value)
{
    return {
        floor(value.x),
        floor(value.y),
        floor(value.z),
        floor(value.w),
    };
}

inline cer::Vector4 cer::ceiling(const Vector4& value)
{
    return {
        ceiling(value.x),
        ceiling(value.y),
        ceiling(value.z),
        ceiling(value.w),
    };
}

inline cer::Vector4 cer::random_vector4(float min, float max)
{
    return {
        random_float(min, max),
        random_float(min, max),
        random_float(min, max),
        random_float(min, max),
    };
}

inline float cer::dot(const Vector4& lhs, const Vector4& rhs)
{
    return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z + lhs.w * rhs.w;
}

inline float cer::distance(const Vector4& lhs, const Vector4& rhs)
{
    return length(rhs - lhs);
}

inline float cer::distance_squared(const Vector4& lhs, const Vector4& rhs)
{
    return length_squared(rhs - lhs);
}

inline cer::Vector4 cer::lerp(const Vector4& start, const Vector4& end, float t)
{
    return {
        lerp(start.x, end.x, t),
        lerp(start.y, end.y, t),
        lerp(start.z, end.z, t),
        lerp(start.w, end.w, t),
    };
}

inline cer::Vector4 cer::smoothstep(const Vector4& start, const Vector4& end, float t)
{
    return {
        smoothstep(start.x, end.x, t),
        smoothstep(start.y, end.y, t),
        smoothstep(start.z, end.z, t),
        smoothstep(start.w, end.w, t),
    };
}

inline cer::Vector4 cer::clamp(const Vector4& value, const Vector4& min, const Vector4& max)
{
    return {
        clamp(value.x, min.x, max.x),
        clamp(value.y, min.y, max.y),
        clamp(value.z, min.z, max.z),
        clamp(value.w, min.w, max.w),
    };
}

inline bool cer::is_zero(const Vector4& vector)
{
    return is_zero(vector.x) && is_zero(vector.y) && is_zero(vector.z) && is_zero(vector.w);
}

inline bool cer::are_equal_within(const Vector4& lhs, const Vector4& rhs, float threshold)
{
    return equal_within(lhs.x, rhs.x, threshold) && equal_within(lhs.y, rhs.y, threshold) &&
           equal_within(lhs.z, rhs.z, threshold) && equal_within(lhs.w, rhs.w, threshold);
}

inline cer::Vector4 cer::min(const Vector4& lhs, const Vector4& rhs)
{
    return {
        min(lhs.x, rhs.x),
        min(lhs.y, rhs.y),
        min(lhs.z, rhs.z),
        min(lhs.w, rhs.w),
    };
}

inline cer::Vector4 cer::max(const Vector4& lhs, const Vector4& rhs)
{
    return {
        max(lhs.x, rhs.x),
        max(lhs.y, rhs.y),
        max(lhs.z, rhs.z),
        max(lhs.w, rhs.w),
    };
}

inline cer::Vector4& cer::operator+=(Vector4& vector, const Vector4& rhs)
{
    vector.x += rhs.x;
    vector.y += rhs.y;
    vector.z += rhs.z;
    vector.w += rhs.w;
    return vector;
}

inline cer::Vector4& cer::operator-=(Vector4& vector, const Vector4& rhs)
{
    vector.x -= rhs.x;
    vector.y -= rhs.y;
    vector.z -= rhs.z;
    vector.w -= rhs.w;
    return vector;
}

inline cer::Vector4& cer::operator*=(Vector4& vector, const Vector4& rhs)
{
    vector.x *= rhs.x;
    vector.y *= rhs.y;
    vector.z *= rhs.z;
    vector.w *= rhs.w;
    return vector;
}

inline cer::Vector4& cer::operator*=(Vector4& vector, float rhs)
{
    vector.x *= rhs;
    vector.y *= rhs;
    vector.z *= rhs;
    vector.w *= rhs;
    return vector;
}

inline cer::Vector4& cer::operator/=(Vector4& vector, const Vector4& rhs)
{
    vector.x /= rhs.x;
    vector.y /= rhs.y;
    vector.z /= rhs.z;
    vector.w /= rhs.w;
    return vector;
}

inline cer::Vector4& cer::operator/=(Vector4& vector, float rhs)
{
    vector.x /= rhs;
    vector.y /= rhs;
    vector.z /= rhs;
    vector.w /= rhs;
    return vector;
}

inline cer::Vector4 cer::operator-(const Vector4& value)
{
    return {
        -value.x,
        -value.y,
        -value.z,
        -value.w,
    };
}

inline cer::Vector4 cer::operator+(const Vector4& lhs, const Vector4& rhs)
{
    return {
        lhs.x + rhs.x,
        lhs.y + rhs.y,
        lhs.z + rhs.z,
        lhs.w + rhs.w,
    };
}

inline cer::Vector4 cer::operator-(const Vector4& lhs, const Vector4& rhs)
{
    return {
        lhs.x - rhs.x,
        lhs.y - rhs.y,
        lhs.z - rhs.z,
        lhs.w - rhs.w,
    };
}

inline cer::Vector4 cer::operator*(const Vector4& lhs, const Vector4& rhs)
{
    return {
        lhs.x * rhs.x,
        lhs.y * rhs.y,
        lhs.z * rhs.z,
        lhs.w * rhs.w,
    };
}

inline cer::Vector4 cer::operator*(const Vector4& lhs, float rhs)
{
    return {
        lhs.x * rhs,
        lhs.y * rhs,
        lhs.z * rhs,
        lhs.w * rhs,
    };
}

inline cer::Vector4 cer::operator*(float lhs, const Vector4& rhs)
{
    return rhs * lhs;
}

inline cer::Vector4 cer::operator/(const Vector4& lhs, const Vector4& rhs)
{
    return {
        lhs.x / rhs.x,
        lhs.y / rhs.y,
        lhs.z / rhs.z,
        lhs.w / rhs.w,
    };
}

inline cer::Vector4 cer::operator/(const Vector4& lhs, float rhs)
{
    return {
        lhs.x / rhs,
        lhs.y / rhs,
        lhs.z / rhs,
        lhs.w / rhs,
    };
}
