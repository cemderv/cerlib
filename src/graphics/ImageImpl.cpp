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
                     ImageFormat                    format,
                     uint32_t                       mipmap_count)
    : GraphicsResourceImpl(parent_device, GraphicsResourceType::Image)
    , m_is_canvas(is_canvas)
    , m_window_for_canvas(window_for_canvas)
    , m_width(width)
    , m_height(height)
    , m_format(format)
    , m_mipmap_count(mipmap_count)
{
}

bool ImageImpl::is_canvas() const
{
    return m_is_canvas;
}

WindowImpl* ImageImpl::window_for_canvas() const
{
    return m_window_for_canvas;
}

uint32_t ImageImpl::width() const
{
    return m_width;
}

uint32_t ImageImpl::height() const
{
    return m_height;
}

ImageFormat ImageImpl::format() const
{
    return m_format;
}

uint32_t ImageImpl::mipmap_count() const
{
    return m_mipmap_count;
}

std::optional<Color> ImageImpl::canvas_clear_color() const
{
    return m_canvas_clear_color;
}

void ImageImpl::set_canvas_clear_color(const std::optional<Color>& value)
{
    m_canvas_clear_color = value;
}
} // namespace cer::details