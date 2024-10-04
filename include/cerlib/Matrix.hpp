// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <limits>

namespace cer
{
struct Vector2;

/**
 * Represents a floating-point (single-precision) 4x4, row-major matrix.
 *
 * @ingroup Math
 */
struct Matrix
{
    /**
     * Default constructor. Creates an identity matrix.
     */
    constexpr Matrix() = default;

    /**
     * Creates a matrix with all of its components specified separately.
     *
     * @param m11 The value of row 1, column 1
     * @param m12 The value of row 1, column 2
     * @param m13 The value of row 1, column 3
     * @param m14 The value of row 1, column 4
     * @param m21 The value of row 2, column 1
     * @param m22 The value of row 2, column 2
     * @param m23 The value of row 2, column 3
     * @param m24 The value of row 2, column 4
     * @param m31 The value of row 3, column 1
     * @param m32 The value of row 3, column 2
     * @param m33 The value of row 3, column 3
     * @param m34 The value of row 3, column 4
     * @param m41 The value of row 4, column 1
     * @param m42 The value of row 4, column 2
     * @param m43 The value of row 4, column 3
     * @param m44 The value of row 4, column 4
     */
    constexpr Matrix(float m11,
                     float m12,
                     float m13,
                     float m14,
                     float m21,
                     float m22,
                     float m23,
                     float m24,
                     float m31,
                     float m32,
                     float m33,
                     float m34,
                     float m41,
                     float m42,
                     float m43,
                     float m44);

    constexpr explicit Matrix(float diagonal_value);

    /**
     * Gets a pointer to the beginning of the matrix's data.
     */
    auto data() const -> const float*;

    /**
     * Gets the beginning iterator of the matrix.
     */
    auto begin() -> float*;

    /**
     * Gets the beginning iterator of the matrix.
     */
    auto begin() const -> const float*;

    /**
     * Gets the beginning iterator of the matrix.
     */
    auto cbegin() const -> const float*;

    /**
     * Gets the end iterator of the matrix.
     */
    auto end() -> float*;

    /**
     * Gets the end iterator of the matrix.
     */
    auto end() const -> const float*;

    /**
     * Gets the end iterator of the matrix.
     */
    auto cend() const -> const float*;

    /** Default comparison */
    auto operator==(const Matrix&) const -> bool = default;

    /** Default comparison */
    auto operator!=(const Matrix&) const -> bool = default;

    float m11 = 1;
    float m12 = 0;
    float m13 = 0;
    float m14 = 0;
    float m21 = 0;
    float m22 = 1;
    float m23 = 0;
    float m24 = 0;
    float m31 = 0;
    float m32 = 0;
    float m33 = 1;
    float m34 = 0;
    float m41 = 0;
    float m42 = 0;
    float m43 = 0;
    float m44 = 1;
};

/**
 * Transposes a matrix.
 *
 * @param matrix The matrix to transpose.
 *
 * @ingroup Math
 */
auto transpose(const Matrix& matrix) -> Matrix;

/**
 * Creates a translation matrix.
 *
 * @param translation The resulting translation of the matrix.
 *
 * @ingroup Math
 */
auto translate(Vector2 translation) -> Matrix;

/**
 * Creates a scaling matrix.
 *
 * @param scale The scale factor along the X and Y axes.
 *
 * @ingroup Math
 */
auto scale(Vector2 scale) -> Matrix;

/**
 * Creates a matrix that rotates around the Z-axis.
 *
 * @param radians The rotation amount, in radians.
 *
 * @ingroup Math
 */
auto rotate(float radians) -> Matrix;

/**
 * Gets a value indicating whether two matrices are equal within a specific
 * threshold.
 *
 * @param lhs The first vector to compare.
 * @param rhs The second vector to compare.
 * @param threshold The threshold within which both vectors count as equal to one
 * another. The comparison is performed one a per-component basis.
 *
 * @ingroup Math
 */
auto are_equal_within(const Matrix& lhs,
                      const Matrix& rhs,
                      float         threshold = std::numeric_limits<float>::epsilon()) -> bool;

auto operator*(const Matrix& lhs, const Matrix& rhs) -> Matrix;
} // namespace cer

#include <cerlib/Vector2.hpp>

namespace cer
{
constexpr Matrix::Matrix(float m11,
                         float m12,
                         float m13,
                         float m14,
                         float m21,
                         float m22,
                         float m23,
                         float m24,
                         float m31,
                         float m32,
                         float m33,
                         float m34,
                         float m41,
                         float m42,
                         float m43,
                         float m44)
    : m11(m11)
    , m12(m12)
    , m13(m13)
    , m14(m14)
    , m21(m21)
    , m22(m22)
    , m23(m23)
    , m24(m24)
    , m31(m31)
    , m32(m32)
    , m33(m33)
    , m34(m34)
    , m41(m41)
    , m42(m42)
    , m43(m43)
    , m44(m44)
{
}

constexpr Matrix::Matrix(float diagonal_value)
    : m11(diagonal_value)
    , m22(diagonal_value)
    , m33(diagonal_value)
    , m44(diagonal_value)
{
}

inline auto Matrix::data() const -> const float*
{
    return &m11;
}

inline auto Matrix::begin() -> float*
{
    return &m11;
}

inline auto Matrix::begin() const -> const float*
{
    return &m11;
}

inline auto Matrix::cbegin() const -> const float*
{
    return &m11;
}

inline auto Matrix::end() -> float*
{
    return &m11 + 16;
}

inline auto Matrix::end() const -> const float*
{
    return &m11 + 16;
}

inline auto Matrix::cend() const -> const float*
{
    return &m11 + 16;
}

inline auto operator*(const Matrix& lhs, const Matrix& rhs) -> Matrix
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
} // namespace cer

inline auto cer::transpose(const Matrix& matrix) -> Matrix
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

inline auto cer::translate(Vector2 translation) -> Matrix
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

inline auto cer::scale(Vector2 scale) -> Matrix
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

inline auto cer::rotate(float radians) -> Matrix
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

inline auto cer::are_equal_within(const Matrix& lhs, const Matrix& rhs, float threshold) -> bool
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
