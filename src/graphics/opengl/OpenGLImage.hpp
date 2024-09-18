// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "OpenGLPrerequisites.hpp"
#include "cerlib/Image.hpp"
#include "cerlib/Sampler.hpp"
#include "graphics/ImageImpl.hpp"

namespace cer::details
{
class OpenGLImage final : public ImageImpl
{
  public:
    explicit OpenGLImage(gsl::not_null<GraphicsDevice*> parent_device,
                         uint32_t                       width,
                         uint32_t                       height,
                         ImageFormat                    format,
                         uint32_t                       mipmap_count,
                         const Image::DataCallback&     data_callback);

    // Canvas overload
    explicit OpenGLImage(gsl::not_null<GraphicsDevice*> parent_device,
                         WindowImpl*                    window_for_canvas,
                         uint32_t                       width,
                         uint32_t                       height,
                         ImageFormat                    format);

    NON_COPYABLE_NON_MOVABLE(OpenGLImage);

    ~OpenGLImage() noexcept override;

    GLuint              gl_handle{};
    GLuint              gl_framebuffer_handle{};
    OpenGLFormatTriplet gl_format_triplet{};
    Sampler             last_applied_sampler{};
};
} // namespace cer::details
