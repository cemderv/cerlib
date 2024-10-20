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
    auto operator==(const Vector4&) const -> bool = default;

    /** Default comparison */
    auto operator!=(const Vector4&) const -> bool = default;

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
auto length(const Vector4& vector) -> float;

/**
 * Calculates the squared length of a 4D vector.
 *
 * @ingroup Math
 */
auto length_squared(const Vector4& vector) -> float;

/**
 * Calculates the normalized version of a 4D vector.
 *
 * @ingroup Math
 */
auto normalize(const Vector4& vector) -> Vector4;

/**
 * Calculates the rounded version of a 4D vector.
 *
 * @ingroup Math
 */
auto round(const Vector4& vector) -> Vector4;

/**
 * Calculates a version of a 4D vector with all of its components being their
 * absolute values.
 *
 * @ingroup Math
 */
auto abs(const Vector4& vector) -> Vector4;

/**
 * Calculates the sine of a 4D vector's elements, specified in radians.
 *
 * @ingroup Math
 */
auto sin(const Vector4& vector) -> Vector4;

/**
 * Calculates the cosine of a 4D vector's elements, specified in radians.
 *
 * @ingroup Math
 */
auto cos(const Vector4& vector) -> Vector4;

/**
 * Calculates the tangent of a 4D vector's elements, specified in radians.
 *
 * @ingroup Math
 */
auto tan(const Vector4& vector) -> Vector4;

/**
 * Calculates the value of `base` raised to the power `exp` (4D vector).
 *
 * @ingroup Math
 */
auto pow(const Vector4& x, const Vector4& y) -> Vector4;

/**
 * Rounds a 4D vector's elements down to their nearest integers.
 *
 * @param value The vector to round down
 *
 * @ingroup Math
 */
auto floor(const Vector4& value) -> Vector4;

/**
 * Rounds a 4D vector's elements up to their nearest integers.
 *
 * @param value The vector to round up
 *
 * @ingroup Math
 */
auto ceiling(const Vector4& value) -> Vector4;

/**
 * Calculates a random 4D vector.
 *
 * @ingroup Math
 */
auto random_vector4(float min = 0.0f, float max = 1.0f) -> Vector4;

/**
 * Calculates the dot product of two 4D vectors.
 *
 * @ingroup Math
 */
auto dot(const Vector4& lhs, const Vector4& rhs) -> float;

/**
 * Calculates the distance between two 4D vectors.
 *
 * @param lhs The start vector
 * @param rhs The destination vector
 *
 * @ingroup Math
 */
auto distance(const Vector4& lhs, const Vector4& rhs) -> float;

/**
 * Calculates the squared distance between two 4D vectors.
 *
 * @param lhs The start vector
 * @param rhs The destination vector
 *
 * @ingroup Math
 */
auto distance_squared(const Vector4& lhs, const Vector4& rhs) -> float;

/**
 * Performs a linear interpolation from one 4D vector to another.
 *
 * @param start The start vector
 * @param end The destination vector
 * @param t The interpolation factor, in the range `[0.0 .. 1.0]`
 *
 * @ingroup Math
 */
auto lerp(const Vector4& start, const Vector4& end, float t) -> Vector4;

/**
 * Performs a smoothstep interpolation from one 4D vector to another.
 *
 * @param start The start vector
 * @param end The destination vector
 * @param t The interpolation factor, in the range `[0.0 .. 1.0]`
 *
 * @ingroup Math
 */
auto smoothstep(const Vector4& start, const Vector4& end, float t) -> Vector4;

/**
 * Clamps a 4D vector into a specific range.
 *
 * @param value The value to clamp
 * @param min The minimum value of the range
 * @param max The maximum value of the range
 *
 * @ingroup Math
 */
auto clamp(const Vector4& value, const Vector4& min, const Vector4& max) -> Vector4;

