// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <limits>

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

namespace cer
{
/**
 * Represents a floating-point (single-precision) 3D vector.
 *
 * @ingroup Math
 */
struct Vector3
{
    /**
     * Creates a 3D vector with all of its components being zero.
     */
    constexpr Vector3();

    /**
     * Creates a 3D vector by splatting a single value to all of its components.
     *
     * @param xyz The value to set for the X, Y and Z component
     */
    constexpr explicit Vector3(float xyz);

    /**
     * Creates a 3D vector from two separate components.
     *
     * @param x The X component
     * @param y The Y component
     * @param z The Z component
     */
    constexpr Vector3(float x, float y, float z);

    /** Default comparison */
    bool operator==(const Vector3&) const = default;

    /** Default comparison */
    bool operator!=(const Vector3&) const = default;

    /** The value of the X component */
    float x{};

    /** The value of the Y component */
    float y{};

    /** The value of the Z component */
    float z{};
};

/**
 * Calculates the length of a 3D vector.
 *
 * @ingroup Math
 */
float length(const Vector3& vector);

/**
 * Calculates the squared length of a 3D vector.
 *
 * @ingroup Math
 */
float length_squared(const Vector3& vector);

/**
 * Calculates the normalized version of a 3D vector.
 *
 * @ingroup Math
 */
Vector3 normalize(const Vector3& vector);

/**
 * Calculates the rounded version of a 3D vector.
 *
 * @ingroup Math
 */
Vector3 round(const Vector3& vector);

/**
 * Calculates a version of a 3D vector with all of its components being their
 * absolute values.
 *
 * @ingroup Math
 */
Vector3 abs(const Vector3& vector);

/**
 * Calculates the sine of a 3D vector's elements, specified in radians.
 *
 * @ingroup Math
 */
Vector3 sin(const Vector3& vector);

/**
 * Calculates the cosine of a 3D vector's elements, specified in radians.
 *
 * @ingroup Math
 */
Vector3 cos(const Vector3& vector);

/**
 * Calculates the tangent of a 3D vector's elements, specified in radians.
 *
 * @ingroup Math
 */
Vector3 tan(const Vector3& vector);

/**
 * Calculates the value of `base` raised to the power `exp` (3D vector).
 *
 * @ingroup Math
 */
Vector3 pow(const Vector3& base, const Vector3& exp);

/**
 * Rounds a 3D vector's elements down to their nearest integers.
 *
 * @param value The vector to round down
 *
 * @ingroup Math
 */
Vector3 floor(const Vector3& value);

/**
 * Rounds a 3D vector's elements up to their nearest integers.
 *
 * @param value The vector to round up
 *
 * @ingroup Math
 */
Vector3 ceiling(const Vector3& value);

/**
 * Calculates a random 3D vector.
 *
 * @ingroup Math
 */
Vector3 random_vector3(float min = 0.0f, float max = 1.0f);

/**
 * Calculates the dot product of two 3D vectors.
 *
 * @ingroup Math
 */
float dot(const Vector3& lhs, const Vector3& rhs);

/**
 * Calculates the distance between two 3D vectors.
 *
 * @param lhs The start vector
 * @param rhs The destination vector
 *
 * @ingroup Math
 */
float distance(const Vector3& lhs, const Vector3& rhs);

/**
 * Calculates the squared distance between two 3D vectors.
 *
 * @param lhs The start vector
 * @param rhs The destination vector
 *
 * @ingroup Math
 */
float distance_squared(const Vector3& lhs, const Vector3& rhs);

/**
 * Performs a linear interpolation from one 3D vector to another.
 *
 * @param end The destination vector
 * @param t The interpolation factor, in the range `[0.0 .. 1.0]`
 *
 * @ingroup Math
 */
Vector3 lerp(const Vector3& start, const Vector3& end, float t);

/**
 * Performs a smoothstep interpolation from one 3D vector to another.
 *
 * @param start The start vector
 * @param end The destination vector
 * @param t The interpolation factor, in the range `[0.0 .. 1.0]`
 *
 * @ingroup Math
 */
Vector3 smoothstep(const Vector3& start, const Vector3& end, float t);

/**
 * Clamps a 3D vector into a specific range.
 *
 * @param value The value to clamp
 * @param min The minimum value of the range
 * @param max The maximum value of the range
 *
 * @ingroup Math
 */
Vector3 clamp(const Vector3& value, const Vector3& min, const Vector3& max);

/**
 * Gets a value indicating whether all components of a 3D vector are exactly
 * equal to zero.
 *
 * @param vector The vector to test
 *
 * @ingroup Math
 */
bool is_zero(const Vector3& vector);

/**
 * Gets a value indicating whether two 3D vectors are equal within a specific
 * threshold.
 *
 * @param lhs The first vector to compare
 * @param rhs The second vector to compare
 * @param threshold The threshold within which both vectors count as equal to one
 * another. The comparison is performed one a per-component basis.
 *
 * @ingroup Math
 */
bool are_equal_within(const Vector3& lhs,
                      const Vector3& rhs,
                      float          threshold = std::numeric_limits<float>::epsilon());

/**
 * Calculates the smaller of two 3D vectors.
 * The comparison is performed on a per-component basis.
 *
 * @param lhs The first vector to compare
 * @param rhs The second vector to compare
 *
 * @code{.cpp}
 * Vector3 smaller_vector = cer::min(Vector3{1, -2, 3}, Vector3{-2, 3, 4});
 * // smaller_vector is Vector3{-2, -2, 3}.
 * @endcode
 *
 * @ingroup Math
 */
static Vector3 min(const Vector3& lhs, const Vector3& rhs);

/**
 * Calculates the larger of two 3D vectors.
 * The comparison is performed on a per-component basis.
 *
 * @param lhs The first vector to compare
 * @param rhs The second vector to compare
 *
 * @code{.cpp}
 * Vector3 larger_vector = cer::max(Vector3{1, -2, 3}, Vector3{-2, 3, 4});
 * // larger_vector is Vector3{1, 3, 4}.
 * @endcode
 *
 * @ingroup Math
 */
static Vector3 max(const Vector3& lhs, const Vector3& rhs);

/**
 * Adds two 3D vectors.
 *
 * @ingroup Math
 */
Vector3 operator+(const Vector3& lhs, const Vector3& rhs);

/**
 * Subtracts two 3D vectors.
 *
 * @ingroup Math
 */
Vector3 operator-(const Vector3& lhs, const Vector3& rhs);

/**
 * Multiplies two 3D vectors.
 *
 * @ingroup Math
 */
Vector3 operator*(const Vector3& lhs, const Vector3& rhs);

/**
 * Multiplies a 3D vector by a number.
 *
 * @ingroup Math
 */
Vector3 operator*(const Vector3& lhs, float rhs);

/**
 * Multiplies a 3D vector by a number.
 *
 * @ingroup Math
 */
Vector3 operator*(float lhs, const Vector3& rhs);

/**
 * Divides a 3D vector by another 3D vector.
 *
 * @ingroup Math
 */
Vector3 operator/(const Vector3& lhs, const Vector3& rhs);

/**
 * Divides a 3D vector by a number.
 *
 * @ingroup Math
 */
Vector3 operator/(const Vector3& lhs, float rhs);

/**
 * Adds a 3D vector to another 3D vector.
 *
 * @ingroup Math
 */
Vector3& operator+=(Vector3& vector, const Vector3& rhs);

/**
 * Subtracts a 3D vector from another 3D vector.
 *
 * @ingroup Math
 */
Vector3& operator-=(Vector3& vector, const Vector3& rhs);

/**
 * Scales a 3D vector by another 3D vector.
 *
 * @ingroup Math
 */
Vector3& operator*=(Vector3& vector, const Vector3& rhs);

/**
 * Scales a 3D vector by a number.
 *
 * @ingroup Math
 */
Vector3& operator*=(Vector3& vector, float rhs);

/**
 * Divides a 3D vector by another 3D vector.
 *
 * @ingroup Math
 */
Vector3& operator/=(Vector3& vector, const Vector3& rhs);

/**
 * Divides a 3D vector by a number.
 *
 * @ingroup Math
 */
Vector3& operator/=(Vector3& vector, float rhs);

/**
 * Negates a 3D vector.
 *
 * @ingroup Math
 */
Vector3 operator-(const Vector3& value);
} // namespace cer

