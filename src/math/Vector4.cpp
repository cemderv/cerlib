// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "cerlib/Vector4.hpp"
#include "cerlib/Math.hpp"

auto cer::length(const Vector4& vector) -> float
{
    return std::sqrt(length_squared(vector));
}

auto cer::length_squared(const Vector4& vector) -> float
{
    return vector.x * vector.x + vector.y * vector.y + vector.z * vector.z + vector.w * vector.w;
}

auto cer::normalize(const Vector4& vector) -> Vector4
{
    const float len = length(vector);
    return is_zero(len) ? Vector4() : vector / len;
}

auto cer::round(const Vector4& vector) -> Vector4
{
    return {
        round(vector.x),
        round(vector.y),
        round(vector.z),
        round(vector.w),
    };
}

auto cer::abs(const Vector4& vector) -> Vector4
{
    return {
        abs(vector.x),
        abs(vector.y),
        abs(vector.z),
        abs(vector.w),
    };
}

auto cer::sin(const Vector4& vector) -> Vector4
{
    return {
        sin(vector.x),
        sin(vector.y),
        sin(vector.z),
        sin(vector.w),
    };
}

auto cer::cos(const Vector4& vector) -> Vector4
{
    return {
        cos(vector.x),
        cos(vector.y),
        cos(vector.z),
        cos(vector.w),
    };
}

auto cer::tan(const Vector4& vector) -> Vector4
{
    return {
        tan(vector.x),
        tan(vector.y),
        tan(vector.z),
        tan(vector.w),
    };
}

auto cer::pow(const Vector4& x, const Vector4& y) -> Vector4
{
    return {
        pow(x.x, y.x),
        pow(x.y, y.y),
        pow(x.z, y.z),
        pow(x.w, y.w),
    };
}

auto cer::floor(const Vector4& value) -> Vector4
{
    return {
        floor(value.x),
        floor(value.y),
        floor(value.z),
        floor(value.w),
    };
}

auto cer::ceiling(const Vector4& value) -> Vector4
{
    return {
        ceiling(value.x),
        ceiling(value.y),
        ceiling(value.z),
        ceiling(value.w),
    };
}

auto cer::random_vector4(float min, float max) -> Vector4
{
    return {
        random_float(min, max),
        random_float(min, max),
        random_float(min, max),
        random_float(min, max),
    };
}

auto cer::dot(const Vector4& lhs, const Vector4& rhs) -> float
{
    return (lhs.x * rhs.x) + (lhs.y * rhs.y) + (lhs.z * rhs.z) + (lhs.w * rhs.w);
}

auto cer::distance(const Vector4& lhs, const Vector4& rhs) -> float
{
    return length(rhs - lhs);
}

auto cer::distance_squared(const Vector4& lhs, const Vector4& rhs) -> float
{
    return length_squared(rhs - lhs);
}

auto cer::lerp(const Vector4& start, const Vector4& end, float t) -> Vector4
{
    return {
        lerp(start.x, end.x, t),
        lerp(start.y, end.y, t),
        lerp(start.z, end.z, t),
        lerp(start.w, end.w, t),
    };
}

auto cer::smoothstep(const Vector4& start, const Vector4& end, float t) -> Vector4
{
    return {
        smoothstep(start.x, end.x, t),
        smoothstep(start.y, end.y, t),
        smoothstep(start.z, end.z, t),
        smoothstep(start.w, end.w, t),
    };
}

auto cer::clamp(const Vector4& value, const Vector4& min, const Vector4& max) -> Vector4
{
    return {
        clamp(value.x, min.x, max.x),
        clamp(value.y, min.y, max.y),
        clamp(value.z, min.z, max.z),
        clamp(value.w, min.w, max.w),
    };
}

auto cer::is_zero(const Vector4& vector) -> bool
{
    return is_zero(vector.x) && is_zero(vector.y) && is_zero(vector.z) && is_zero(vector.w);
}

