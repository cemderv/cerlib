// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "GraphicsResourceImpl.hpp"
#include "cerlib/Color.hpp"
#include "cerlib/Image.hpp"

#include <optional>

namespace cer
{
enum class ImageFormat;
}

namespace cer::details
{
class WindowImpl;

class ImageImpl : public GraphicsResourceImpl
{
  public:
    explicit ImageImpl(gsl::not_null<GraphicsDevice*> parent_device,
                       bool                           is_canvas,
                       WindowImpl*                    window_for_canvas,
                       uint32_t                       width,
                       uint32_t                       height,
                       ImageFormat                    format,
                       uint32_t                       mipmap_count);

    bool is_canvas() const;

    WindowImpl* window_for_canvas() const;

    uint32_t width() const;

    uint32_t height() const;

    ImageFormat format() const;

    uint32_t mipmap_count() const;

    std::optional<Color> canvas_clear_color() const;

    void set_canvas_clear_color(const std::optional<Color>& value);

  private:
    bool                 m_is_canvas{};
    WindowImpl*          m_window_for_canvas{};
    uint32_t             m_width{};
    uint32_t             m_height{};
    ImageFormat          m_format{};
    uint32_t             m_mipmap_count{};
    std::optional<Color> m_canvas_clear_color{};
};
} // namespace cer::details