template <>
class std::numeric_limits<cer::Vector3>
{
  public:
    static constexpr auto min() noexcept
    {
        return cer::Vector3{std::numeric_limits<float>::min()};
    }

    static constexpr auto lowest() noexcept
    {
        return cer::Vector3{std::numeric_limits<float>::lowest()};
    }

    static constexpr auto max() noexcept
    {
        return cer::Vector3{std::numeric_limits<float>::max()};
    }

    static constexpr auto epsilon() noexcept
    {
        return cer::Vector3{std::numeric_limits<float>::epsilon()};
    }

    static constexpr auto round_error() noexcept
    {
        return cer::Vector3{std::numeric_limits<float>::round_error()};
    }

    static constexpr auto infinity() noexcept
    {
        return cer::Vector3{std::numeric_limits<float>::infinity()};
    }
};

#include <cerlib/Math.hpp>

namespace cer
{
constexpr Vector3::Vector3() = default;

constexpr Vector3::Vector3(float xyz)
    : x(xyz)
    , y(xyz)
    , z(xyz)
{
}

constexpr Vector3::Vector3(float x, float y, float z)
    : x(x)
    , y(y)
    , z(z)
{
}
} // namespace cer

inline float cer::length(const Vector3& vector)
{
    return std::sqrt(length_squared(vector));
}

