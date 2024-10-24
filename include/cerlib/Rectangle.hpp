// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <cerlib/Option.hpp>

namespace cer
{
struct Circle;
struct Vector2;

/**
 * Represents a floating-point rectangle that defines its coordinates as a
 * top-left point and a size, typically in pixels.
 *
 * @ingroup Math
 */
struct Rectangle
{
    constexpr Rectangle();

    constexpr Rectangle(float x, float y, float width, float height);

    constexpr Rectangle(float x, float y, Vector2 size);

    constexpr Rectangle(Vector2 position, Vector2 size);

    constexpr Rectangle(Vector2 position, float width, float height);

    /** Gets the left border coordinate of the rectangle (equivalent to x). */
    auto left() const -> float;

    /** Gets the top border coordinate of the rectangle (equivalent to y). */
    auto top() const -> float;

    /** Gets the right border coordinate of the rectangle (equivalent to x + width). */
    auto right() const -> float;

    /** Gets the bottom border coordinate of the rectangle (equivalent to y + height). */
    auto bottom() const -> float;

    /** Gets the center point of the rectangle. */
    auto center() const -> Vector2;

    /** Gets the top-left corner of the rectangle. */
    auto top_left() const -> Vector2;

    /** Gets the top-center point of the rectangle. */
    auto top_center() const -> Vector2;

    /** Gets the top-right corner of the rectangle. */
    auto top_right() const -> Vector2;

    /** Gets the bottom-left corner of the rectangle. */
    auto bottom_left() const -> Vector2;

    /** Gets the bottom-center point of the rectangle. */
    auto bottom_center() const -> Vector2;

    /** Gets the bottom-right corner of the rectangle. */
    auto bottom_right() const -> Vector2;

    /** Scales all components of the rectangle by a specific factor. */
    auto scaled(const Vector2& scale) const -> Rectangle;

    /** Gets a value indicating whether the rectangle contains a specific point. */
    auto contains(const Vector2& vector) const -> bool;

    /** Gets a value indicating whether the rectangle fully contains a specific rectangle.
     */
    auto contains(const Rectangle& other) const -> bool;

    /**
     * Gets a version of the rectangle that is inflated by a specific amount.
     * @param amount The amount by which to inflate the rectangle.
     */
    auto inflated(float amount) const -> Rectangle;

    /**
     * Gets a version of the rectangle that is moved by a specific amount.
     *
     * @param offset The amount by which to move the rectangle.
     */
    auto offset(const Vector2& offset) const -> Rectangle;

    /**
     * Gets a value indicating whether the rectangle intersects with a specific
     * rectangle.
     *
     * @param other The rectangle to test for intersection.
     */
    auto intersects(const Rectangle& other) const -> bool;

    /**
     * Gets a value indicating whether the rectangle intersects with a specific
     * circle.
     *
     * @param circle The circle to test for intersection.
     */
    auto intersects(const Circle& circle) const -> bool;

    /**
     * Calculates the signed depth of intersection between two rectangles.
     *
     * @param lhs The first rectangle to test.
     * @param rhs The second rectangle to test.
     *
     * @return The amount of overlap between two intersecting rectangles. These depth
     * values can be negative depending on which sides the rectangles intersect. This
     * allows the caller to determine the correct direction to push objects in order to
     * resolve collisions. If the rectangles are not intersecting, an empty value is
     * returned.
     *
     * Example:
     * @code{.cpp}
     * const Option<Vector2> depth =
     *   Rectangle::intersection_depth({0, 0, 100, 100}, {50, 50, 100, 100});
     *
     * if (depth.has_value())
     * {
     *   cer::log_debug("We have an intersection: {}", depth.value());
     * }
     * @endcode
     */
    static auto intersection_depth(const Rectangle& lhs, const Rectangle& rhs) -> Option<Vector2>;

    /**
     * Calculates the intersection rectangle of two rectangles.
     *
     * @param lhs The first rectangle to test.
     * @param rhs The second rectangle to test.
     */
    static auto make_union(const Rectangle& lhs, const Rectangle& rhs) -> Rectangle;

    /**
     * Gets the top-left corner of the rectangle as a vector.
     */
    auto position() const -> Vector2;

    /**
     * Gets the size of the rectangle as a vector.
     */
    auto size() const -> Vector2;

    /** Default comparison */
    auto operator==(const Rectangle&) const -> bool = default;

    /** Default comparison */
    auto operator!=(const Rectangle&) const -> bool = default;

    /** The X coordinate of the rectangle's top-left corner */
    float x{};

    /** The Y coordinate of the rectangle's top-left corner */
    float y{};

    /** The width of the rectangle */
    float width{};

    /** The height of the rectangle */
    float height{};
};
} // namespace cer

#include <cerlib/Math.hpp>
#include <cerlib/Vector2.hpp>

namespace cer
{
constexpr Rectangle::Rectangle()
    : Rectangle(0, 0, 0, 0)
{
}

constexpr Rectangle::Rectangle(float x, float y, float width, float height)
    : x(x)
    , y(y)
    , width(width)
    , height(height)
{
}

constexpr Rectangle::Rectangle(float x, float y, Vector2 size)
    : x(x)
    , y(y)
    , width(size.x)
    , height(size.y)
{
}

constexpr Rectangle::Rectangle(Vector2 position, Vector2 size)
    : x(position.x)
    , y(position.y)
    , width(size.x)
    , height(size.y)
{
}

constexpr Rectangle::Rectangle(Vector2 position, float width, float height)
    : x(position.x)
    , y(position.y)
    , width(width)
    , height(height)
{
}
} // namespace cer
