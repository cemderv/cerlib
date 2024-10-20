// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "cerlib/Image.hpp"

#include "GraphicsDevice.hpp"
#include "ImageImpl.hpp"
#include "cerlib/Window.hpp"
#include "contentmanagement/ImageLoading.hpp"
#include "game/GameImpl.hpp"
#include <cerlib/InternalError.hpp>
#include <cerlib/Util2.hpp>

// NOLINTBEGIN
#define DECLARE_IMAGE_IMPL                                                                         \
    const auto impl = static_cast<details::ImageImpl*>(this->impl());                              \
    VERIFY_IMPL_ACCESS
// NOLINTEND

namespace cer
{
CERLIB_IMPLEMENT_DERIVED_OBJECT(GraphicsResource, Image);

Image::Image(uint32_t width, uint32_t height, ImageFormat format, const void* data)
{
    if (data == nullptr)
    {
        CER_THROW_INVALID_ARG("No image data specified (width={}; height={}; format={}).",
                              width,
                              height,
                              image_format_name(format));
    }

    LOAD_DEVICE_IMPL;

    set_impl(*this, device_impl.create_image(width, height, format, data).release());
}

Image::Image(std::span<const std::byte> memory)
{
    LOAD_DEVICE_IMPL;
    set_impl(*this, details::load_image(device_impl, memory).release());
}

Image::Image(std::string_view filename)
{
    LOAD_DEVICE_IMPL;
    set_impl(*this, details::load_image(device_impl, filename).release());
}

Image::Image(uint32_t width, uint32_t height, ImageFormat format, const Window& window)
{
    if (!window)
    {
        CER_THROW_INVALID_ARG_STR("No window specified.");
    }

    LOAD_DEVICE_IMPL;
    set_impl(*this, device_impl.create_canvas(window, width, height, format).release());
}

auto Image::is_canvas() const -> bool
{
    DECLARE_IMAGE_IMPL;
    return impl->is_canvas();
}

auto Image::width() const -> uint32_t
{
    DECLARE_IMAGE_IMPL;
    return impl->width();
}

auto Image::height() const -> uint32_t
{
    DECLARE_IMAGE_IMPL;
    return impl->height();
}

auto Image::widthf() const -> float
{
    return float(width());
}

auto Image::heightf() const -> float
{
    return float(height());
}

auto Image::size() const -> Vector2
{
    DECLARE_IMAGE_IMPL;
    return {float(impl->width()), float(impl->height())};
}

auto Image::format() const -> ImageFormat
{
    DECLARE_IMAGE_IMPL;
    return impl->format();
}

auto Image::canvas_clear_color() const -> std::optional<Color>
{
    DECLARE_IMAGE_IMPL;
    return impl->canvas_clear_color();
}

void Image::set_canvas_clear_color(std::optional<Color> value)
{
    DECLARE_IMAGE_IMPL;
    impl->set_canvas_clear_color(value);
}

auto Image::size_in_bytes() const -> uint32_t
{
    DECLARE_IMAGE_IMPL;
    return image_slice_pitch(impl->width(), impl->height(), impl->format());
}
} // namespace cer

auto cer::image_format_bits_per_pixel(ImageFormat format) -> uint32_t
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

auto cer::image_row_pitch(uint32_t width, ImageFormat format) -> uint32_t
{
    return width * image_format_bits_per_pixel(format) / 8;
}

auto cer::image_slice_pitch(uint32_t width, uint32_t height, ImageFormat format) -> uint32_t
{
    return width * height * image_format_bits_per_pixel(format) / 8;
}

auto cer::image_format_name(ImageFormat format) -> std::string_view
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
