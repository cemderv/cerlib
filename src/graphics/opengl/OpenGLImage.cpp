// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "OpenGLImage.hpp"

namespace cer::details
{
OpenGLImage::OpenGLImage(GraphicsDevice& parent_device,
                         uint32_t        width,
                         uint32_t        height,
                         ImageFormat     format,
                         const void*     data)
    : ImageImpl(parent_device, false, nullptr, width, height, format)
{
    const auto format_gl = convert_to_opengl_pixel_format(format);

    verify_opengl_state();
    GL_CALL(glGenTextures(1, &gl_handle));

    if (gl_handle == 0)
    {
        throw std::runtime_error{"Failed to create the texture handle."};
    }

    verify_opengl_state();

    GL_CALL(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));

    GLuint previous_texture = 0;
    GL_CALL(glGetIntegerv(GL_TEXTURE_BINDING_2D, reinterpret_cast<GLint*>(&previous_texture)));

    GL_CALL(glBindTexture(GL_TEXTURE_2D, gl_handle));

    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

#ifndef CERLIB_GFX_IS_GLES
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0));
#endif

    last_applied_sampler = linear_clamp;

    GL_CALL(glTexImage2D(GL_TEXTURE_2D,
                         /*level: */ 0,
                         format_gl.internal_format,
                         GLsizei(width),
                         GLsizei(height),
                         /*border: */ 0,
                         format_gl.base_format,
                         format_gl.type,
                         data));

    GL_CALL(glBindTexture(GL_TEXTURE_2D, previous_texture));
    verify_opengl_state();
}

OpenGLImage::OpenGLImage(GraphicsDevice& parent_device,
                         WindowImpl*     window_for_canvas,
                         uint32_t        width,
                         uint32_t        height,
                         ImageFormat     format)
    : ImageImpl(parent_device, true, window_for_canvas, width, height, format)
{
    gl_format_triplet = convert_to_opengl_pixel_format(format);

    verify_opengl_state();

    GLuint previous_texture = 0;
    GL_CALL(glGetIntegerv(GL_TEXTURE_BINDING_2D, reinterpret_cast<GLint*>(&previous_texture)));

    defer
    {
        glBindTexture(GL_TEXTURE_2D, previous_texture);
    };

    GL_CALL(glGenTextures(1, &gl_handle));

    if (gl_handle == 0)
    {
        throw std::runtime_error{"Failed to create the canvas texture handle."};
    }

    GL_CALL(glBindTexture(GL_TEXTURE_2D, gl_handle));

    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

#ifndef CERLIB_GFX_IS_GLES
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0));
#endif

    GL_CALL(glTexImage2D(GL_TEXTURE_2D,
                         0,
                         gl_format_triplet.internal_format,
                         GLsizei(width),
                         GLsizei(height),
                         0,
                         gl_format_triplet.base_format,
                         gl_format_triplet.type,
                         nullptr));

    verify_opengl_state();

    GL_CALL(glGenFramebuffers(1, &gl_framebuffer_handle));

    if (gl_framebuffer_handle == 0)
    {
        throw std::runtime_error{"Failed to create the canvas handle."};
    }

    verify_opengl_state();

    GLuint previous_fbo = 0;
    GL_CALL(glGetIntegerv(GL_FRAMEBUFFER_BINDING, reinterpret_cast<GLint*>(&previous_fbo)));

    defer
    {
        glBindFramebuffer(GL_FRAMEBUFFER, previous_fbo);
    };

    GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, gl_framebuffer_handle));
    GL_CALL(
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gl_handle, 0));

    const auto fbo_status = GL_CALL(glCheckFramebufferStatus(GL_FRAMEBUFFER));

    if (fbo_status != GL_FRAMEBUFFER_COMPLETE)
    {
        throw std::runtime_error{"Failed to create the internal canvas object."};
    }

    verify_opengl_state();
}

OpenGLImage::~OpenGLImage() noexcept
{
    if (gl_framebuffer_handle != 0)
    {
        glDeleteFramebuffers(1, &gl_framebuffer_handle);
        gl_framebuffer_handle = 0;
    }

    if (gl_handle != 0)
    {
        glDeleteTextures(1, &gl_handle);
        gl_handle = 0;
    }
}
} // namespace cer::details
