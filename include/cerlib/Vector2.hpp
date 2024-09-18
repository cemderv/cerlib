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
 * Represents a floating-point (single-precision) 2D vector.
 *
 * @ingroup Math
 */
struct Vector2
{
    /**
     * Creates a 2D vector with all of its components being zero.
     */
    constexpr Vector2();

    /**
     * Creates a 2D vector by splatting a single value to all of its components.
     *
     * @param xy The value to set for the X and Y component
     */
    constexpr explicit Vector2(float xy);

    /**
     * Creates a 2D vector from two separate components.
     *
     * @param x The X component
     * @param y The Y component
     */
    constexpr Vector2(float x, float y);

    bool operator==(const Vector2&) const = default;
    bool operator!=(const Vector2&) const = default;

    /** The value of the X component */
    float x{};

    /** The value of the Y component */
    float y{};
};

/**
 * Calculates the length of a 2D vector.
 *
 * @ingroup Math
 */
float length(const Vector2& vector);

/**
 * Calculates the squared length of a 2D vector.
 *
 * @ingroup Math
 */
float length_squared(const Vector2& vector);

/**
 * Calculates the normalized version of a 2D vector.
 *
 * @ingroup Math
 */
Vector2 normalize(const Vector2& vector);

/**
 * Calculates the rounded version of a 2D vector.
 *
 * @ingroup Math
 */
Vector2 round(const Vector2& vector);

/**
 * Calculates a version of a 2D vector with all of its components being their
 * absolute values.
 *
 * @ingroup Math
 */
Vector2 abs(const Vector2& vector);

/**
 * Calculates the sine of a 2D vector's elements, specified in radians.
 *
 * @ingroup Math
 */
Vector2 sin(const Vector2& vector);

/**
 * Calculates the cosine of a 2D vector's elements, specified in radians.
 *
 * @ingroup Math
 */
Vector2 cos(const Vector2& vector);

/**
 * Calculates the tangent of a 2D vector's elements, specified in radians.
 *
 * @ingroup Math
 */
Vector2 tan(const Vector2& vector);

/**
 * Calculates the value of `base` raised to the power `exp` (2D vector).
 *
 * @ingroup Math
 */
Vector2 pow(const Vector2& x, const Vector2& y);

/**
 * Rounds a 2D vector's elements up to their nearest integers.
 *
 * @param value The vector to round up
 *
 * @ingroup Math
 */
Vector2 floor(const Vector2& value);

/**
 * Rounds a 2D vector's elements down to their nearest integers.
 *
 * @param value The vector to round down
 *
 * @ingroup Math
 */
Vector2 ceiling(const Vector2& value);

/**
 * Calculates a random 2D vector.
 *
 * @ingroup Math
 */
Vector2 random_vector2(float min = 0.0f, float max = 1.0f);

/**
 * Calculates the dot product of two 2D vectors.
 *
 * @ingroup Math
 */
float dot(const Vector2& lhs, const Vector2& rhs);

/**
 * Calculates the distance between two 2D vectors.
 *
 * @ingroup Math
 */
float distance(const Vector2& lhs, const Vector2& rhs);

/**
 * Calculates the squared distance between two 2D vectors.
 *
 * @ingroup Math
 */
float distance_squared(const Vector2& lhs, const Vector2& rhs);

/**
 * Performs a linear interpolation between two 2D vectors.
 *
 * @param start The start vector
 * @param end The destination vector
 * @param t The interpolation factor, in the range `[0.0 .. 1.0]`
 *
 * @ingroup Math
 */
Vector2 lerp(const Vector2& start, const Vector2& end, float t);

/**
 * Performs a smoothstep interpolation from one 2D vector to another.
 *
 * @param start The start vector
 * @param end The destination vector
 * @param t The interpolation factor, in the range `[0.0 .. 1.0]`
 *
 * @ingroup Math
 */
Vector2 smoothstep(const Vector2& start, const Vector2& end, float t);