auto cer::are_equal_within(const Vector4& lhs, const Vector4& rhs, float threshold) -> bool
{
    return equal_within(lhs.x, rhs.x, threshold) && equal_within(lhs.y, rhs.y, threshold) &&
           equal_within(lhs.z, rhs.z, threshold) && equal_within(lhs.w, rhs.w, threshold);
}

auto cer::min(const Vector4& lhs, const Vector4& rhs) -> Vector4
{
    return {
        min(lhs.x, rhs.x),
        min(lhs.y, rhs.y),
        min(lhs.z, rhs.z),
        min(lhs.w, rhs.w),
    };
}

auto cer::max(const Vector4& lhs, const Vector4& rhs) -> Vector4
{
    return {
        max(lhs.x, rhs.x),
        max(lhs.y, rhs.y),
        max(lhs.z, rhs.z),
        max(lhs.w, rhs.w),
    };
}

auto cer::operator+=(Vector4& vector, const Vector4& rhs) -> Vector4&
{
    vector.x += rhs.x;
    vector.y += rhs.y;
    vector.z += rhs.z;
    vector.w += rhs.w;
    return vector;
}

auto cer::operator-=(Vector4& vector, const Vector4& rhs) -> Vector4&
{
    vector.x -= rhs.x;
    vector.y -= rhs.y;
    vector.z -= rhs.z;
    vector.w -= rhs.w;
    return vector;
}

auto cer::operator*=(Vector4& vector, const Vector4& rhs) -> Vector4&
{
    vector.x *= rhs.x;
    vector.y *= rhs.y;
    vector.z *= rhs.z;
    vector.w *= rhs.w;
    return vector;
}

auto cer::operator*=(Vector4& vector, float rhs) -> Vector4&
{
    vector.x *= rhs;
    vector.y *= rhs;
    vector.z *= rhs;
    vector.w *= rhs;
    return vector;
}

auto cer::operator/=(Vector4& vector, const Vector4& rhs) -> Vector4&
{
    vector.x /= rhs.x;
    vector.y /= rhs.y;
    vector.z /= rhs.z;
    vector.w /= rhs.w;
    return vector;
}

auto cer::operator/=(Vector4& vector, float rhs) -> Vector4&
{
    vector.x /= rhs;
    vector.y /= rhs;
    vector.z /= rhs;
    vector.w /= rhs;
    return vector;
}

auto cer::operator-(const Vector4& value) -> Vector4
{
    return {
        -value.x,
        -value.y,
        -value.z,
        -value.w,
    };
}

auto cer::operator+(const Vector4& lhs, const Vector4& rhs) -> Vector4
{
    return {
        lhs.x + rhs.x,
        lhs.y + rhs.y,
        lhs.z + rhs.z,
        lhs.w + rhs.w,
    };
}

auto cer::operator-(const Vector4& lhs, const Vector4& rhs) -> Vector4
{
    return {
        lhs.x - rhs.x,
        lhs.y - rhs.y,
        lhs.z - rhs.z,
        lhs.w - rhs.w,
    };
}

auto cer::operator*(const Vector4& lhs, const Vector4& rhs) -> Vector4
{
    return {
        lhs.x * rhs.x,
        lhs.y * rhs.y,
        lhs.z * rhs.z,
        lhs.w * rhs.w,
    };
}

auto cer::operator*(const Vector4& lhs, float rhs) -> Vector4
{
    return {
        lhs.x * rhs,
        lhs.y * rhs,
        lhs.z * rhs,
        lhs.w * rhs,
    };
}

auto cer::operator*(float lhs, const Vector4& rhs) -> Vector4
{
    return rhs * lhs;
}

auto cer::operator/(const Vector4& lhs, const Vector4& rhs) -> Vector4
{
    return {
        lhs.x / rhs.x,
        lhs.y / rhs.y,
        lhs.z / rhs.z,
        lhs.w / rhs.w,
    };
}

auto cer::operator/(const Vector4& lhs, float rhs) -> Vector4
{
    return {
        lhs.x / rhs,
        lhs.y / rhs,
        lhs.z / rhs,
        lhs.w / rhs,
    };
}
