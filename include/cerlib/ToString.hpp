// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <cerlib/Export.hpp>
#include <string>

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
} // namespace cer

namespace std
{
CERLIB_API std::string to_string(const cer::Color& value);

CERLIB_API std::string to_string(const cer::Vector2& value);

CERLIB_API std::string to_string(const cer::Vector3& value);

CERLIB_API std::string to_string(const cer::Vector4& value);

CERLIB_API std::string to_string(const cer::Matrix& value);

CERLIB_API std::string to_string(cer::ImageFormat value);

CERLIB_API std::string to_string(const cer::Image& value);

CERLIB_API std::string to_string(const cer::Rectangle& value);
} // namespace std
