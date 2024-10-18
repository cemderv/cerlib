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
    auto operator==(const Vector3&) const -> bool = default;

    /** Default comparison */
    auto operator!=(const Vector3&) const -> bool = default;

    /** The value of the X component */
    float x = 0.0f;

    /** The value of the Y component */
    float y = 0.0f;

    /** The value of the Z component */
    float z = 0.0f;
};

/**
 * Calculates the length of a 3D vector.
 *
 * @ingroup Math
 */
auto length(const Vector3& vector) -> float;

/**
 * Calculates the squared length of a 3D vector.
 *
 * @ingroup Math
 */
auto length_squared(const Vector3& vector) -> float;

/**
 * Calculates the normalized version of a 3D vector.
 *
 * @ingroup Math
 */
auto normalize(const Vector3& vector) -> Vector3;

/**
 * Calculates the rounded version of a 3D vector.
 *
 * @ingroup Math
 */
auto round(const Vector3& vector) -> Vector3;

/**
 * Calculates a version of a 3D vector with all of its components being their
 * absolute values.
 *
 * @ingroup Math
 */
auto abs(const Vector3& vector) -> Vector3;

/**
 * Calculates the sine of a 3D vector's elements, specified in radians.
 *
 * @ingroup Math
 */
auto sin(const Vector3& vector) -> Vector3;

/**
 * Calculates the cosine of a 3D vector's elements, specified in radians.
 *
 * @ingroup Math
 */
auto cos(const Vector3& vector) -> Vector3;

/**
 * Calculates the tangent of a 3D vector's elements, specified in radians.
 *
 * @ingroup Math
 */
auto tan(const Vector3& vector) -> Vector3;

/**
 * Calculates the value of `base` raised to the power `exp` (3D vector).
 *
 * @ingroup Math
 */
auto pow(const Vector3& base, const Vector3& exp) -> Vector3;

/**
 * Rounds a 3D vector's elements down to their nearest integers.
 *
 * @param value The vector to round down
 *
 * @ingroup Math
 */
auto floor(const Vector3& value) -> Vector3;

/**
 * Rounds a 3D vector's elements up to their nearest integers.
 *
 * @param value The vector to round up
 *
 * @ingroup Math
 */
auto ceiling(const Vector3& value) -> Vector3;

/**
 * Calculates a random 3D vector.
 *
 * @ingroup Math
 */
auto random_vector3(float min = 0.0f, float max = 1.0f) -> Vector3;

/**
 * Calculates the dot product of two 3D vectors.
 *
 * @ingroup Math
 */
auto dot(const Vector3& lhs, const Vector3& rhs) -> float;

/**
 * Calculates the cross product of two 3D vectors.
 *
 * @ingroup Math
 */
auto cross(const Vector3& lhs, const Vector3& rhs) -> Vector3;

/**
 * Calculates the distance between two 3D vectors.
 *
 * @param lhs The start vector
 * @param rhs The destination vector
 *
 * @ingroup Math
 */
auto distance(const Vector3& lhs, const Vector3& rhs) -> float;

/**
 * Calculates the squared distance between two 3D vectors.
 *
 * @param lhs The start vector
 * @param rhs The destination vector
 *
 * @ingroup Math
 */
auto distance_squared(const Vector3& lhs, const Vector3& rhs) -> float;

/**
 * Performs a linear interpolation from one 3D vector to another.
 *
 * @param end The destination vector
 * @param t The interpolation factor, in the range `[0.0 .. 1.0]`
 *
 * @ingroup Math
 */
auto lerp(const Vector3& start, const Vector3& end, float t) -> Vector3;

/**
 * Performs a smoothstep interpolation from one 3D vector to another.
 *
 * @param start The start vector
 * @param end The destination vector
 * @param t The interpolation factor, in the range `[0.0 .. 1.0]`
 *
 * @ingroup Math
 */
auto smoothstep(const Vector3& start, const Vector3& end, float t) -> Vector3;

/**
 * Clamps a 3D vector into a specific range.
 *
 * @param value The value to clamp
 * @param min The minimum value of the range
 * @param max The maximum value of the range
 *
 * @ingroup Math
 */
auto clamp(const Vector3& value, const Vector3& min, const Vector3& max) -> Vector3;

/**
 * Gets a value indicating whether all components of a 3D vector are exactly
 * equal to zero.
 *
 * @param vector The vector to test
 *
 * @ingroup Math
 */
auto is_zero(const Vector3& vector) -> bool;

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
auto are_equal_within(const Vector3& lhs,
                      const Vector3& rhs,
                      float          threshold = std::numeric_limits<float>::epsilon()) -> bool;

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
auto min(const Vector3& lhs, const Vector3& rhs) -> Vector3;

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
auto max(const Vector3& lhs, const Vector3& rhs) -> Vector3;

/**
 * Adds two 3D vectors.
 *
 * @ingroup Math
 */
auto operator+(const Vector3& lhs, const Vector3& rhs) -> Vector3;

/**
 * Subtracts two 3D vectors.
 *
 * @ingroup Math
 */
auto operator-(const Vector3& lhs, const Vector3& rhs) -> Vector3;

/**
 * Multiplies two 3D vectors.
 *
 * @ingroup Math
 */
auto operator*(const Vector3& lhs, const Vector3& rhs) -> Vector3;

/**
 * Multiplies a 3D vector by a number.
 *
 * @ingroup Math
 */
auto operator*(const Vector3& lhs, float rhs) -> Vector3;

/**
 * Multiplies a 3D vector by a number.
 *
 * @ingroup Math
 */
auto operator*(float lhs, const Vector3& rhs) -> Vector3;

/**
 * Divides a 3D vector by another 3D vector.
 *
 * @ingroup Math
 */
auto operator/(const Vector3& lhs, const Vector3& rhs) -> Vector3;

/**
 * Divides a 3D vector by a number.
 *
 * @ingroup Math
 */
auto operator/(const Vector3& lhs, float rhs) -> Vector3;

/**
 * Adds a 3D vector to another 3D vector.
 *
 * @ingroup Math
 */
auto operator+=(Vector3& vector, const Vector3& rhs) -> Vector3&;

/**
 * Subtracts a 3D vector from another 3D vector.
 *
 * @ingroup Math
 */
auto operator-=(Vector3& vector, const Vector3& rhs) -> Vector3&;

/**
 * Scales a 3D vector by another 3D vector.
 *
 * @ingroup Math
 */
auto operator*=(Vector3& vector, const Vector3& rhs) -> Vector3&;

/**
 * Scales a 3D vector by a number.
 *
 * @ingroup Math
 */
auto operator*=(Vector3& vector, float rhs) -> Vector3&;

/**
 * Divides a 3D vector by another 3D vector.
 *
 * @ingroup Math
 */
auto operator/=(Vector3& vector, const Vector3& rhs) -> Vector3&;

/**
 * Divides a 3D vector by a number.
 *
 * @ingroup Math
 */
auto operator/=(Vector3& vector, float rhs) -> Vector3&;

/**
 * Negates a 3D vector.
 *
 * @ingroup Math
 */
auto operator-(const Vector3& value) -> Vector3;
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
