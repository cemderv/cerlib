// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "cerlib/Color.hpp"
#include "cerlib/Vector3.hpp"
#include "cerlib/Vector4.hpp"

namespace cer
{
auto Color::to_vector3() const -> Vector3
{
    return {r, g, b};
}

auto Color::to_vector4() const -> Vector4
{
    return {r, g, b, a};
}
} // namespace cer

auto cer::random_color(Option<float> alpha) -> Color
{
    if (!alpha.has_value())
    {
        alpha = random_float(0.0f, 1.0f);
    }

    return {
        random_float(0.0f, 1.0f),
        random_float(0.0f, 1.0f),
        random_float(0.0f, 1.0f),
        *alpha,
    };
}

auto cer::fastrand_color(Option<float> alpha) -> Color
{
    if (!alpha.has_value())
    {
        alpha = fastrand_float_zero_to_one();
    }

    return {
        fastrand_float_zero_to_one(),
        fastrand_float_zero_to_one(),
        fastrand_float_zero_to_one(),
        *alpha,
    };
}

auto cer::fastrand_color(const ColorInterval& interval) -> Color
{
    return {
        fastrand_float(interval.min.r, interval.max.r),
        fastrand_float(interval.min.g, interval.max.g),
        fastrand_float(interval.min.b, interval.max.b),
        fastrand_float(interval.min.a, interval.max.a),
    };
}


auto cer::operator+(const Color& lhs, const Color& rhs) -> Color
{
    return {
        lhs.r + rhs.r,
        lhs.g + rhs.g,
        lhs.b + rhs.b,
        lhs.a + rhs.a,
    };
}

auto cer::operator-(const Color& lhs, const Color& rhs) -> Color
{
    return {
        lhs.r - rhs.r,
        lhs.g - rhs.g,
        lhs.b - rhs.b,
        lhs.a - rhs.a,
    };
}

auto cer::operator*(const Color& lhs, float rhs) -> Color
{
    return {
        lhs.r * rhs,
        lhs.g * rhs,
        lhs.b * rhs,
        lhs.a * rhs,
    };
}

auto cer::operator*(float lhs, const Color& rhs) -> Color
{
    return {
        lhs * rhs.r,
        lhs * rhs.g,
        lhs * rhs.b,
        lhs * rhs.a,
    };
}