/**
 * Clamps a 2D vector into a specific range.
 *
 * @param value The value to clamp.
 * @param min The minimum value of the range.
 * @param max The maximum value of the range.
 *
 * @ingroup Math
 */
Vector2 clamp(const Vector2& value, const Vector2& min, const Vector2& max);

/**
 * Gets a value indicating whether all components of a 2D vector are exactly
 * equal to zero.
 *
 * @param vector The vector to test
 *
 * @ingroup Math
 */
bool is_zero(const Vector2& vector);

/**
 * Gets a value indicating whether two 2D vectors are equal within a specific
 * threshold.
 *
 * @param lhs The first vector to compare
 * @param rhs The second vector to compare
 * @param threshold The threshold within which both vectors count as equal to one
 * another. The comparison is performed one a per-component basis.
 *
 * @ingroup Math
 */
bool are_equal_within(const Vector2& lhs,
                      const Vector2& rhs,
                      float          threshold = std::numeric_limits<float>::epsilon());

/**
 * Calculates the smaller of two 2D vectors.
 * The comparison is performed on a per-component basis.
 *
 * @param lhs The first vector to compare
 * @param rhs The second vector to compare
 *
 * @code{.cpp}
 * Vector2 smaller_vector = cer::min(Vector2{1, -2}, Vector2{-2, 3});
 * // smaller_vector is Vector2{-2, -2}.
 * @endcode
 *
 * @ingroup Math
 */
Vector2 min(const Vector2& lhs, const Vector2& rhs);

/**
 * Calculates the larger of two 2D vectors.
 * The comparison is performed on a per-component basis.
 *
 * @param lhs The first vector to compare
 * @param rhs The second vector to compare
 *
 * @code{.cpp}
 * Vector2 larger_vector = cer::max(Vector2{1, -2}, Vector2{-2, 3});
 * // larger_vector is Vector2{1, 3}.
 * @endcode
 *
 * @ingroup Math
 */
Vector2 max(const Vector2& lhs, const Vector2& rhs);

/**
 * Calculates the normal of a 2D line.
 *
 * @param start The start point of the line
 * @param end The end point of the line
 *
 * @ingroup Math
 */
Vector2 line_normal(const Vector2& start, const Vector2& end);

/**
 * Adds two 2D vectors.
 *
 * @ingroup Math
 */
Vector2 operator+(const Vector2& lhs, const Vector2& rhs);

/**
 * Subtracts two 2D vectors.
 *
 * @ingroup Math
 */
Vector2 operator-(const Vector2& lhs, const Vector2& rhs);

/**
 * Multiplies two 2D vectors.
 *
 * @ingroup Math
 */
Vector2 operator*(const Vector2& lhs, const Vector2& rhs);

/**
 * Multiplies a 2D vector by a number.
 *
 * @ingroup Math
 */
Vector2 operator*(const Vector2& lhs, float rhs);

/**
 * Multiplies a 2D vector by a number.
 *
 * @ingroup Math
 */
Vector2 operator*(float lhs, const Vector2& rhs);

/**
 * Divides a 2D vector by another 2D vector.
 *
 * @ingroup Math
 */
Vector2 operator/(const Vector2& lhs, const Vector2& rhs);

/**
 * Divides a 2D vector by a number.
 *
 * @ingroup Math
 */
Vector2 operator/(const Vector2& lhs, float rhs);

/**
 * Adds a 2D vector to another 2D vector.
 *
 * @ingroup Math
 */
Vector2& operator+=(Vector2& vector, const Vector2& rhs);

/**
 * Subtracts a 2D vector from another 2D vector.
 *
 * @ingroup Math
 */
Vector2& operator-=(Vector2& vector, const Vector2& rhs);

/**
 * Scales a 2D vector by another 2D vector.
 *
 * @ingroup Math
 */
Vector2& operator*=(Vector2& vector, const Vector2& rhs);

/**
 * Scales a 2D vector by a number.
 *
 * @ingroup Math
 */
