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
std::string to_string(const cer::Color& value);

std::string to_string(const cer::Vector2& value);

std::string to_string(const cer::Vector3& value);

std::string to_string(const cer::Vector4& value);

std::string to_string(const cer::Matrix& value);

std::string to_string(cer::ImageFormat value);

std::string to_string(const cer::Image& value);

std::string to_string(const cer::Rectangle& value);
} // namespace std
