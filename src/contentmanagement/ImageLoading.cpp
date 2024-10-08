// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "ImageLoading.hpp"

#include "DDS.hpp"
#include "cerlib/Image.hpp"
#include "cerlib/Logging.hpp"
#include "cerlib/Math.hpp"
#include "contentmanagement/FileSystem.hpp"
#include "graphics/GraphicsDevice.hpp"
#include "util/InternalError.hpp"
#include <cstddef>
#include <gsl/narrow>

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4244)
#endif

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif

#include <stb_image.h>

#ifndef __ANDROID__
#include <stb_image_resize2.h>
#endif

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#include <cassert>
#include <gsl/util>

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

namespace cer::details
{
static auto try_load_misc(GraphicsDevice& device, std::span<const std::byte> memory)
    -> gsl::owner<ImageImpl*>
{
    const auto is_hdr = stbi_is_hdr_from_memory(reinterpret_cast<const stbi_uc*>(memory.data()),
                                                gsl::narrow<int>(memory.size())) != 0;

    int   width      = 0;
    int   height     = 0;
    int   comp       = 0;
    void* image_data = nullptr;

    if (is_hdr)
    {
        image_data = stbi_loadf_from_memory(reinterpret_cast<const stbi_uc*>(memory.data()),
                                            gsl::narrow<int>(memory.size()),
                                            &width,
                                            &height,
                                            &comp,
                                            STBI_rgb_alpha);
    }
    else
    {
        image_data = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(memory.data()),
                                           gsl::narrow<int>(memory.size()),
                                           &width,
                                           &height,
                                           &comp,
                                           4);
    }

    if (image_data == nullptr)
    {
        return nullptr;
    }

    if (width <= 0 || height <= 0 || comp <= 0)
    {
        CER_THROW_RUNTIME_ERROR_STR("Failed to load the image (invalid extents/channels).");
    }

    const auto _ = gsl::finally([&] {
        stbi_image_free(image_data);
    });

    const auto format = is_hdr ? ImageFormat::R32G32B32A32_Float : ImageFormat::R8G8B8A8_UNorm;

    return device.create_image(width, height, format, image_data);
}
} // namespace cer::details

auto cer::details::load_image(GraphicsDevice& device_impl, std::span<const std::byte> memory)
    -> gsl::not_null<cer::details::ImageImpl*>
{
    log_verbose("Loading image from memory. Span is {} bytes", memory.size());

    // Try loading misc image first

    if (const gsl::owner<ImageImpl*> image = try_load_misc(device_impl, memory))
    {
        return image; // NOLINT
    }

    if (const auto maybe_dds_image = dds::load(memory))
    {
        const auto& dds_image    = *maybe_dds_image;
        const auto& first_mipmap = dds_image.faces.front().mipmaps.front();

        return device_impl.create_image(dds_image.width,
                                        dds_image.height,
                                        dds_image.format,
                                        first_mipmap.data_span.data());
    }

    CER_THROW_RUNTIME_ERROR_STR("Failed to load the image.");
}

auto cer::details::load_image(GraphicsDevice& device_impl, std::string_view filename)
    -> gsl::not_null<ImageImpl*>
{
    return load_image(device_impl, filesystem::load_file_data_from_disk(filename));
}
