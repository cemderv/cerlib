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
static const void* misc_image_data_upload(
    const void*                                         base_image_data,
    const SmallVector<std::unique_ptr<std::byte[]>, 4>& mipmap_datas,
    [[maybe_unused]] uint32_t                           array_index,
    uint32_t                                            mipmap)
{
    assert(array_index == 0);
    return mipmap == 0 ? base_image_data : mipmap_datas[mipmap - 1].get();
}

static gsl::owner<ImageImpl*> try_load_misc(GraphicsDevice&            device,
                                            std::span<const std::byte> memory,
                                            [[maybe_unused]] bool      generate_mipmaps)
{
    const bool is_hdr = stbi_is_hdr_from_memory(reinterpret_cast<const stbi_uc*>(memory.data()),
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

    const auto _ = gsl::finally([&] { stbi_image_free(image_data); });

    uint32_t mipmap_count = 1;

    // Contains data of additional mipmaps (m > 0)
    SmallVector<std::unique_ptr<std::byte[]>, 4> mipmap_datas{};

#ifndef __ANDROID__
    if (generate_mipmaps && !is_hdr)
    {
        mipmap_count = max_mipmap_count_for_extent(min(width, height));

        uint32_t w = static_cast<uint32_t>(width);
        uint32_t h = static_cast<uint32_t>(height);

        for (uint32_t i = 0; i < mipmap_count - 1; ++i)
        {
            const void* src_data = i == 0 ? image_data : mipmap_datas[i - 1].get();

            const uint32_t output_width  = max(w >> 1, 1u);
            const uint32_t output_height = max(h >> 1, 1u);

            auto dst_data = std::make_unique<std::byte[]>(static_cast<size_t>(output_width) *
                                                          static_cast<size_t>(output_height) * 4);

            stbir_resize_uint8_srgb(static_cast<const stbi_uc*>(src_data),
                                    static_cast<int>(w),
                                    static_cast<int>(h),
                                    0,
                                    reinterpret_cast<stbi_uc*>(dst_data.get()),
                                    static_cast<int>(output_width),
                                    static_cast<int>(output_height),
                                    0,
                                    STBIR_RGBA);

            mipmap_datas.push_back(std::move(dst_data));

            w = output_width;
            h = output_height;
        }
    }
#endif

    const ImageFormat format =
        is_hdr ? ImageFormat::R32G32B32A32_Float : ImageFormat::R8G8B8A8_UNorm;

    return device.create_image(width, height, format, mipmap_count, [&](uint32_t mipmap) {
        return misc_image_data_upload(image_data, mipmap_datas, 0, mipmap);
    });
}
} // namespace cer::details

gsl::not_null<cer::details::ImageImpl*> cer::details::load_image(GraphicsDevice& device_impl,
                                                                 std::span<const std::byte> memory,
                                                                 bool generate_mipmaps)
{
    log_verbose("Loading image from memory. Span is {} bytes, generate_mipmaps={}",
                memory.size(),
                generate_mipmaps);

    // Try loading misc image first

    if (const gsl::owner<ImageImpl*> image = try_load_misc(device_impl, memory, generate_mipmaps))
    {
        return image; // NOLINT
    }

    if (const std::optional<dds::DDSImage> maybe_dds_image = dds::load(memory))
    {
        const dds::DDSImage& dds_image = *maybe_dds_image;

        return device_impl.create_image(
            dds_image.width,
            dds_image.height,
            dds_image.format,
            gsl::narrow<uint32_t>(dds_image.faces.front().mipmaps.size()),
            [&](uint32_t mipmap) { return dds_image_data_upload(dds_image, 0, mipmap); });
    }

    CER_THROW_RUNTIME_ERROR_STR("Failed to load the image.");
}

gsl::not_null<cer::details::ImageImpl*> cer::details::load_image(GraphicsDevice&  device_impl,
                                                                 std::string_view filename,
                                                                 bool             generate_mipmaps)
{
    const std::vector<std::byte> file_data = filesystem::load_file_data_from_disk(filename);

    return load_image(device_impl, file_data, generate_mipmaps);
}
