// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <cerlib/Vector2.hpp>

namespace cer
{
/**
 * Represents a circle, defined by a center and a radius.
 *
 * @ingroup Math
 */
struct Circle
{
    /** Default constructor */
    constexpr Circle() = default;

    /**
     * Creates a simple circle.
     *
     * @param center The center of the circle
     * @param radius The radius of the circle
     */
    constexpr Circle(Vector2 center, float radius);

    /** Default comparison */
    bool operator==(const Circle&) const = default;

    /** Default comparison */
    bool operator!=(const Circle&) const = default;

    /** The center of the circle. */
    Vector2 center;

    /** The radius of the circle. */
    float radius{};
};

constexpr Circle::Circle(Vector2 center, float radius)
    : center(center)
    , radius(radius)
{
}
} // namespace cer
