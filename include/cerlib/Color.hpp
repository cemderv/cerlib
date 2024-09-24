// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

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
    Vector3 to_vector3() const;

    /** Obtains the color value as a Vector4 representation. */
    Vector4 to_vector4() const;

    /** Default comparison */
    bool operator==(const Color&) const = default;

    /** Default comparison */
    bool operator!=(const Color&) const = default;

    /** The value of the color's red channel */
    float r{};

    /** The value of the color's green channel */
    float g{};

    /** The value of the color's blue channel */
    float b{};

    /** The value of the color's alpha channel */
    float a{};
};

Color operator*(const Color& lhs, float rhs);
Color operator*(float lhs, const Color& rhs);
} // namespace cer

#include <cerlib/Vector3.hpp>
#include <cerlib/Vector4.hpp>

namespace cer
{
constexpr Color::Color(float r, float g, float b, float a)
    : r(r)
    , g(g)
    , b(b)
    , a(a)
{
}

inline Vector3 Color::to_vector3() const
{
    return {r, g, b};
}

inline Vector4 Color::to_vector4() const
{
    return {r, g, b, a};
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
} // namespace cer

inline cer::Color cer::operator*(const Color& lhs, float rhs)
{
    return {lhs.r * rhs, lhs.g * rhs, lhs.b * rhs, lhs.a * rhs};
}

inline cer::Color cer::operator*(float lhs, const Color& rhs)
{
    return {lhs * rhs.r, lhs * rhs.g, lhs * rhs.b, lhs * rhs.a};
}