inline float cer::length_squared(const Vector3& vector)
{
    return vector.x * vector.x + vector.y * vector.y + vector.z * vector.z;
}

inline cer::Vector3 cer::normalize(const Vector3& vector)
{
    const auto len = length(vector);
    return is_zero(len) ? Vector3() : vector / len;
}

inline cer::Vector3 cer::round(const Vector3& vector)
{
    return {
        round(vector.x),
        round(vector.y),
        round(vector.z),
    };
}

inline cer::Vector3 cer::abs(const Vector3& vector)
{
    return {
        abs(vector.x),
        abs(vector.y),
        abs(vector.z),
    };
}

inline cer::Vector3 cer::sin(const Vector3& vector)
{
    return {
        sin(vector.x),
        sin(vector.y),
        sin(vector.z),
    };
}

inline cer::Vector3 cer::cos(const Vector3& vector)
{
    return {
        cos(vector.x),
        cos(vector.y),
        cos(vector.z),
    };
}

inline cer::Vector3 cer::tan(const Vector3& vector)
{
    return {
        tan(vector.x),
        tan(vector.y),
        tan(vector.z),
    };
}

inline cer::Vector3 cer::pow(const Vector3& base, const Vector3& exp)
{
    return {
        pow(base.x, exp.x),
        pow(base.y, exp.y),
        pow(base.z, exp.z),
    };
}

inline cer::Vector3 cer::floor(const Vector3& value)
{
    return {
        floor(value.x),
        floor(value.y),
        floor(value.z),
    };
}

inline cer::Vector3 cer::ceiling(const Vector3& value)
{
    return {
        ceiling(value.x),
        ceiling(value.y),
        ceiling(value.z),
    };
}

inline cer::Vector3 cer::random_vector3(float min, float max)
{
    return {
        random_float(min, max),
        random_float(min, max),
        random_float(min, max),
    };
}