Vector2& operator*=(Vector2& vector, float rhs);

/**
 * Divides a 2D vector by another 2D vector.
 *
 * @ingroup Math
 */
Vector2& operator/=(Vector2& vector, const Vector2& rhs);

/**
 * Divides a 2D vector by a number.
 *
 * @ingroup Math
 */
Vector2& operator/=(Vector2& vector, float rhs);

/**
 * Negates a 2D vector.
 *
 * @ingroup Math
 */
Vector2 operator-(const Vector2& value);
} // namespace cer

template <>
class std::numeric_limits<cer::Vector2>
{
  public:
    static constexpr auto min() noexcept
    {
        return cer::Vector2(std::numeric_limits<float>::min());
    }

    static constexpr auto lowest() noexcept
    {
        return cer::Vector2(std::numeric_limits<float>::lowest());
    }

    static constexpr auto max() noexcept
    {
        return cer::Vector2(std::numeric_limits<float>::max());
    }

    static constexpr auto epsilon() noexcept
    {
        return cer::Vector2(std::numeric_limits<float>::epsilon());
    }

    static constexpr auto round_error() noexcept
    {
        return cer::Vector2(std::numeric_limits<float>::round_error());
    }

    static constexpr auto infinity() noexcept
    {
        return cer::Vector2(std::numeric_limits<float>::infinity());
    }
};

#include <cerlib/Math.hpp>

namespace cer
{
constexpr Vector2::Vector2() = default;

constexpr Vector2::Vector2(float xy)
    : x(xy)
    , y(xy)
{
}

constexpr Vector2::Vector2(float x, float y)
    : x(x)
    , y(y)
{
}
} // namespace cer

inline float cer::length(const Vector2& vector)
{
    return std::sqrt(length_squared(vector));
}

inline float cer::length_squared(const Vector2& vector)
{
    return vector.x * vector.x + vector.y * vector.y;
}

inline cer::Vector2 cer::normalize(const Vector2& vector)
{
    const float len = length(vector);
    return is_zero(len) ? Vector2() : vector / len;
}

inline cer::Vector2 cer::round(const Vector2& vector)
{
    return {
        round(vector.x),
        round(vector.y),
    };
}

inline cer::Vector2 cer::abs(const Vector2& vector)
{
    return {
        abs(vector.x),
        abs(vector.y),
    };
}

inline cer::Vector2 cer::sin(const Vector2& vector)
{
    return {
        sin(vector.x),
        sin(vector.y),
    };
}

inline cer::Vector2 cer::cos(const Vector2& vector)
{
    return {
        cos(vector.x),
        cos(vector.y),
    };
}

inline cer::Vector2 cer::tan(const Vector2& vector)
{
    return {
        tan(vector.x),
        tan(vector.y),
    };
}

inline cer::Vector2 cer::pow(const Vector2& x, const Vector2& y)
{
    return {
        pow(x.x, y.x),
        pow(x.y, y.y),
    };
}

inline cer::Vector2 cer::floor(const Vector2& value)
{
    return {
        floor(value.x),
        floor(value.y),
    };
}

inline cer::Vector2 cer::ceiling(const Vector2& value)
{
    return {
        ceiling(value.x),
        ceiling(value.y),
    };
}

inline cer::Vector2 cer::random_vector2(float min, float max)
{
    return {
        random_float(min, max),
        random_float(min, max),
    };
}

inline float cer::dot(const Vector2& lhs, const Vector2& rhs)
{
    return lhs.x * rhs.x + lhs.y * rhs.y;
}

inline float cer::distance(const Vector2& lhs, const Vector2& rhs)
{
    return length(rhs - lhs);
}

inline float cer::distance_squared(const Vector2& lhs, const Vector2& rhs)
{
    return length_squared(rhs - lhs);
}

inline cer::Vector2 cer::lerp(const Vector2& start, const Vector2& end, float t)
{
    return {
        lerp(start.x, end.x, t),
        lerp(start.y, end.y, t),
    };
}

