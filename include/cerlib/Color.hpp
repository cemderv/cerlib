// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <cerlib/Interval.hpp>
#include <compare>
#include <optional>

namespace cer
{
struct Vector2;
struct Vector3;
struct Vector4;

/**
 * Represents a floating-point RGBA color.
 *
 * @ingroup Math
 */
struct Color
{
    /** Default constructor */
    constexpr Color() = default;

    /**
     * Creates a color from separate RGBA values.
     *
     * @param r The value of the red channel
     * @param g The value of the green channel
     * @param b The value of the blue channel
     * @param a The value of the alpha channel
     */
    constexpr Color(float r, float g, float b, float a = 1.0f);

    /** Obtains the color value as a Vector3 representation. */
    auto to_vector3() const -> Vector3;

    /** Obtains the color value as a Vector4 representation. */
    auto to_vector4() const -> Vector4;

    /** Default comparison */
    auto operator==(const Color&) const -> bool = default;

    /** Default comparison */
    auto operator!=(const Color&) const -> bool = default;

    auto operator<=>(const Color&) const = default;

    /** The value of the color's red channel */
    float r{};

    /** The value of the color's green channel */
    float g{};

    /** The value of the color's blue channel */
    float b{};

    /** The value of the color's alpha channel */
    float a{};
};

/**
 * Represents a closed interval of color values.
 */
using ColorInterval = details::IntervalType<Color>;

/**
 * Calculates a random color.
 *
 * @param alpha If specified, the resulting color will have this alpha value.
 *              If empty, the alpha value is determined randomly.
 *
 * @ingroup Math
 */
auto random_color(std::optional<float> alpha = std::nullopt) -> Color;

/**
 * Calculates a random color.
 * The components are determined using the FastRand algorithm.
 *
 * @param alpha If specified, the resulting color will have this alpha value.
 *              If empty, the alpha value is determined randomly.
 *
 * @ingroup Math
 */
auto fastrand_color(std::optional<float> alpha = std::nullopt) -> Color;

/**
 * Calculates a random color with its components being in a specific interval.
 * The components are determined using the FastRand algorithm.
 *
 * @param interval The interval
 *
 * @ingroup Math
 */
auto fastrand_color(const ColorInterval& interval) -> Color;

auto operator+(const Color& lhs, const Color& rhs) -> Color;

auto operator-(const Color& lhs, const Color& rhs) -> Color;

auto operator*(const Color& lhs, float rhs) -> Color;

auto operator*(float lhs, const Color& rhs) -> Color;
} // namespace cer

namespace cer
{
constexpr Color::Color(float r, float g, float b, float a)
    : r(r)
    , g(g)
    , b(b)
    , a(a)
{
}
/**
 * A constant white color.
 *
 * @ingroup Math
 */
static constexpr Color white{1.0f, 1.0f, 1.0f, 1.0f};

/**
 * A constant black color.
 *
 * @ingroup Math
 */
static constexpr Color black{0.0f, 0.0f, 0.0f, 1.0f};

/**
 * A constant red color.
 *
 * @ingroup Math
 */
static constexpr Color red{1.0f, 0.0f, 0.0f, 1.0f};

/**
 * A constant green color.
 *
 * @ingroup Math
 */
static constexpr Color green{0.0f, 0.5f, 0.0f, 1.0f};

/**
 * A constant blue color.
 *
 * @ingroup Math
 */
static constexpr Color blue{0.0f, 0.0f, 1.0f, 1.0f};

/**
 * A constant cornflower blue color.
 *
 * @ingroup Math
 */
static constexpr Color cornflowerblue{100.0f / 255.0f, 149.0f / 255.0f, 237.0f / 255.0f, 1};

/**
 * A constant yellow color.
 *
 * @ingroup Math
 */
static constexpr Color yellow{1.0f, 1.0f, 0.0f, 1.0f};

/**
 * A fully transparent black.
 */
static constexpr Color transparent{0.0f, 0.0f, 0.0f, 0.0f};
} // namespace cer
