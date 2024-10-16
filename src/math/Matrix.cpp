// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "cerlib/Matrix.hpp"
#include "cerlib/Vector2.hpp"

namespace cer
{
auto Matrix::data() const -> const float*
{
    return &m11;
}

auto Matrix::begin() -> float*
{
    return &m11;
}

auto Matrix::begin() const -> const float*
{
    return &m11;
}

auto Matrix::cbegin() const -> const float*
{
    return &m11;
}

auto Matrix::end() -> float*
{
    return &m11 + 16;
}

auto Matrix::end() const -> const float*
{
    return &m11 + 16;
}

auto Matrix::cend() const -> const float*
{
    return &m11 + 16;
}
} // namespace cer

auto cer::operator*(const Matrix& lhs, const Matrix& rhs) -> Matrix
{
    return {
        (lhs.m11 * rhs.m11) + (lhs.m12 * rhs.m21) + (lhs.m13 * rhs.m31) + (lhs.m14 * rhs.m41),
        (lhs.m11 * rhs.m12) + (lhs.m12 * rhs.m22) + (lhs.m13 * rhs.m32) + (lhs.m14 * rhs.m42),
        (lhs.m11 * rhs.m13) + (lhs.m12 * rhs.m23) + (lhs.m13 * rhs.m33) + (lhs.m14 * rhs.m43),
        (lhs.m11 * rhs.m14) + (lhs.m12 * rhs.m24) + (lhs.m13 * rhs.m34) + (lhs.m14 * rhs.m44),
        (lhs.m21 * rhs.m11) + (lhs.m22 * rhs.m21) + (lhs.m23 * rhs.m31) + (lhs.m24 * rhs.m41),
        (lhs.m21 * rhs.m12) + (lhs.m22 * rhs.m22) + (lhs.m23 * rhs.m32) + (lhs.m24 * rhs.m42),
        (lhs.m21 * rhs.m13) + (lhs.m22 * rhs.m23) + (lhs.m23 * rhs.m33) + (lhs.m24 * rhs.m43),
        (lhs.m21 * rhs.m14) + (lhs.m22 * rhs.m24) + (lhs.m23 * rhs.m34) + (lhs.m24 * rhs.m44),
        (lhs.m31 * rhs.m11) + (lhs.m32 * rhs.m21) + (lhs.m33 * rhs.m31) + (lhs.m34 * rhs.m41),
        (lhs.m31 * rhs.m12) + (lhs.m32 * rhs.m22) + (lhs.m33 * rhs.m32) + (lhs.m34 * rhs.m42),
        (lhs.m31 * rhs.m13) + (lhs.m32 * rhs.m23) + (lhs.m33 * rhs.m33) + (lhs.m34 * rhs.m43),
        (lhs.m31 * rhs.m14) + (lhs.m32 * rhs.m24) + (lhs.m33 * rhs.m34) + (lhs.m34 * rhs.m44),
        (lhs.m41 * rhs.m11) + (lhs.m42 * rhs.m21) + (lhs.m43 * rhs.m31) + (lhs.m44 * rhs.m41),
        (lhs.m41 * rhs.m12) + (lhs.m42 * rhs.m22) + (lhs.m43 * rhs.m32) + (lhs.m44 * rhs.m42),
        (lhs.m41 * rhs.m13) + (lhs.m42 * rhs.m23) + (lhs.m43 * rhs.m33) + (lhs.m44 * rhs.m43),
        (lhs.m41 * rhs.m14) + (lhs.m42 * rhs.m24) + (lhs.m43 * rhs.m34) + (lhs.m44 * rhs.m44),
    };
}

auto cer::transpose(const Matrix& matrix) -> Matrix
{
    // NOLINTBEGIN
    return {
        matrix.m11,
        matrix.m21,
        matrix.m31,
        matrix.m41,
        matrix.m12,
        matrix.m22,
        matrix.m32,
        matrix.m42,
        matrix.m13,
        matrix.m23,
        matrix.m33,
        matrix.m43,
        matrix.m14,
        matrix.m24,
        matrix.m34,
        matrix.m44,
    };
    // NOLINTEND
}

auto cer::translate(Vector2 translation) -> Matrix
{
    const float x = translation.x;
    const float y = translation.y;

    return {
        1,
        0,
        0,
        0,
        0,
        1,
        0,
        0,
        0,
        0,
        1,
        0,
        x,
        y,
        0,
        1,
    };
}

auto cer::scale(Vector2 scale) -> Matrix
{
    return {
        scale.x,
        0,
        0,
        0,
        0,
        scale.y,
        0,
        0,
        0,
        0,
        1,
        0,
        0,
        0,
        0,
        1,
    };
}

auto cer::rotate(float radians) -> Matrix
{
    const auto c = cos(radians);
    const auto s = sin(radians);

    return {
        c,
        s,
        0,
        0,
        -s,
        c,
        0,
        0,
        0,
        0,
        1,
        0,
        0,
        0,
        0,
        1,
    };
}

auto cer::are_equal_within(const Matrix& lhs, const Matrix& rhs, float threshold) -> bool
{
    return equal_within(lhs.m11, rhs.m11, threshold) && equal_within(lhs.m12, rhs.m12, threshold) &&
           equal_within(lhs.m13, rhs.m13, threshold) && equal_within(lhs.m14, rhs.m14, threshold) &&
           equal_within(lhs.m21, rhs.m21, threshold) && equal_within(lhs.m22, rhs.m22, threshold) &&
           equal_within(lhs.m23, rhs.m23, threshold) && equal_within(lhs.m24, rhs.m24, threshold) &&
           equal_within(lhs.m31, rhs.m31, threshold) && equal_within(lhs.m32, rhs.m32, threshold) &&
           equal_within(lhs.m33, rhs.m33, threshold) && equal_within(lhs.m34, rhs.m34, threshold) &&
           equal_within(lhs.m41, rhs.m41, threshold) && equal_within(lhs.m42, rhs.m42, threshold) &&
           equal_within(lhs.m43, rhs.m43, threshold) && equal_within(lhs.m44, rhs.m44, threshold);
}