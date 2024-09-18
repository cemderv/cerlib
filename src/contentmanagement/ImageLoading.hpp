// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <gsl/pointers>
#include <span>

namespace cer::details
{
class GraphicsDevice;
class ImageImpl;

gsl::not_null<ImageImpl*> load_image(GraphicsDevice&            device_impl,
                                     std::span<const std::byte> memory,
                                     bool                       generate_mipmaps);

gsl::not_null<ImageImpl*> load_image(GraphicsDevice&  device_impl,
                                     std::string_view filename,
                                     bool             generate_mipmaps);
} // namespace cer::details
