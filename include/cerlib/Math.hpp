// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <cerlib/Interval.hpp>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <numbers>
#include <type_traits>

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
static constexpr float two_pi  = float(std::numbers::pi_v<double> * 2.0);
static constexpr float half_pi = float(std::numbers::pi_v<double> * 0.5);

/**
 * Calculates the sine of a value, specified in radians.
 *
 * @ingroup Math
 */
template <std::floating_point T>
auto sin(T value) -> T;

/**
 * Calculates the cosine of a value, specified in radians.
 *
 * @ingroup Math
 */
template <std::floating_point T>
auto cos(T value) -> T;

/**
 * Calculates the tangent of a value, specified in radians.
 *
 * @ingroup Math
 */
template <std::floating_point T>
auto tan(T value) -> T;

/**
 * Calculates the nearest value of a value,
 * rounding halfway cases away from zero.
 *
 * @ingroup Math
 */
template <std::floating_point T>
auto round(T value) -> T;

/**
 * Calculates the value of `base` raised to the power `exp`.
 *
 * @ingroup Math
 */
template <std::floating_point T>
auto pow(T base, T exp) -> T;

/**
 * Rounds a number down to its nearest integer.
 *
 * @param value The value to round down
 *
 * @ingroup Math
 */
template <std::floating_point T>
auto floor(T value) -> T;

/**
 * Rounds a number up to its nearest integer.
 *
 * @param value The value to round up
 *
 * @ingroup Math
 */
template <std::floating_point T>
auto ceiling(T value) -> T;

/**
 * Calculates a random integer in the interval `[min, max)`.
 * The number is determined using a Mersenne Twister.
 *
 * @param min The minimum allowed value
 * @param max The upper bound
 *
 * @ingroup Math
 */
auto random_int(int32_t min, int32_t max) -> int32_t;

/**
 * Calculates a random unsigned integer in the interval `[min, max)`.
 * The number is determined using a Mersenne Twister.
 *
 * @param min The minimum allowed value
 * @param max The upper bound
 *
 * @ingroup Math
 */
auto random_uint(uint32_t min, uint32_t max) -> uint32_t;

/**
 * Calculates a random single-precision floating-point value in the interval `[min, max)`.
 * The number is determined using a Mersenne Twister.
 *
 * @param min The minimum allowed value
 * @param max The upper bound
 *
 * @ingroup Math
 */
auto random_float(float min = 0.0f, float max = 1.0f) -> float;

/**
 * Calculates a random double-precision floating-point value in the interval `[min, max)`.
 * The number is determined using a Mersenne Twister.
 *
 * @param min The minimum allowed value
 * @param max The upper bound
 *
 * @ingroup Math
 */
auto random_double(double min = 0.0, double max = 1.0) -> double;

/**
 * Seeds the randomizer that is used in fastrand_* functions.
 *
 * @param value The new seed
 */
void seed_fastrand(int32_t value);

/**
 * Calculates a random integer.
 * The number is determined using the FastRand algorithm.
 *
 * @ingroup Math
 */
auto fastrand_int() -> int32_t;

/**
 * Calculates a random integer in the interval `[min, max]`.
 * The number is determined using the FastRand algorithm.
 *
 * @param min The minimum allowed value
 * @param max The maximum allowed value
 *
 * @ingroup Math
 */
auto fastrand_int(int32_t min, int32_t max) -> int32_t;

/**
 * Calculates a random integer in a specific interval.
 * The number is determined using the FastRand algorithm.
 *
 * @param interval The interval
 *
 * @ingroup Math
 */
auto fastrand_int(const IntInterval& interval) -> int32_t;

/**
 * Calculates a random unsigned integer.
 * The number is determined using the FastRand algorithm.
 *
 * @ingroup Math
 */
auto fastrand_uint() -> uint32_t;

