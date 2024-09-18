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
    float left() const;

    /** Gets the top border coordinate of the rectangle (equivalent to y). */
    float top() const;

    /** Gets the right border coordinate of the rectangle (equivalent to x + width). */
    float right() const;

    /** Gets the bottom border coordinate of the rectangle (equivalent to y + height). */
    float bottom() const;

    /** Gets the center point of the rectangle. */
    Vector2 center() const;

    /** Gets the top-left corner of the rectangle. */
    Vector2 top_left() const;

    /** Gets the top-center point of the rectangle. */
    Vector2 top_center() const;

    /** Gets the top-right corner of the rectangle. */
    Vector2 top_right() const;

    /** Gets the bottom-left corner of the rectangle. */
    Vector2 bottom_left() const;

    /** Gets the bottom-center point of the rectangle. */
    Vector2 bottom_center() const;

    /** Gets the bottom-right corner of the rectangle. */
    Vector2 bottom_right() const;

    /** Scales all components of the rectangle by a specific factor. */
    Rectangle scaled(const Vector2& scale) const;

    /** Gets a value indicating whether the rectangle contains a specific point. */
    bool contains(const Vector2& vector) const;

    /** Gets a value indicating whether the rectangle fully contains a specific rectangle.
     */
    bool contains(const Rectangle& other) const;

    /**
     * Gets a version of the rectangle that is inflated by a specific amount.
     * @param amount The amount by which to inflate the rectangle.
     */
    Rectangle inflated(float amount) const;

    /**
     * Gets a version of the rectangle that is moved by a specific amount.
     *
     * @param offset The amount by which to move the rectangle.
     */
    Rectangle offset(const Vector2& offset) const;

    /**
     * Gets a value indicating whether the rectangle intersects with a specific
     * rectangle.
     *
     * @param other The rectangle to test for intersection.
     */
    bool intersects(const Rectangle& other) const;

    /**
     * Gets a value indicating whether the rectangle intersects with a specific
     * circle.
     *
     * @param circle The circle to test for intersection.
     */
    bool intersects(const Circle& circle) const;

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
    static std::optional<Vector2> intersection_depth(const Rectangle& lhs, const Rectangle& rhs);

    /**
     * Calculates the intersection rectangle of two rectangles.
     *
     * @param lhs The first rectangle to test.
     * @param rhs The second rectangle to test.
     */
    static Rectangle make_union(const Rectangle& lhs, const Rectangle& rhs);

    /**
     * Gets the top-left corner of the rectangle as a vector.
     */
    Vector2 position() const;

    /**
     * Gets the size of the rectangle as a vector.
     */
    Vector2 size() const;

    /** Default comparison */
    bool operator==(const Rectangle&) const = default;

    /** Default comparison */
    bool operator!=(const Rectangle&) const = default;

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

inline constexpr Rectangle::Rectangle(Vector2 position, float width, float height)
    : x(position.x)
    , y(position.y)
    , width(width)
    , height(height)
{
}

inline float Rectangle::left() const
{
    return x;
}

inline float Rectangle::top() const
{
    return y;
}

inline float Rectangle::right() const
{
    return x + width;
}

inline float Rectangle::bottom() const
{
    return y + height;
}

inline Vector2 Rectangle::center() const
{
    return {x + width / 2, y + height / 2};
}

inline Vector2 Rectangle::top_left() const
{
    return {x, y};
}

inline Vector2 Rectangle::top_center() const
{
    return {x + (width / 2), y};
}

inline Vector2 Rectangle::top_right() const
{
    return {x + width, y};
}

inline Vector2 Rectangle::bottom_left() const
{
    return {x, y + height};
}

inline Vector2 Rectangle::bottom_center() const
{
    return {x + (width / 2), y + height};
}

inline Vector2 Rectangle::bottom_right() const
{
    return {x + width, y + height};
}

inline Rectangle Rectangle::scaled(const Vector2& scale) const
{
    return {x * scale.x, y * scale.y, width * scale.x, height * scale.y};
}

inline bool Rectangle::contains(const Vector2& vector) const
{
    if (x <= vector.x && vector.x < x + width && y <= vector.y)
        return vector.y < y + height;

    return false;
}

inline bool Rectangle::contains(const Rectangle& other) const
{
    if (x <= other.x && other.x + other.width <= x + width && y <= other.y)
        return other.y + other.height <= y + height;

    return false;
}

inline Rectangle Rectangle::inflated(float amount) const
{
    return {
        x - amount,
        y - amount,
        width + amount * 2,
        height + amount * 2,
    };
}

inline Rectangle Rectangle::offset(const Vector2& offset) const
{
    return {
        x + offset.x,
        y + offset.y,
        width,
        height,
    };
}

inline bool Rectangle::intersects(const Rectangle& other) const
{
    return other.left() < right() && left() < other.right() && other.top() < bottom() &&
           top() < other.bottom();
}

inline bool Rectangle::intersects(const Circle& circle) const
{
    const Vector2 center = circle.center;
    const float   radius = circle.radius;

    const Vector2 v{
        clamp(center.x, left(), right()),
        clamp(center.y, top(), bottom()),
    };

    const Vector2 direction        = center - v;
    const float   distance_squared = length_squared(direction);

    return distance_squared > 0 && distance_squared < radius * radius;
}

inline std::optional<Vector2> Rectangle::intersection_depth(const Rectangle& lhs,
                                                            const Rectangle& rhs)
{
    // Calculate half sizes.
    const float half_width_a  = lhs.width / 2.0f;
    const float half_height_a = lhs.height / 2.0f;
    const float half_width_b  = rhs.width / 2.0f;
    const float half_height_b = rhs.height / 2.0f;

    // Calculate centers.
    const Vector2 center_a{
        lhs.left() + half_width_a,
        lhs.top() + half_height_a,
    };

    const Vector2 center_b{
        rhs.left() + half_width_b,
        rhs.top() + half_height_b,
    };

    // Calculate current and minimum-non-intersecting distances between centers.
    const float distance_x     = center_a.x - center_b.x;
    const float distance_y     = center_a.y - center_b.y;
    const float min_distance_x = half_width_a + half_width_b;
    const float min_distance_y = half_height_a + half_height_b;

    // If we are not intersecting at all, return (0, 0).
    if (abs(distance_x) >= min_distance_x || abs(distance_y) >= min_distance_y)
        return {};

    // Calculate and return intersection depths.
    return {{
        distance_x > 0 ? min_distance_x - distance_x : -min_distance_x - distance_x,
        distance_y > 0 ? min_distance_y - distance_y : -min_distance_y - distance_y,
    }};
}

inline Rectangle Rectangle::make_union(const Rectangle& lhs, const Rectangle& rhs)
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

inline Vector2 Rectangle::position() const
{
    return {x, y};
}

inline Vector2 Rectangle::size() const
{
    return {width, height};
}
} // namespace cer
