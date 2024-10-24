// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <span>

namespace cer::details
{
class GraphicsDevice;
class ImageImpl;

auto load_image(GraphicsDevice& device_impl, std::span<const std::byte> memory)
    -> std::unique_ptr<ImageImpl>;

auto load_image(GraphicsDevice& device_impl, std::string_view filename)
    -> std::unique_ptr<ImageImpl>;
} // namespace cer::details
