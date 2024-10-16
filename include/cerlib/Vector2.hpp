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

    auto operator==(const Vector2&) const -> bool = default;

    auto operator!=(const Vector2&) const -> bool = default;

    /** The value of the X component */
    float x = 0.0f;

    /** The value of the Y component */
    float y = 0.0f;
};

/**
 * Calculates the length of a 2D vector.
 *
 * @ingroup Math
 */
auto length(const Vector2& vector) -> float;

/**
 * Calculates the squared length of a 2D vector.
 *
 * @ingroup Math
 */
auto length_squared(const Vector2& vector) -> float;

/**
 * Calculates the normalized version of a 2D vector.
 *
 * @ingroup Math
 */
auto normalize(const Vector2& vector) -> Vector2;

/**
 * Calculates the rounded version of a 2D vector.
 *
 * @ingroup Math
 */
auto round(const Vector2& vector) -> Vector2;

/**
 * Calculates a version of a 2D vector with all of its components being their
 * absolute values.
 *
 * @ingroup Math
 */
auto abs(const Vector2& vector) -> Vector2;

/**
 * Calculates the sine of a 2D vector's elements, specified in radians.
 *
 * @ingroup Math
 */
auto sin(const Vector2& vector) -> Vector2;

/**
 * Calculates the cosine of a 2D vector's elements, specified in radians.
 *
 * @ingroup Math
 */
auto cos(const Vector2& vector) -> Vector2;

/**
 * Calculates the tangent of a 2D vector's elements, specified in radians.
 *
 * @ingroup Math
 */
auto tan(const Vector2& vector) -> Vector2;

/**
 * Calculates the value of `base` raised to the power `exp` (2D vector).
 *
 * @ingroup Math
 */
auto pow(const Vector2& x, const Vector2& y) -> Vector2;

/**
 * Rounds a 2D vector's elements up to their nearest integers.
 *
 * @param value The vector to round up
 *
 * @ingroup Math
 */
auto floor(const Vector2& value) -> Vector2;

/**
 * Rounds a 2D vector's elements down to their nearest integers.
 *
 * @param value The vector to round down
 *
 * @ingroup Math
 */
auto ceiling(const Vector2& value) -> Vector2;

/**
 * Calculates a random 2D vector.
 *
 * @ingroup Math
 */
auto random_vector2(float min = 0.0f, float max = 1.0f) -> Vector2;

/**
 * Calculates a random 2D angle vector.
 * The number is determined using the FastRand algorithm.
 *
 * @ingroup Math
 */
auto fastrand_angle_vector2() -> Vector2;

/**
 * Calculates the dot product of two 2D vectors.
 *
 * @ingroup Math
 */
auto dot(const Vector2& lhs, const Vector2& rhs) -> float;

/**
 * Calculates the distance between two 2D vectors.
 *
 * @ingroup Math
 */
auto distance(const Vector2& lhs, const Vector2& rhs) -> float;

/**
 * Calculates the squared distance between two 2D vectors.
 *
 * @ingroup Math
 */
auto distance_squared(const Vector2& lhs, const Vector2& rhs) -> float;

/**
 * Performs a linear interpolation between two 2D vectors.
 *
 * @param start The start vector
 * @param end The destination vector
 * @param t The interpolation factor, in the range `[0.0 .. 1.0]`
 *
 * @ingroup Math
 */
auto lerp(const Vector2& start, const Vector2& end, float t) -> Vector2;

/**
 * Performs a smoothstep interpolation from one 2D vector to another.
 *
 * @param start The start vector
 * @param end The destination vector
 * @param t The interpolation factor, in the range `[0.0 .. 1.0]`
 *
 * @ingroup Math
 */
auto smoothstep(const Vector2& start, const Vector2& end, float t) -> Vector2;

/**
 * Clamps a 2D vector into a specific range.
 *
 * @param value The value to clamp.
 * @param min The minimum value of the range.
 * @param max The maximum value of the range.
 *
 * @ingroup Math
 */
auto clamp(const Vector2& value, const Vector2& min, const Vector2& max) -> Vector2;

/**
 * Gets a value indicating whether all components of a 2D vector are exactly
 * equal to zero.
 *
 * @param vector The vector to test
 *
 * @ingroup Math
 */
auto is_zero(const Vector2& vector) -> bool;

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
auto are_equal_within(const Vector2& lhs,
                      const Vector2& rhs,
                      float          threshold = std::numeric_limits<float>::epsilon()) -> bool;

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
auto min(const Vector2& lhs, const Vector2& rhs) -> Vector2;

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
auto max(const Vector2& lhs, const Vector2& rhs) -> Vector2;

/**
 * Calculates the normal of a 2D line.
 *
 * @param start The start point of the line
 * @param end The end point of the line
 *
 * @ingroup Math
 */
auto line_normal(const Vector2& start, const Vector2& end) -> Vector2;

/**
 * Adds two 2D vectors.
 *
 * @ingroup Math
 */
auto operator+(const Vector2& lhs, const Vector2& rhs) -> Vector2;

/**
 * Subtracts two 2D vectors.
 *
 * @ingroup Math
 */
auto operator-(const Vector2& lhs, const Vector2& rhs) -> Vector2;

/**
 * Multiplies two 2D vectors.
 *
 * @ingroup Math
 */
auto operator*(const Vector2& lhs, const Vector2& rhs) -> Vector2;

/**
 * Multiplies a 2D vector by a number.
 *
 * @ingroup Math
 */
auto operator*(const Vector2& lhs, float rhs) -> Vector2;

/**
 * Multiplies a 2D vector by a number.
 *
 * @ingroup Math
 */
auto operator*(float lhs, const Vector2& rhs) -> Vector2;

/**
 * Divides a 2D vector by another 2D vector.
 *
 * @ingroup Math
 */
auto operator/(const Vector2& lhs, const Vector2& rhs) -> Vector2;

/**
 * Divides a 2D vector by a number.
 *
 * @ingroup Math
 */
auto operator/(const Vector2& lhs, float rhs) -> Vector2;

/**
 * Adds a 2D vector to another 2D vector.
 *
 * @ingroup Math
 */
auto operator+=(Vector2& vector, const Vector2& rhs) -> Vector2&;

/**
 * Subtracts a 2D vector from another 2D vector.
 *
 * @ingroup Math
 */
auto operator-=(Vector2& vector, const Vector2& rhs) -> Vector2&;

/**
 * Scales a 2D vector by another 2D vector.
 *
 * @ingroup Math
 */
auto operator*=(Vector2& vector, const Vector2& rhs) -> Vector2&;

/**
 * Scales a 2D vector by a number.
 *
 * @ingroup Math
 */
auto operator*=(Vector2& vector, float rhs) -> Vector2&;

/**
 * Divides a 2D vector by another 2D vector.
 *
 * @ingroup Math
 */
auto operator/=(Vector2& vector, const Vector2& rhs) -> Vector2&;

/**
 * Divides a 2D vector by a number.
 *
 * @ingroup Math
 */
auto operator/=(Vector2& vector, float rhs) -> Vector2&;

/**
 * Negates a 2D vector.
 *
 * @ingroup Math
 */
auto operator-(const Vector2& value) -> Vector2;
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