inline float cer::dot(const Vector3& lhs, const Vector3& rhs)
{
    return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

inline float cer::distance(const Vector3& lhs, const Vector3& rhs)
{
    return length(rhs - lhs);
}

inline float cer::distance_squared(const Vector3& lhs, const Vector3& rhs)
{
    return length_squared(rhs - lhs);
}

inline cer::Vector3 cer::lerp(const Vector3& start, const Vector3& end, float t)
{
    return {
        lerp(start.x, end.x, t),
        lerp(start.y, end.y, t),
        lerp(start.z, end.z, t),
    };
}

inline cer::Vector3 cer::smoothstep(const Vector3& start, const Vector3& end, float t)
{
    return {
        smoothstep(start.x, end.x, t),
        smoothstep(start.y, end.y, t),
        smoothstep(start.z, end.z, t),
    };
}

inline cer::Vector3 cer::clamp(const Vector3& value, const Vector3& min, const Vector3& max)
{
    return {
        clamp(value.x, min.x, max.x),
        clamp(value.y, min.y, max.y),
        clamp(value.z, min.z, max.z),
    };
}

inline auto cer::is_zero(const Vector3& vector) -> bool
{
    return is_zero(vector.x) && is_zero(vector.y) && is_zero(vector.z);
}

inline bool cer::are_equal_within(const Vector3& lhs, const Vector3& rhs, float threshold)
{
    return equal_within(lhs.x, rhs.x, threshold) && equal_within(lhs.y, rhs.y, threshold) &&
           equal_within(lhs.z, rhs.z, threshold);
}

inline cer::Vector3 cer::min(const Vector3& lhs, const Vector3& rhs)
{
    return {
        min(lhs.x, rhs.x),
        min(lhs.y, rhs.y),
        min(lhs.z, rhs.z),
    };
}

inline cer::Vector3 cer::max(const Vector3& lhs, const Vector3& rhs)
{
    return {
        max(lhs.x, rhs.x),
        max(lhs.y, rhs.y),
        max(lhs.z, rhs.z),
    };
}

inline cer::Vector3& cer::operator+=(Vector3& vector, const Vector3& rhs)
{
    vector.x += rhs.x;
    vector.y += rhs.y;
    vector.z += rhs.z;
    return vector;
}

inline cer::Vector3& cer::operator-=(Vector3& vector, const Vector3& rhs)
{
    vector.x -= rhs.x;
    vector.y -= rhs.y;
    vector.z -= rhs.z;
    return vector;
}

inline cer::Vector3& cer::operator*=(Vector3& vector, const Vector3& rhs)
{
    vector.x *= rhs.x;
    vector.y *= rhs.y;
    vector.z *= rhs.z;
    return vector;
}

inline cer::Vector3& cer::operator*=(Vector3& vector, float rhs)
{
    vector.x *= rhs;
    vector.y *= rhs;
    vector.z *= rhs;
    return vector;
}

inline cer::Vector3& cer::operator/=(Vector3& vector, const Vector3& rhs)
{
    vector.x /= rhs.x;
    vector.y /= rhs.y;
    vector.z /= rhs.z;
    return vector;
}

inline cer::Vector3& cer::operator/=(Vector3& vector, float rhs)
{
    vector.x /= rhs;
    vector.y /= rhs;
    vector.z /= rhs;
    return vector;
}

inline cer::Vector3 cer::operator-(const Vector3& value)
{
    return {
        -value.x,
        -value.y,
        -value.z,
    };
}

inline cer::Vector3 cer::operator+(const Vector3& lhs, const Vector3& rhs)
{
    return {
        lhs.x + rhs.x,
        lhs.y + rhs.y,
        lhs.z + rhs.z,
    };
}

inline cer::Vector3 cer::operator-(const Vector3& lhs, const Vector3& rhs)
{
    return {
        lhs.x - rhs.x,
        lhs.y - rhs.y,
        lhs.z - rhs.z,
    };
}

inline cer::Vector3 cer::operator*(const Vector3& lhs, const Vector3& rhs)
{
    return {
        lhs.x * rhs.x,
        lhs.y * rhs.y,
        lhs.z * rhs.z,
    };
}

inline cer::Vector3 cer::operator*(const Vector3& lhs, float rhs)
{
    return {
        lhs.x * rhs,
        lhs.y * rhs,
        lhs.z * rhs,
    };
}

inline cer::Vector3 cer::operator*(float lhs, const Vector3& rhs)
{
    return rhs * lhs;
}

inline cer::Vector3 cer::operator/(const Vector3& lhs, const Vector3& rhs)
{
    return {
        lhs.x / rhs.x,
        lhs.y / rhs.y,
        lhs.z / rhs.z,
    };
}

inline cer::Vector3 cer::operator/(const Vector3& lhs, float rhs)
{
    return {
        lhs.x / rhs,
        lhs.y / rhs,
        lhs.z / rhs,
    };
}
