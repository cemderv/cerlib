// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "cerlib/Rectangle.hpp"
#include "cerlib/Circle.hpp"

namespace cer
{
auto Rectangle::left() const -> float
{
    return x;
}

auto Rectangle::top() const -> float
{
    return y;
}

auto Rectangle::right() const -> float
{
    return x + width;
}

auto Rectangle::bottom() const -> float
{
    return y + height;
}

auto Rectangle::center() const -> Vector2
{
    return {x + (width / 2), y + (height / 2)};
}

auto Rectangle::top_left() const -> Vector2
{
    return {x, y};
}

auto Rectangle::top_center() const -> Vector2
{
    return {x + (width / 2), y};
}

auto Rectangle::top_right() const -> Vector2
{
    return {x + width, y};
}

auto Rectangle::bottom_left() const -> Vector2
{
    return {x, y + height};
}

auto Rectangle::bottom_center() const -> Vector2
{
    return {x + (width / 2), y + height};
}

auto Rectangle::bottom_right() const -> Vector2
{
    return {x + width, y + height};
}

auto Rectangle::scaled(const Vector2& scale) const -> Rectangle
{
    return {x * scale.x, y * scale.y, width * scale.x, height * scale.y};
}

auto Rectangle::contains(const Vector2& vector) const -> bool
{
    if (x <= vector.x && vector.x < x + width && y <= vector.y)
    {
        return vector.y < y + height;
    }

    return false;
}

auto Rectangle::contains(const Rectangle& other) const -> bool
{
    if (x <= other.x && other.x + other.width <= x + width && y <= other.y)
    {
        return other.y + other.height <= y + height;
    }

    return false;
}

auto Rectangle::inflated(float amount) const -> Rectangle
{
    return {
        x - amount,
        y - amount,
        width + (amount * 2),
        height + (amount * 2),
    };
}

auto Rectangle::offset(const Vector2& offset) const -> Rectangle
{
    return {
        x + offset.x,
        y + offset.y,
        width,
        height,
    };
}

auto Rectangle::intersects(const Rectangle& other) const -> bool
{
    return other.left() < right() && left() < other.right() && other.top() < bottom() &&
           top() < other.bottom();
}

auto Rectangle::intersects(const Circle& circle) const -> bool
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

auto Rectangle::intersection_depth(const Rectangle& lhs, const Rectangle& rhs) -> Option<Vector2>
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

auto Rectangle::make_union(const Rectangle& lhs, const Rectangle& rhs) -> Rectangle
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

auto Rectangle::position() const -> Vector2
{
    return {x, y};
}

auto Rectangle::size() const -> Vector2
{
    return {width, height};
}
} // namespace cer