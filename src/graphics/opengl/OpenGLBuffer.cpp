// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "OpenGLBuffer.hpp"
#include "util/InternalError.hpp"

namespace cer::details
{
OpenGLBuffer::OpenGLBuffer(GLenum target, size_t size_in_bytes, GLenum usage, const void* data)
{
    destroy();

    verify_opengl_state();
    GL_CALL(glGenBuffers(1, &gl_handle));

    if (gl_handle == 0)
    {
        CER_THROW_RUNTIME_ERROR_STR("Failed to create the OpenGL buffer");
    }

    GL_CALL(glBindBuffer(target, gl_handle));
    GL_CALL(glBufferData(target, GLsizeiptr(size_in_bytes), data, usage));
}

OpenGLBuffer::OpenGLBuffer(OpenGLBuffer&& other) noexcept
    : gl_handle(other.gl_handle)
{
    other.gl_handle = 0;
}

auto OpenGLBuffer::operator=(OpenGLBuffer&& other) noexcept -> OpenGLBuffer&
{
    if (&other != this)
    {
        destroy();
        gl_handle       = other.gl_handle;
        other.gl_handle = 0;
    }

    return *this;
}

/**
 *
 */
OpenGLBuffer::~OpenGLBuffer() noexcept
{
    destroy();
}

void OpenGLBuffer::destroy()
{
    if (gl_handle != 0)
    {
        glDeleteBuffers(1, &gl_handle);
        gl_handle = 0;
    }
}
} // namespace cer::details