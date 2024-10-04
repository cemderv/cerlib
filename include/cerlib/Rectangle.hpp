// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <optional>

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
     * const std::optional<Vector2> depth =
     *   Rectangle::intersection_depth({0, 0, 100, 100}, {50, 50, 100, 100});
     *
     * if (depth.has_value())
     * {
     *   cer::log_debug("We have an intersection: {}", depth.value());
     * }
     * @endcode
     */
    static auto intersection_depth(const Rectangle& lhs, const Rectangle& rhs)
        -> std::optional<Vector2>;

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

#include <cerlib/Circle.hpp>
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

inline auto Rectangle::left() const -> float
{
    return x;
}

inline auto Rectangle::top() const -> float
{
    return y;
}

inline auto Rectangle::right() const -> float
{
    return x + width;
}

inline auto Rectangle::bottom() const -> float
{
    return y + height;
}

inline auto Rectangle::center() const -> Vector2
{
    return {x + (width / 2), y + (height / 2)};
}

inline auto Rectangle::top_left() const -> Vector2
{
    return {x, y};
}

inline auto Rectangle::top_center() const -> Vector2
{
    return {x + (width / 2), y};
}

inline auto Rectangle::top_right() const -> Vector2
{
    return {x + width, y};
}

inline auto Rectangle::bottom_left() const -> Vector2
{
    return {x, y + height};
}

inline auto Rectangle::bottom_center() const -> Vector2
{
    return {x + (width / 2), y + height};
}

inline auto Rectangle::bottom_right() const -> Vector2
{
    return {x + width, y + height};
}

inline auto Rectangle::scaled(const Vector2& scale) const -> Rectangle
{
    return {x * scale.x, y * scale.y, width * scale.x, height * scale.y};
}

inline auto Rectangle::contains(const Vector2& vector) const -> bool
{
    if (x <= vector.x && vector.x < x + width && y <= vector.y)
    {
        return vector.y < y + height;
    }

    return false;
}

inline auto Rectangle::contains(const Rectangle& other) const -> bool
{
    if (x <= other.x && other.x + other.width <= x + width && y <= other.y)
    {
        return other.y + other.height <= y + height;
    }

    return false;
}

inline auto Rectangle::inflated(float amount) const -> Rectangle
{
    return {
        x - amount,
        y - amount,
        width + (amount * 2),
        height + (amount * 2),
    };
}

inline auto Rectangle::offset(const Vector2& offset) const -> Rectangle
{
    return {
        x + offset.x,
        y + offset.y,
        width,
        height,
    };
}

inline auto Rectangle::intersects(const Rectangle& other) const -> bool
{
    return other.left() < right() && left() < other.right() && other.top() < bottom() &&
           top() < other.bottom();
}

inline auto Rectangle::intersects(const Circle& circle) const -> bool
{
    const auto center = circle.center;
    const auto radius = circle.radius;

    const auto v = Vector2{
        clamp(center.x, left(), right()),
        clamp(center.y, top(), bottom()),
    };

    const auto direction        = center - v;
    const auto distance_squared = length_squared(direction);

    return distance_squared > 0 && distance_squared < radius * radius;
}

inline auto Rectangle::intersection_depth(const Rectangle& lhs, const Rectangle& rhs)
    -> std::optional<Vector2>
{
    // Calculate half sizes.
    const auto half_width_a  = lhs.width / 2.0f;
    const auto half_height_a = lhs.height / 2.0f;
    const auto half_width_b  = rhs.width / 2.0f;
    const auto half_height_b = rhs.height / 2.0f;

    // Calculate centers.
    const auto center_a = Vector2{
        lhs.left() + half_width_a,
        lhs.top() + half_height_a,
    };

    const auto center_b = Vector2{
        rhs.left() + half_width_b,
        rhs.top() + half_height_b,
    };

    // Calculate current and minimum-non-intersecting distances between centers.
    const auto distance_x     = center_a.x - center_b.x;
    const auto distance_y     = center_a.y - center_b.y;
    const auto min_distance_x = half_width_a + half_width_b;
    const auto min_distance_y = half_height_a + half_height_b;

    // If we are not intersecting at all, return (0, 0).
    if (abs(distance_x) >= min_distance_x || abs(distance_y) >= min_distance_y)
    {
        return {};
    }

    // Calculate and return intersection depths.
    return {{
        distance_x > 0 ? min_distance_x - distance_x : -min_distance_x - distance_x,
        distance_y > 0 ? min_distance_y - distance_y : -min_distance_y - distance_y,
    }};
}

inline auto Rectangle::make_union(const Rectangle& lhs, const Rectangle& rhs) -> Rectangle
{
    const float x = min(lhs.x, rhs.x);
    const float y = min(lhs.y, rhs.y);

    return {
        x,
        y,
        max(lhs.right(), rhs.right()) - x,
        max(lhs.bottom(), rhs.bottom()) - y,
    };
}

inline auto Rectangle::position() const -> Vector2
{
    return {x, y};
}

inline auto Rectangle::size() const -> Vector2
{
    return {width, height};
}
} // namespace cer