/**
 * Gets a value indicating whether all components of a 4D vector are exactly
 * equal to zero.
 *
 * @param vector The vector to test
 *
 * @ingroup Math
 */
auto is_zero(const Vector4& vector) -> bool;

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
auto are_equal_within(const Vector4& lhs,
                      const Vector4& rhs,
                      float          threshold = std::numeric_limits<float>::epsilon()) -> bool;

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
auto min(const Vector4& lhs, const Vector4& rhs) -> Vector4;

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
auto max(const Vector4& lhs, const Vector4& rhs) -> Vector4;

/**
 * Adds two 4D vectors.
 *
 * @ingroup Math
 */
auto operator+(const Vector4& lhs, const Vector4& rhs) -> Vector4;

/**
 * Subtracts two 4D vectors.
 *
 * @ingroup Math
 */
auto operator-(const Vector4& lhs, const Vector4& rhs) -> Vector4;

/**
 * Multiplies two 4D vectors.
 *
 * @ingroup Math
 */
auto operator*(const Vector4& lhs, const Vector4& rhs) -> Vector4;

/**
 * Multiplies a 4D vector by a number.
 *
 * @ingroup Math
 */
auto operator*(const Vector4& lhs, float rhs) -> Vector4;

/**
 * Multiplies a 4D vector by a number.
 *
 * @ingroup Math
 */
auto operator*(float lhs, const Vector4& rhs) -> Vector4;

/**
 * Divides a 4D vector by another 4D vector.
 *
 * @ingroup Math
 */
auto operator/(const Vector4& lhs, const Vector4& rhs) -> Vector4;

/**
 * Divides a 4D vector by a number.
 *
 * @ingroup Math
 */
auto operator/(const Vector4& lhs, float rhs) -> Vector4;

/**
 * Adds a 4D vector to another 4D vector.
 *
 * @ingroup Math
 */
auto operator+=(Vector4& vector, const Vector4& rhs) -> Vector4&;

/**
 * Subtracts a 4D vector from another 4D vector.
 *
 * @ingroup Math
 */
auto operator-=(Vector4& vector, const Vector4& rhs) -> Vector4&;

/**
 * Scales a 4D vector by another 4D vector.
 *
 * @ingroup Math
 */
auto operator*=(Vector4& vector, const Vector4& rhs) -> Vector4&;

/**
 * Scales a 4D vector by a number.
 *
 * @ingroup Math
 */
auto operator*=(Vector4& vector, float rhs) -> Vector4&;

/**
 * Divides a 4D vector by another 4D vector.
 *
 * @ingroup Math
 */
auto operator/=(Vector4& vector, const Vector4& rhs) -> Vector4&;

/**
 * Divides a 4D vector by a number.
 *
 * @ingroup Math
 */
auto operator/=(Vector4& vector, float rhs) -> Vector4&;

/**
 * Negates a 4D vector.
 *
 * @ingroup Math
 */
auto operator-(const Vector4& value) -> Vector4;
} // namespace cer

template <>
class std::numeric_limits<cer::Vector4>
{
  public:
    static constexpr auto min() noexcept -> cer::Vector4
    {
        return cer::Vector4(std::numeric_limits<float>::min());
    }

    static constexpr auto lowest() noexcept -> cer::Vector4
    {
        return cer::Vector4(std::numeric_limits<float>::lowest());
    }

    static constexpr auto max() noexcept -> cer::Vector4
    {
        return cer::Vector4(std::numeric_limits<float>::max());
    }

    static constexpr auto epsilon() noexcept -> cer::Vector4
    {
        return cer::Vector4(std::numeric_limits<float>::epsilon());
    }

    static constexpr auto round_error() noexcept -> cer::Vector4
    {
        return cer::Vector4(std::numeric_limits<float>::round_error());
    }

    static constexpr auto infinity() noexcept -> cer::Vector4
    {
        return cer::Vector4(std::numeric_limits<float>::infinity());
    }
};

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
