// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "ImageLoading.hpp"

#include "DDS.hpp"
#include "cerlib/Image.hpp"
#include "cerlib/Logging.hpp"
#include "contentmanagement/FileSystem.hpp"
#include "graphics/GraphicsDevice.hpp"
#include "graphics/ImageImpl.hpp"
#include "util/narrow_cast.hpp"
#include <cstddef>

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4244)
#endif

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif

#include "graphics/stb_image.hpp"

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#include "util/narrow_cast.hpp"
#include <cassert>

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

namespace cer::details
{
static auto try_load_misc(GraphicsDevice& device, std::span<const std::byte> memory)
    -> std::unique_ptr<ImageImpl>
{
    const auto is_hdr = stbi_is_hdr_from_memory(reinterpret_cast<const stbi_uc*>(memory.data()),
                                                narrow<int>(memory.size())) != 0;

    int   width      = 0;
    int   height     = 0;
    int   comp       = 0;
    void* image_data = nullptr;

    if (is_hdr)
    {
        image_data = stbi_loadf_from_memory(reinterpret_cast<const stbi_uc*>(memory.data()),
                                            narrow<int>(memory.size()),
                                            &width,
                                            &height,
                                            &comp,
                                            STBI_rgb_alpha);
    }
    else
    {
        image_data = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(memory.data()),
                                           narrow<int>(memory.size()),
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
        throw std::runtime_error{"Failed to load the image (invalid extents/channels)."};
    }

    defer
    {
        stbi_image_free(image_data);
    };

    const auto format = is_hdr ? ImageFormat::R32G32B32A32_Float : ImageFormat::R8G8B8A8_UNorm;

    return device.create_image(width, height, format, image_data);
}
} // namespace cer::details

auto cer::details::load_image(GraphicsDevice& device_impl, std::span<const std::byte> memory)
    -> std::unique_ptr<ImageImpl>
{
    log_verbose("Loading image from memory. Span is {} bytes", memory.size());

    // Try loading misc image first

    if (auto image = try_load_misc(device_impl, memory))
    {
        return image;
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

    throw std::runtime_error{"Failed to load the image (unknown image type)."};
}

auto cer::details::load_image(GraphicsDevice& device_impl, std::string_view filename)
    -> std::unique_ptr<ImageImpl>
{
    return load_image(device_impl, filesystem::load_file_data_from_disk(filename));
}
