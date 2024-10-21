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
} // namespace cer
