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

    /** Gets a constant white color. */
    static constexpr Color white();

    /** Gets a constant black color (with an alpha value of 1.0). */
    static constexpr Color black();

    /** Gets a constant red color. */
    static constexpr Color red();

    /** Gets a constant green color. */
    static constexpr Color green();

    /** Gets a constant blue color. */
    static constexpr Color blue();

    /** Gets a constant cornflower blue color. */
    static constexpr Color cornflowerblue();

    /** Gets a constant yellow color. */
    static constexpr Color yellow();

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

constexpr Color Color::white()
{
    return {1, 1, 1, 1};
}

constexpr Color Color::black()
{
    return {0, 0, 0, 1};
}

constexpr Color Color::red()
{
    return {1, 0, 0, 1};
}

constexpr Color Color::green()
{
    return {0, 0.5f, 0, 1};
}

constexpr Color Color::blue()
{
    return {0, 0, 1, 1};
}

constexpr Color Color::cornflowerblue()
{
    return {100.0f / 255.0f, 149.0f / 255.0f, 237.0f / 255.0f, 1};
}

constexpr Color Color::yellow()
{
    return {1, 1, 0, 1};
}
} // namespace cer

inline cer::Color cer::operator*(const Color& lhs, float rhs)
{
    return {lhs.r * rhs, lhs.g * rhs, lhs.b * rhs, lhs.a * rhs};
}

inline cer::Color cer::operator*(float lhs, const Color& rhs)
{
    return {lhs * rhs.r, lhs * rhs.g, lhs * rhs.b, lhs * rhs.a};
}
