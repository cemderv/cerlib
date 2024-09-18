// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <cerlib/Export.hpp>
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

CERLIB_API std::ostream& operator<<(std::ostream& os, const Color& value);

CERLIB_API std::ostream& operator<<(std::ostream& os, const Vector2& value);

CERLIB_API std::ostream& operator<<(std::ostream& os, const Vector3& value);

CERLIB_API std::ostream& operator<<(std::ostream& os, const Vector4& value);

CERLIB_API std::ostream& operator<<(std::ostream& os, const Matrix& value);

CERLIB_API std::ostream& operator<<(std::ostream& os, ImageFormat value);

CERLIB_API std::ostream& operator<<(std::ostream& os, const Image& value);

CERLIB_API std::ostream& operator<<(std::ostream& os, const Rectangle& value);
} // namespace cer
