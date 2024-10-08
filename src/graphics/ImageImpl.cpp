// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "ImageImpl.hpp"

#include "cerlib/GraphicsResource.hpp"

namespace cer::details
{
ImageImpl::ImageImpl(gsl::not_null<GraphicsDevice*> parent_device,
                     bool                           is_canvas,
                     WindowImpl*                    window_for_canvas,
                     uint32_t                       width,
                     uint32_t                       height,
                     ImageFormat                    format)
    : GraphicsResourceImpl(parent_device, GraphicsResourceType::Image)
    , m_is_canvas(is_canvas)
    , m_window_for_canvas(window_for_canvas)
    , m_width(width)
    , m_height(height)
    , m_format(format)
{
}

auto ImageImpl::is_canvas() const -> bool
{
    return m_is_canvas;
}

auto ImageImpl::window_for_canvas() const -> WindowImpl*
{
    return m_window_for_canvas;
}

auto ImageImpl::width() const -> uint32_t
{
    return m_width;
}

auto ImageImpl::height() const -> uint32_t
{
    return m_height;
}

auto ImageImpl::format() const -> ImageFormat
{
    return m_format;
}

auto ImageImpl::canvas_clear_color() const -> std::optional<Color>
{
    return m_canvas_clear_color;
}

void ImageImpl::set_canvas_clear_color(const std::optional<Color>& value)
{
    m_canvas_clear_color = value;
}
} // namespace cer::details