// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "cerlib/Vector2.hpp"
#include "cerlib/Math.hpp"

auto cer::length(const Vector2& vector) -> float
{
    return std::sqrt(length_squared(vector));
}

auto cer::length_squared(const Vector2& vector) -> float
{
    return (vector.x * vector.x) + (vector.y * vector.y);
}

auto cer::normalize(const Vector2& vector) -> Vector2
{
    const float len = length(vector);
    return is_zero(len) ? Vector2() : vector / len;
}

auto cer::round(const Vector2& vector) -> Vector2
{
    return {
        round(vector.x),
        round(vector.y),
    };
}

auto cer::abs(const Vector2& vector) -> Vector2
{
    return {
        abs(vector.x),
        abs(vector.y),
    };
}

auto cer::sin(const Vector2& vector) -> Vector2
{
    return {
        sin(vector.x),
        sin(vector.y),
    };
}

auto cer::cos(const Vector2& vector) -> Vector2
{
    return {
        cos(vector.x),
        cos(vector.y),
    };
}

auto cer::tan(const Vector2& vector) -> Vector2
{
    return {
        tan(vector.x),
        tan(vector.y),
    };
}

auto cer::pow(const Vector2& x, const Vector2& y) -> Vector2
{
    return {
        pow(x.x, y.x),
        pow(x.y, y.y),
    };
}

auto cer::floor(const Vector2& value) -> Vector2
{
    return {
        floor(value.x),
        floor(value.y),
    };
}

auto cer::ceiling(const Vector2& value) -> Vector2
{
    return {
        ceiling(value.x),
        ceiling(value.y),
    };
}

auto cer::random_vector2(float min, float max) -> Vector2
{
    return {
        random_float(min, max),
        random_float(min, max),
    };
}

auto cer::fastrand_angle_vector2() -> Vector2
{
    const auto angle = fastrand_angle();

    return {cos(angle), sin(angle)};
}

auto cer::dot(const Vector2& lhs, const Vector2& rhs) -> float
{
    return (lhs.x * rhs.x) + (lhs.y * rhs.y);
}

auto cer::distance(const Vector2& lhs, const Vector2& rhs) -> float
{
    return length(rhs - lhs);
}

auto cer::distance_squared(const Vector2& lhs, const Vector2& rhs) -> float
{
    return length_squared(rhs - lhs);
}

auto cer::lerp(const Vector2& start, const Vector2& end, float t) -> Vector2
{
    return {
        lerp(start.x, end.x, t),
        lerp(start.y, end.y, t),
    };
}

auto cer::smoothstep(const Vector2& start, const Vector2& end, float t) -> Vector2
{
    return {
        smoothstep(start.x, end.x, t),
        smoothstep(start.y, end.y, t),
    };
}

auto cer::clamp(const Vector2& value, const Vector2& min, const Vector2& max) -> Vector2
{
    return {
        clamp(value.x, min.x, max.x),
        clamp(value.y, min.y, max.y),
    };
}

auto cer::is_zero(const Vector2& vector) -> bool
{
    return is_zero(vector.x) && is_zero(vector.y);
}

auto cer::are_equal_within(const Vector2& lhs, const Vector2& rhs, float threshold) -> bool
{
    return equal_within(lhs.x, rhs.x, threshold) && equal_within(lhs.y, rhs.y, threshold);
}

auto cer::min(const Vector2& lhs, const Vector2& rhs) -> Vector2
{
    return {
        min(lhs.x, rhs.x),
        min(lhs.y, rhs.y),
    };
}

auto cer::max(const Vector2& lhs, const Vector2& rhs) -> Vector2
{
    return {
        max(lhs.x, rhs.x),
        max(lhs.y, rhs.y),
    };
}

auto cer::line_normal(const Vector2& start, const Vector2& end) -> Vector2
{
    const auto dx = end.x - start.x;
    const auto dy = end.y - start.y;

    return normalize({-dy, dx});
}

auto cer::operator+=(Vector2& vector, const Vector2& rhs) -> Vector2&
{
    vector.x += rhs.x;
    vector.y += rhs.y;
    return vector;
}

auto cer::operator-=(Vector2& vector, const Vector2& rhs) -> Vector2&
{
    vector.x -= rhs.x;
    vector.y -= rhs.y;
    return vector;
}

auto cer::operator*=(Vector2& vector, const Vector2& rhs) -> Vector2&
{
    vector.x *= rhs.x;
    vector.y *= rhs.y;
    return vector;
}

auto cer::operator*=(Vector2& vector, float rhs) -> Vector2&
{
    vector.x *= rhs;
    vector.y *= rhs;
    return vector;
}

auto cer::operator/=(Vector2& vector, const Vector2& rhs) -> Vector2&
{
    vector.x /= rhs.x;
    vector.y /= rhs.y;
    return vector;
}

auto cer::operator/=(Vector2& vector, float rhs) -> Vector2&
{
    vector.x /= rhs;
    vector.y /= rhs;
    return vector;
}

auto cer::operator-(const Vector2& value) -> Vector2
{
    return {
        -value.x,
        -value.y,
    };
}

auto cer::operator+(const Vector2& lhs, const Vector2& rhs) -> Vector2
{
    return {
        lhs.x + rhs.x,
        lhs.y + rhs.y,
    };
}

auto cer::operator-(const Vector2& lhs, const Vector2& rhs) -> Vector2
{
    return {
        lhs.x - rhs.x,
        lhs.y - rhs.y,
    };
}

auto cer::operator*(const Vector2& lhs, const Vector2& rhs) -> Vector2
{
    return {
        lhs.x * rhs.x,
        lhs.y * rhs.y,
    };
}

auto cer::operator*(const Vector2& lhs, float rhs) -> Vector2
{
    return {
        lhs.x * rhs,
        lhs.y * rhs,
    };
}

auto cer::operator*(float lhs, const Vector2& rhs) -> Vector2
{
    return rhs * lhs;
}

auto cer::operator/(const Vector2& lhs, const Vector2& rhs) -> Vector2
{
    return {
        lhs.x / rhs.x,
        lhs.y / rhs.y,
    };
}

auto cer::operator/(const Vector2& lhs, float rhs) -> Vector2
{
    return {
        lhs.x / rhs,
        lhs.y / rhs,
    };
}