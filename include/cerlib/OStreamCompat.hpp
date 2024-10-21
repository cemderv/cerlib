// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <cerlib/details/ObjectMacros.hpp>
#include <iosfwd>

namespace cer
{
struct Color;
struct Vector2;
struct Vector3;
struct Vector4;
struct Matrix;
class Image;
struct Rectangle;
enum class ImageFormat;

auto operator<<(std::ostream& os, const Color& value) -> std::ostream&;

auto operator<<(std::ostream& os, const Vector2& value) -> std::ostream&;

auto operator<<(std::ostream& os, const Vector3& value) -> std::ostream&;

auto operator<<(std::ostream& os, const Vector4& value) -> std::ostream&;

auto operator<<(std::ostream& os, const Matrix& value) -> std::ostream&;

auto operator<<(std::ostream& os, ImageFormat value) -> std::ostream&;

auto operator<<(std::ostream& os, const Image& value) -> std::ostream&;

auto operator<<(std::ostream& os, const Rectangle& value) -> std::ostream&;
} // namespace cer