inline cer::Vector2 cer::smoothstep(const Vector2& start, const Vector2& end, float t)
{
    return {
        smoothstep(start.x, end.x, t),
        smoothstep(start.y, end.y, t),
    };
}

inline cer::Vector2 cer::clamp(const Vector2& value, const Vector2& min, const Vector2& max)
{
    return {
        clamp(value.x, min.x, max.x),
        clamp(value.y, min.y, max.y),
    };
}

inline bool cer::is_zero(const Vector2& vector)
{
    return is_zero(vector.x) && is_zero(vector.y);
}

inline bool cer::are_equal_within(const Vector2& lhs, const Vector2& rhs, float threshold)
{
    return equal_within(lhs.x, rhs.x, threshold) && equal_within(lhs.y, rhs.y, threshold);
}

inline cer::Vector2 cer::min(const Vector2& lhs, const Vector2& rhs)
{
    return Vector2(min(lhs.x, rhs.x), min(lhs.y, rhs.y));
}

inline cer::Vector2 cer::max(const Vector2& lhs, const Vector2& rhs)
{
    return Vector2(max(lhs.x, rhs.x), max(lhs.y, rhs.y));
}

inline cer::Vector2 cer::line_normal(const Vector2& start, const Vector2& end)
{
    const auto dx = end.x - start.x;
    const auto dy = end.y - start.y;
    return normalize(Vector2(-dy, dx));
}

inline cer::Vector2& cer::operator+=(Vector2& vector, const Vector2& rhs)
{
    vector.x += rhs.x;
    vector.y += rhs.y;
    return vector;
}

inline cer::Vector2& cer::operator-=(Vector2& vector, const Vector2& rhs)
{
    vector.x -= rhs.x;
    vector.y -= rhs.y;
    return vector;
}

inline cer::Vector2& cer::operator*=(Vector2& vector, const Vector2& rhs)
{
    vector.x *= rhs.x;
    vector.y *= rhs.y;
    return vector;
}

inline cer::Vector2& cer::operator*=(Vector2& vector, float rhs)
{
    vector.x *= rhs;
    vector.y *= rhs;
    return vector;
}

inline cer::Vector2& cer::operator/=(Vector2& vector, const Vector2& rhs)
{
    vector.x /= rhs.x;
    vector.y /= rhs.y;
    return vector;
}

inline cer::Vector2& cer::operator/=(Vector2& vector, float rhs)
{
    vector.x /= rhs;
    vector.y /= rhs;
    return vector;
}

inline cer::Vector2 cer::operator-(const Vector2& value)
{
    return {
        -value.x,
        -value.y,
    };
}

inline cer::Vector2 cer::operator+(const Vector2& lhs, const Vector2& rhs)
{
    return {
        lhs.x + rhs.x,
        lhs.y + rhs.y,
    };
}

inline cer::Vector2 cer::operator-(const Vector2& lhs, const Vector2& rhs)
{
    return {
        lhs.x - rhs.x,
        lhs.y - rhs.y,
    };
}

inline cer::Vector2 cer::operator*(const Vector2& lhs, const Vector2& rhs)
{
    return {
        lhs.x * rhs.x,
        lhs.y * rhs.y,
    };
}

inline cer::Vector2 cer::operator*(const Vector2& lhs, float rhs)
{
    return {
        lhs.x * rhs,
        lhs.y * rhs,
    };
}

inline cer::Vector2 cer::operator*(float lhs, const Vector2& rhs)
{
    return rhs * lhs;
}

inline cer::Vector2 cer::operator/(const Vector2& lhs, const Vector2& rhs)
{
    return {
        lhs.x / rhs.x,
        lhs.y / rhs.y,
    };
}

inline cer::Vector2 cer::operator/(const Vector2& lhs, float rhs)
{
    return {
        lhs.x / rhs,
        lhs.y / rhs,
    };
}
