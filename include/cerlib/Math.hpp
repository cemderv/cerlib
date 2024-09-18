// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <cerlib/Export.hpp>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <numbers>

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

namespace cer
{
template <typename T>
concept Number = std::is_arithmetic_v<T>;

static constexpr float pi      = std::numbers::pi_v<float>;
static constexpr float two_pi  = static_cast<float>(std::numbers::pi_v<double> * 2.0);
static constexpr float half_pi = static_cast<float>(std::numbers::pi_v<double> * 0.5);

/**
 * Calculates the sine of a value, specified in radians.
 *
 * @ingroup Math
 */
template <std::floating_point T>
T sin(T value);

/**
 * Calculates the cosine of a value, specified in radians.
 *
 * @ingroup Math
 */
template <std::floating_point T>
T cos(T value);

/**
 * Calculates the tangent of a value, specified in radians.
 *
 * @ingroup Math
 */
template <std::floating_point T>
T tan(T value);

/**
 * Calculates the nearest value of a value,
 * rounding halfway cases away from zero.
 *
 * @ingroup Math
 */
template <std::floating_point T>
T round(T value);

/**
 * Calculates the value of `base` raised to the power `exp`.
 *
 * @ingroup Math
 */
template <std::floating_point T>
T pow(T base, T exp);

/**
 * Rounds a number down to its nearest integer.
 *
 * @param value The value to round down
 *
 * @ingroup Math
 */
template <std::floating_point T>
T floor(T value);

/**
 * Rounds a number up to its nearest integer.
 *
 * @param value The value to round up
 *
 * @ingroup Math
 */
template <std::floating_point T>
T ceiling(T value);

/**
 * Calculates a random integer in the interval `[min, max)`.
 *
 * @param min The minimum allowed value
 * @param max The upper bound
 *
 * @ingroup Math
 */
CERLIB_API int32_t random_int(int32_t min, int32_t max);

/**
 * Calculates a random unsigned integer in the interval `[min, max)`.
 *
 * @param min The minimum allowed value
 * @param max The upper bound
 *
 * @ingroup math
 */
CERLIB_API uint32_t random_uint(uint32_t min, uint32_t max);

/**
 * Calculates a random single-precision floating-point value in the interval `[min, max)`.
 *
 * @param min The minimum allowed value
 * @param max The upper bound
 *
 * @ingroup Math
 */
CERLIB_API float random_float(float min = 0.0f, float max = 1.0f);

/**
 * Calculates a random double-precision floating-point value in the interval `[min, max)`.
 *
 * @param min The minimum allowed value
 * @param max The upper bound
 *
 * @ingroup Math
 */
CERLIB_API double random_double(double min = 0.0, double max = 1.0);

/**
 * Returns the smaller of two values.
 *
 * @ingroup Math
 */
template <Number T>
constexpr T min(T lhs, T rhs);

/**
 * Returns the smallest of three values.
 *
 * @ingroup Math
 */
template <Number T>
constexpr T min(T value1, T value2, T value3);

/**
 * Returns the larger of two values.
 *
 * @ingroup Math
 */
template <Number T>
constexpr T max(T lhs, T rhs);

/**
 * Returns the largest of three values.
 *
 * @ingroup Math
 */
template <Number T>
constexpr T max(T value1, T value2, T value3);

/**
 * Returns the absolute of a value.
 *
 * @ingroup Math
 */
template <Number T>
constexpr T abs(T value);

/**
 * Converts degrees to radians.
 *
 * @ingroup Math
 */
template <std::floating_point T>
constexpr T radians(T degrees);

/**
 * Converts radians to degrees.
 *
 * @ingroup Math
 */
template <std::floating_point T>
constexpr T degrees(T radians);

/**
 * Calculates the unsigned distance between two values.
 *
 * @ingroup Math
 */
template <std::floating_point T>
constexpr T distance(T lhs, T rhs);

/**
 * Clamps a value to a range.
 * @tparam T The type of value to clamp.
 * @param value The value to clamp.
 * @param min The minimum allowed value.
 * @param max The maximum allowed value. Must be larger than or equal to min.
 * @return The clamped value.
 *
 * @ingroup Math
 */
template <Number T>
constexpr T clamp(T value, T min, T max);

/**
 * Linearly interpolates a value depending on a factor.
 *
 * @tparam T The type of value to interpolate
 * @param start The start value of the range
 * @param end The end value of the range
 * @param t The interpolation factor, in the range `[0.0 .. 1.0]`
 * @return The interpolated value
 *
 * Example:
 * @code{.cpp}
 * const auto value = cer::lerp(100.0f, 300.0f, 0.5f);
 * // value => 200.0f
 * @endcode
 *
 * @ingroup Math
 */
template <std::floating_point T>
constexpr T lerp(T start, T end, T t);

/**
 * Reverses a linear interpolation, producing an interpolation factor.
 * @tparam T The type of value to interpolate.
 * @param start The start value of the range.
 * @param end The end value of the range.
 * @param value The value between start and end.
 * @return The interpolation factor.
 *
 * Example:
 * @code{.cpp}
 * const auto factor = cer::inverse_lerp(100.0f, 300.0f, 200.0f);
 * // factor => 0.5f
 * @endcode
 *
 * @ingroup Math
 */
template <std::floating_point T>
constexpr T inverse_lerp(T start, T end, T value);

template <std::floating_point T>
constexpr T smoothstep(T start, T end, T t);

/**
 * Proportionally maps a value from one range to another.
 * @tparam T The type of value to remap.
 * @param input_min The start of the input range.
 * @param input_max The end of the input range.
 * @param output_min The start of the output range.
 * @param output_max The end of the output range.
 * @param value The value to remap, within the input range.
 * @return The remapped value.
 *
 * @ingroup Math
 */
template <std::floating_point T>
constexpr T remap(T input_min, T input_max, T output_min, T output_max, T value);

/**
 * Gets a value indicating whether a number is exactly equal to zero.
 *
 * @ingroup Math
 */
template <std::floating_point T>
bool is_zero(T number);

/**
 * Gets a value indicating a number is almost zero.
 *
 * @ingroup Math
 */
template <std::floating_point T>
bool is_within_epsilon(T number);

/**
 * Gets a value indicating whether two numbers are almost equal (threshold
 * being epsilon).
 *
 * @ingroup Math
 */
template <std::floating_point T>
bool equal_within_epsilon(T lhs, T rhs);

/**
 * Gets a value indicating whether two numbers are equal within a specific
 * threshold.
 *
 * @param lhs The first number.
 * @param rhs The second number.
 * @param threshold The threshold within which both numbers count as being equal.
 *
 * @ingroup Math
 */
template <std::floating_point T>
bool equal_within(T lhs, T rhs, T threshold);

/**
 * Gets the extent of a mipmap at a specific level.
 *
 * @param base_extent The base extent. Typically the extent of the largest mipmap (the
 * image size).
 * @param mipmap The mipmap level to calculate the extent of.
 * @return The mipmap's extent.
 *
 * @ingroup Math
 */
CERLIB_API uint32_t mipmap_extent(uint32_t base_extent, uint32_t mipmap);

/**
 * Gets the numbers of mipmaps that can be generated for a specific base
 * extent.
 *
 * @param base_extent The base extent. Typically the extent of the largest mipmap (the
 * image size).
 * @return The number of mipmaps that can be generated, including the base mipmap.
 *
 * @ingroup Math
 */
CERLIB_API uint32_t max_mipmap_count_for_extent(uint32_t base_extent);

/**
 * Calculates a number that is aligned to a specific alignment.
 * @param number The number to align.
 * @param alignment The alignment.
 * @return The aligned number.
 *
 * @ingroup Math
 */
CERLIB_API int64_t next_aligned_number(int64_t number, int64_t alignment);
} // namespace cer