/**
 * Calculates a random unsigned integer in the interval `[min, max]`.
 * The number is determined using the FastRand algorithm.
 *
 * @param min The minimum allowed value
 * @param max The maximum allowed value
 *
 * @ingroup Math
 */
auto fastrand_uint(uint32_t min, uint32_t max) -> uint32_t;

/**
 * Calculates a random unsigned integer in a specific interval.
 * The number is determined using the FastRand algorithm.
 *
 * @param interval The interval
 *
 * @ingroup Math
 */
auto fastrand_uint(const UIntInterval& interval) -> uint32_t;

/**
 * Calculates a random single-precision floating-point value in the interval `[min, max]`.
 * The number is determined using the FastRand algorithm.
 *
 * @ingroup Math
 */
auto fastrand_float_zero_to_one() -> float;

/**
 * Calculates a random single-precision floating-point value in the interval `[min, max]`.
 * The number is determined using the FastRand algorithm.
 *
 * @param min The minimum allowed value
 * @param max The maximum allowed value
 *
 * @ingroup Math
 */
auto fastrand_float(float min, float max) -> float;

/**
 * Calculates a random single-precision floating-point value in a specific interval.
 * The number is determined using the FastRand algorithm.
 *
 * @param interval The interval
 *
 * @ingroup Math
 */
auto fastrand_float(const FloatInterval& interval) -> float;

/**
 * Calculates a random angle value, in radians.
 * The number is determined using the FastRand algorithm.
 *
 * @ingroup Math
 */
auto fastrand_angle() -> float;

/**
 * Returns the smaller of two values.
 *
 * @ingroup Math
 */
template <Number T>
constexpr auto min(T lhs, T rhs) -> T;

/**
 * Returns the smallest of three values.
 *
 * @ingroup Math
 */
template <Number T>
constexpr auto min(T value1, T value2, T value3) -> T;

/**
 * Returns the larger of two values.
 *
 * @ingroup Math
 */
template <Number T>
constexpr auto max(T lhs, T rhs) -> T;

/**
 * Returns the largest of three values.
 *
 * @ingroup Math
 */
template <Number T>
constexpr auto max(T value1, T value2, T value3) -> T;

/**
 * Returns the absolute of a value.
 *
 * @ingroup Math
 */
template <Number T>
constexpr auto abs(T value) -> T;

/**
 * Converts degrees to radians.
 *
 * @ingroup Math
 */
template <std::floating_point T>
constexpr auto radians(T degrees) -> T;

/**
 * Converts radians to degrees.
 *
 * @ingroup Math
 */
template <std::floating_point T>
constexpr auto degrees(T radians) -> T;

/**
 * Calculates the unsigned distance between two values.
 *
 * @ingroup Math
 */
template <std::floating_point T>
constexpr auto distance(T lhs, T rhs) -> T;

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
constexpr auto clamp(T value, T min, T max) -> T;

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
constexpr auto lerp(T start, T end, T t) -> T;

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
constexpr auto inverse_lerp(T start, T end, T value) -> T;

template <std::floating_point T>
constexpr auto smoothstep(T start, T end, T t) -> T;

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
constexpr auto remap(T input_min, T input_max, T output_min, T output_max, T value) -> T;

/**
 * Gets a value indicating whether a number is exactly equal to zero.
 *
 * @ingroup Math
 */
template <std::floating_point T>
auto is_zero(T number) -> bool;

/**
 * Gets a value indicating a number is almost zero.
 *
 * @ingroup Math
 */
template <std::floating_point T>
auto is_within_epsilon(T number) -> bool;

/**
 * Gets a value indicating whether two numbers are almost equal (threshold
 * being epsilon).
 *
 * @ingroup Math
 */
template <std::floating_point T>
auto equal_within_epsilon(T lhs, T rhs) -> bool;

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
auto equal_within(T lhs, T rhs, T threshold) -> bool;

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
auto mipmap_extent(uint32_t base_extent, uint32_t mipmap) -> uint32_t;

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
auto max_mipmap_count_for_extent(uint32_t base_extent) -> uint32_t;

