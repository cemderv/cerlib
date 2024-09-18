// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "cerlib/Image.hpp"

#include "GraphicsDevice.hpp"
#include "ImageImpl.hpp"
#include "cerlib/Window.hpp"
#include "contentmanagement/ImageLoading.hpp"
#include "game/GameImpl.hpp"
#include "util/InternalError.hpp"
#include "util/Util.hpp"

#define DECLARE_IMAGE_IMPL                                                                         \
    const auto impl = static_cast<details::ImageImpl*>(this->impl());                              \
    VERIFY_IMPL_ACCESS

namespace cer
{
CERLIB_IMPLEMENT_DERIVED_OBJECT(GraphicsResource, Image);

Image::Image(
    uint32_t width, uint32_t height, ImageFormat format, uint32_t mipmap_count, const void* data)
{
    if (data == nullptr)
    {
        CER_THROW_INVALID_ARG(
            "No image data specified (width={}; height={}; format={}, mipmapCount={}).",
            width,
            height,
            image_format_name(format),
            mipmap_count);
    }

    LOAD_DEVICE_IMPL;

    set_impl(
        *this,
        device_impl
            .create_image(width, height, format, mipmap_count, [data](uint32_t) { return data; })
            .get());
}

Image::Image(uint32_t            width,
             uint32_t            height,
             ImageFormat         format,
             uint32_t            mipmap_count,
             const DataCallback& data_callback)
{
    if (!data_callback)
    {
        CER_THROW_INVALID_ARG_STR("No image data callback specified.");
    }

    LOAD_DEVICE_IMPL;

    set_impl(*this,
             device_impl.create_image(width, height, format, mipmap_count, data_callback).get());
}

Image::Image(std::span<const std::byte> memory, bool generate_mipmaps)
{
    LOAD_DEVICE_IMPL;
    set_impl(*this, details::load_image(device_impl, memory, generate_mipmaps).get());
}

Image::Image(std::string_view filename, bool generate_mipmaps)
{
    LOAD_DEVICE_IMPL;
    set_impl(*this, details::load_image(device_impl, filename, generate_mipmaps).get());
}

Image::Image(uint32_t width, uint32_t height, ImageFormat format, const Window& window)
{
    if (!window)
    {
        CER_THROW_INVALID_ARG_STR("No window specified.");
    }

    LOAD_DEVICE_IMPL;
    set_impl(*this, device_impl.create_canvas(window, width, height, format).get());
}

bool Image::is_canvas() const
{
    DECLARE_IMAGE_IMPL;
    return impl->is_canvas();
}

uint32_t Image::width() const
{
    DECLARE_IMAGE_IMPL;
    return impl->width();
}

uint32_t Image::height() const
{
    DECLARE_IMAGE_IMPL;
    return impl->height();
}

float Image::widthf() const
{
    return static_cast<float>(width());
}

float Image::heightf() const
{
    return static_cast<float>(height());
}

Vector2 Image::size() const
{
    DECLARE_IMAGE_IMPL;

    return {
        static_cast<float>(impl->width()),
        static_cast<float>(impl->height()),
    };
}

ImageFormat Image::format() const
{
    DECLARE_IMAGE_IMPL;
    return impl->format();
}

uint32_t Image::mipmap_count() const
{
    DECLARE_IMAGE_IMPL;
    return impl->mipmap_count();
}

std::optional<Color> Image::canvas_clear_color() const
{
    DECLARE_IMAGE_IMPL;
    return impl->canvas_clear_color();
}

void Image::set_canvas_clear_color(std::optional<Color> value)
{
    DECLARE_IMAGE_IMPL;
    impl->set_canvas_clear_color(value);
}

uint32_t Image::size_in_bytes() const
{
    DECLARE_IMAGE_IMPL;
    return image_slice_pitch(impl->width(), impl->height(), impl->format());
}
} // namespace cer

uint32_t cer::image_format_bits_per_pixel(ImageFormat format)
{
    switch (format)
    {
        case ImageFormat::R8_UNorm: return 8;
        case ImageFormat::R8G8B8A8_UNorm:
        case ImageFormat::R8G8B8A8_Srgb: return 8 * 4;
        case ImageFormat::R32G32B32A32_Float: return 32 * 4;
    }

    return 0;
}

uint32_t cer::image_row_pitch(uint32_t width, ImageFormat format)
{
    return width * image_format_bits_per_pixel(format) / 8;
}

uint32_t cer::image_slice_pitch(uint32_t width, uint32_t height, ImageFormat format)
{
    return width * height * image_format_bits_per_pixel(format) / 8;
}

std::string_view cer::image_format_name(ImageFormat format)
{
    switch (format)
    {
        case ImageFormat::R8_UNorm: return "R8_UNorm";
        case ImageFormat::R8G8B8A8_UNorm: return "R8G8B8A8_UNorm";
        case ImageFormat::R8G8B8A8_Srgb: return "R8G8B8A8_Srgb";
        case ImageFormat::R32G32B32A32_Float: return "R32G32B32A32_Float";
    }

    return {};
}