#include <cmath>

template <std::floating_point T>
T cer::sin(T value)
{
    return std::sin(value);
}

template <std::floating_point T>
T cer::cos(T value)
{
    return std::cos(value);
}

template <std::floating_point T>
T cer::tan(T value)
{
    return std::tan(value);
}

template <std::floating_point T>
T cer::round(T value)
{
    return std::round(value);
}

template <std::floating_point T>
T cer::pow(T base, T exp)
{
    return std::pow(base, exp);
}

template <std::floating_point T>
T cer::floor(T value)
{
    return std::floor(value);
}

template <std::floating_point T>
T cer::ceiling(T value)
{
    return std::ceil(value);
}

template <cer::Number T>
constexpr T cer::min(T lhs, T rhs)
{
    return lhs < rhs ? lhs : rhs;
}

template <cer::Number T>
constexpr T cer::min(T value1, T value2, T value3)
{
    return min(value1, min(value2, value3));
}

template <cer::Number T>
constexpr T cer::max(T lhs, T rhs)
{
    return rhs < lhs ? lhs : rhs;
}

template <cer::Number T>
constexpr T cer::max(T value1, T value2, T value3)
{
    return max(value1, max(value2, value3));
}

template <cer::Number T>
constexpr T cer::abs(T value)
{
    return std::abs(value);
}

template <std::floating_point T>
constexpr T cer::radians(T degrees)
{
    return degrees * static_cast<T>(pi / 180.0);
}

template <std::floating_point T>
constexpr T cer::degrees(T radians)
{
    return radians * static_cast<T>(180.0 / pi);
}

template <std::floating_point T>
constexpr T cer::distance(T lhs, T rhs)
{
    return abs(lhs - rhs);
}

template <cer::Number T>
constexpr T cer::clamp(T value, T min, T max)
{
    if (value < min)
        value = min;
    else if (value > max)
        value = max;

    return value;
}

template <std::floating_point T>
constexpr T cer::lerp(T start, T end, T t)
{
    return start + (end - start) * t;
}

template <std::floating_point T>
constexpr T cer::inverse_lerp(T start, T end, T value)
{
    return (value - start) / (end - start);
}

template <std::floating_point T>
constexpr T cer::smoothstep(T start, T end, T t)
{
    t = t > 1 ? 1 : t < 0 ? 0 : t;
    t = t * t * (3.0f - 3.0f * t);
    return lerp(start, end, t);
}

template <std::floating_point T>
constexpr T cer::remap(T input_min, T input_max, T output_min, T output_max, T value)
{
    const auto t = inverse_lerp(input_min, input_max, value);
    return lerp(output_min, output_max, t);
}

template <std::floating_point T>
bool cer::is_zero(T number)
{
    return number == T(0);
}

template <std::floating_point T>
bool cer::is_within_epsilon(T number)
{
    return are_numbers_within_epsilon(number, 0.0f);
}

template <std::floating_point T>
bool cer::equal_within_epsilon(T lhs, T rhs)
{
    return equal_within(lhs, rhs, std::numeric_limits<T>::epsilon());
}

template <std::floating_point T>
bool cer::equal_within(T lhs, T rhs, T threshold)
{
    return abs(lhs - rhs) <= threshold;
}
