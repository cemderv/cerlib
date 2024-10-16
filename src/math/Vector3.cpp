// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "cerlib/Vector3.hpp"
#include "cerlib/Math.hpp"

auto cer::length(const Vector3& vector) -> float
{
    return std::sqrt(length_squared(vector));
}

auto cer::length_squared(const Vector3& vector) -> float
{
    return vector.x * vector.x + vector.y * vector.y + vector.z * vector.z;
}

auto cer::normalize(const Vector3& vector) -> cer::Vector3
{
    const auto len = length(vector);
    return is_zero(len) ? Vector3() : vector / len;
}

auto cer::round(const Vector3& vector) -> cer::Vector3
{
    return {
        round(vector.x),
        round(vector.y),
        round(vector.z),
    };
}

auto cer::abs(const Vector3& vector) -> cer::Vector3
{
    return {
        abs(vector.x),
        abs(vector.y),
        abs(vector.z),
    };
}

auto cer::sin(const Vector3& vector) -> cer::Vector3
{
    return {
        sin(vector.x),
        sin(vector.y),
        sin(vector.z),
    };
}

auto cer::cos(const Vector3& vector) -> cer::Vector3
{
    return {
        cos(vector.x),
        cos(vector.y),
        cos(vector.z),
    };
}

auto cer::tan(const Vector3& vector) -> cer::Vector3
{
    return {
        tan(vector.x),
        tan(vector.y),
        tan(vector.z),
    };
}

auto cer::pow(const Vector3& base, const Vector3& exp) -> cer::Vector3
{
    return {
        pow(base.x, exp.x),
        pow(base.y, exp.y),
        pow(base.z, exp.z),
    };
}

auto cer::floor(const Vector3& value) -> cer::Vector3
{
    return {
        floor(value.x),
        floor(value.y),
        floor(value.z),
    };
}

auto cer::ceiling(const Vector3& value) -> cer::Vector3
{
    return {
        ceiling(value.x),
        ceiling(value.y),
        ceiling(value.z),
    };
}

auto cer::random_vector3(float min, float max) -> cer::Vector3
{
    return {
        random_float(min, max),
        random_float(min, max),
        random_float(min, max),
    };
}

auto cer::dot(const Vector3& lhs, const Vector3& rhs) -> float
{
    return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

auto cer::distance(const Vector3& lhs, const Vector3& rhs) -> float
{
    return length(rhs - lhs);
}

auto cer::distance_squared(const Vector3& lhs, const Vector3& rhs) -> float
{
    return length_squared(rhs - lhs);
}

auto cer::lerp(const Vector3& start, const Vector3& end, float t) -> cer::Vector3
{
    return {
        lerp(start.x, end.x, t),
        lerp(start.y, end.y, t),
        lerp(start.z, end.z, t),
    };
}

auto cer::smoothstep(const Vector3& start, const Vector3& end, float t) -> cer::Vector3
{
    return {
        smoothstep(start.x, end.x, t),
        smoothstep(start.y, end.y, t),
        smoothstep(start.z, end.z, t),
    };
}

auto cer::clamp(const Vector3& value, const Vector3& min, const Vector3& max) -> cer::Vector3
{
    return {
        clamp(value.x, min.x, max.x),
        clamp(value.y, min.y, max.y),
        clamp(value.z, min.z, max.z),
    };
}

auto cer::is_zero(const Vector3& vector) -> bool
{
    return is_zero(vector.x) && is_zero(vector.y) && is_zero(vector.z);
}

auto cer::are_equal_within(const Vector3& lhs, const Vector3& rhs, float threshold) -> bool
{
    return equal_within(lhs.x, rhs.x, threshold) && equal_within(lhs.y, rhs.y, threshold) &&
           equal_within(lhs.z, rhs.z, threshold);
}

auto cer::min(const Vector3& lhs, const Vector3& rhs) -> cer::Vector3
{
    return {
        min(lhs.x, rhs.x),
        min(lhs.y, rhs.y),
        min(lhs.z, rhs.z),
    };
}

auto cer::max(const Vector3& lhs, const Vector3& rhs) -> cer::Vector3
{
    return {
        max(lhs.x, rhs.x),
        max(lhs.y, rhs.y),
        max(lhs.z, rhs.z),
    };
}

auto cer::operator+=(Vector3& vector, const Vector3& rhs) -> cer::Vector3&
{
    vector.x += rhs.x;
    vector.y += rhs.y;
    vector.z += rhs.z;
    return vector;
}

auto cer::operator-=(Vector3& vector, const Vector3& rhs) -> cer::Vector3&
{
    vector.x -= rhs.x;
    vector.y -= rhs.y;
    vector.z -= rhs.z;
    return vector;
}

auto cer::operator*=(Vector3& vector, const Vector3& rhs) -> cer::Vector3&
{
    vector.x *= rhs.x;
    vector.y *= rhs.y;
    vector.z *= rhs.z;
    return vector;
}

auto cer::operator*=(Vector3& vector, float rhs) -> cer::Vector3&
{
    vector.x *= rhs;
    vector.y *= rhs;
    vector.z *= rhs;
    return vector;
}

auto cer::operator/=(Vector3& vector, const Vector3& rhs) -> cer::Vector3&
{
    vector.x /= rhs.x;
    vector.y /= rhs.y;
    vector.z /= rhs.z;
    return vector;
}

auto cer::operator/=(Vector3& vector, float rhs) -> cer::Vector3&
{
    vector.x /= rhs;
    vector.y /= rhs;
    vector.z /= rhs;
    return vector;
}

auto cer::operator-(const Vector3& value) -> cer::Vector3
{
    return {
        -value.x,
        -value.y,
        -value.z,
    };
}

auto cer::operator+(const Vector3& lhs, const Vector3& rhs) -> cer::Vector3
{
    return {
        lhs.x + rhs.x,
        lhs.y + rhs.y,
        lhs.z + rhs.z,
    };
}

auto cer::operator-(const Vector3& lhs, const Vector3& rhs) -> cer::Vector3
{
    return {
        lhs.x - rhs.x,
        lhs.y - rhs.y,
        lhs.z - rhs.z,
    };
}

auto cer::operator*(const Vector3& lhs, const Vector3& rhs) -> cer::Vector3
{
    return {
        lhs.x * rhs.x,
        lhs.y * rhs.y,
        lhs.z * rhs.z,
    };
}

auto cer::operator*(const Vector3& lhs, float rhs) -> cer::Vector3
{
    return {
        lhs.x * rhs,
        lhs.y * rhs,
        lhs.z * rhs,
    };
}

auto cer::operator*(float lhs, const Vector3& rhs) -> cer::Vector3
{
    return rhs * lhs;
}

auto cer::operator/(const Vector3& lhs, const Vector3& rhs) -> cer::Vector3
{
    return {
        lhs.x / rhs.x,
        lhs.y / rhs.y,
        lhs.z / rhs.z,
    };
}

auto cer::operator/(const Vector3& lhs, float rhs) -> cer::Vector3
{
    return {
        lhs.x / rhs,
        lhs.y / rhs,
        lhs.z / rhs,
    };
}
