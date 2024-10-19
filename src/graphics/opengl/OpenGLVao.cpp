// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "OpenGLVao.hpp"
#include "util/InternalError.hpp"
#include "util/small_vector.hpp"
#include <cassert>
#include <gsl/narrow>

namespace cer::details
{
OpenGLVao::OpenGLVao(GLuint vbo, GLuint ibo, std::span<const VertexElement> vertex_elements)
    : vbo_handle(vbo)
    , ibo_handle(ibo)
{
    verify_opengl_state();

    glGenVertexArrays(1, &gl_handle);

    if (gl_handle == 0)
    {
        CER_THROW_RUNTIME_ERROR_STR("Failed to create the VAO handle.");
    }

    GL_CALL(glBindVertexArray(gl_handle));

    if (vbo != 0)
    {
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        verify_opengl_state();

        auto index                  = GLuint{};
        auto vertex_stride          = GLsizei{};
        auto element_sizes_in_bytes = gch::small_vector<GLsizei, 6>{};

        for (const auto& element : vertex_elements)
        {
            const auto maybe_size = [element]() -> std::optional<size_t> {
                switch (element)
                {
                    case VertexElement::Int: return sizeof(int32_t);
                    case VertexElement::UInt: return sizeof(uint32_t);
                    case VertexElement::Float: return sizeof(float);
                    case VertexElement::Vector2: return sizeof(float) * 2;
                    case VertexElement::Vector3: return sizeof(float) * 3;
                    case VertexElement::Vector4: return sizeof(float) * 4;
                }

                return std::nullopt;
            }();

            const auto size = maybe_size.value();

            element_sizes_in_bytes.push_back(GLsizei(size));
            vertex_stride += GLsizei(size);
            ++index;
        }

        index       = 0;
        auto offset = GLsizeiptr{};

        for (const auto element : vertex_elements)
        {
            GL_CALL(glEnableVertexAttribArray(index));

            const auto [size, type] = [element]() -> std::pair<GLsizei, GLenum> {
                switch (element)
                {
                    case VertexElement::Int: return {1, GL_INT};
                    case VertexElement::UInt: return {1, GL_UNSIGNED_INT};
                    case VertexElement::Float: return {1, GL_FLOAT};
                    case VertexElement::Vector2: return {2, GL_FLOAT};
                    case VertexElement::Vector3: return {3, GL_FLOAT};
                    case VertexElement::Vector4: return {4, GL_FLOAT};
                }
                return {0, 0};
            }();

            GL_CALL(glVertexAttribPointer(index,
                                          size,
                                          type,
                                          GL_FALSE,
                                          vertex_stride,
                                          reinterpret_cast<const void*>(offset)));

            offset += element_sizes_in_bytes[index];
            ++index;
        }

        assert(offset == vertex_stride);
    }

    if (ibo != 0)
    {
        GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo));
        verify_opengl_state();
    }

    GL_CALL(glBindVertexArray(0));
}

OpenGLVao::OpenGLVao(OpenGLVao&& other) noexcept
    : gl_handle(other.gl_handle)
    , vbo_handle(other.vbo_handle)
    , ibo_handle(other.ibo_handle)
{
    other.gl_handle = 0;
}

auto OpenGLVao::operator=(OpenGLVao&& other) noexcept -> OpenGLVao&
{
    if (&other != this)
    {
        destroy();
        gl_handle       = other.gl_handle;
        vbo_handle      = other.vbo_handle;
        ibo_handle      = other.ibo_handle;
        other.gl_handle = 0;
    }

    return *this;
}

OpenGLVao::~OpenGLVao() noexcept
{
    destroy();
}

void OpenGLVao::destroy()
{
    if (gl_handle != 0)
    {
#ifdef glDeleteVertexArrays
        glDeleteVertexArrays(1, &gl_handle);
#else
        glDeleteVertexArraysOES(1, &m_GLHandle);
#endif
        gl_handle = 0;
    }
}
} // namespace cer::details