/**
 * Calculates a number that is aligned to a specific alignment.
 * @param number The number to align.
 * @param alignment The alignment.
 * @return The aligned number.
 *
 * @ingroup Math
 */
auto next_aligned_number(int64_t number, int64_t alignment) -> int64_t;
} // namespace cer

#include <cmath>

template <std::floating_point T>
auto cer::sin(T value) -> T
{
    return std::sin(value);
}

template <std::floating_point T>
auto cer::cos(T value) -> T
{
    return std::cos(value);
}

template <std::floating_point T>
auto cer::tan(T value) -> T
{
    return std::tan(value);
}

template <std::floating_point T>
auto cer::round(T value) -> T
{
    return std::round(value);
}

template <std::floating_point T>
auto cer::pow(T base, T exp) -> T
{
    return std::pow(base, exp);
}

template <std::floating_point T>
auto cer::floor(T value) -> T
{
    return std::floor(value);
}

template <std::floating_point T>
auto cer::ceiling(T value) -> T
{
    return std::ceil(value);
}

template <cer::Number T>
constexpr auto cer::min(T lhs, T rhs) -> T
{
    return lhs < rhs ? lhs : rhs;
}

template <cer::Number T>
constexpr auto cer::min(T value1, T value2, T value3) -> T
{
    return min(value1, min(value2, value3));
}

template <cer::Number T>
constexpr auto cer::max(T lhs, T rhs) -> T
{
    return rhs < lhs ? lhs : rhs;
}

template <cer::Number T>
constexpr auto cer::max(T value1, T value2, T value3) -> T
{
    return max(value1, max(value2, value3));
}

template <cer::Number T>
constexpr auto cer::abs(T value) -> T
{
    return std::abs(value);
}

template <std::floating_point T>
constexpr auto cer::radians(T degrees) -> T
{
    return degrees * T(pi / 180.0);
}

template <std::floating_point T>
constexpr auto cer::degrees(T radians) -> T
{
    return radians * T(180.0 / pi);
}

template <std::floating_point T>
constexpr auto cer::distance(T lhs, T rhs) -> T
{
    return abs(lhs - rhs);
}

template <cer::Number T>
constexpr auto cer::clamp(T value, T min, T max) -> T
{
    if (value < min)
    {
        value = min;
    }
    else if (value > max)
    {
        value = max;
    }

    return value;
}

template <std::floating_point T>
constexpr auto cer::lerp(T start, T end, T t) -> T
{
    return start + ((end - start) * t);
}

template <std::floating_point T>
constexpr auto cer::inverse_lerp(T start, T end, T value) -> T
{
    return (value - start) / (end - start);
}

template <std::floating_point T>
constexpr auto cer::smoothstep(T start, T end, T t) -> T
{
    t = t > 1 ? 1 : t < 0 ? 0 : t;
    t = t * t * (3.0f - 3.0f * t);
    return lerp(start, end, t);
}

template <std::floating_point T>
constexpr auto cer::remap(T input_min, T input_max, T output_min, T output_max, T value) -> T
{
    const auto t = inverse_lerp(input_min, input_max, value);
    return lerp(output_min, output_max, t);
}

template <std::floating_point T>
auto cer::is_zero(T number) -> bool
{
    return number == T(0);
}

template <std::floating_point T>
auto cer::is_within_epsilon(T number) -> bool
{
    return are_numbers_within_epsilon(number, 0.0f);
}

template <std::floating_point T>
auto cer::equal_within_epsilon(T lhs, T rhs) -> bool
{
    return equal_within(lhs, rhs, std::numeric_limits<T>::epsilon());
}

template <std::floating_point T>
auto cer::equal_within(T lhs, T rhs, T threshold) -> bool
{
    return abs(lhs - rhs) <= threshold;
}
