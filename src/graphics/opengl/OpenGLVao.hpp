// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "OpenGLPrerequisites.hpp"
#include "graphics/VertexElement.hpp"
#include "util/NonCopyable.hpp"
#include <span>

namespace cer::details
{
class OpenGLVao final
{
  public:
    explicit OpenGLVao() = default;

    explicit OpenGLVao(GLuint vbo, GLuint ibo, std::span<const VertexElement> vertex_elements);

    NON_COPYABLE(OpenGLVao);

    OpenGLVao(OpenGLVao&&) noexcept;

    OpenGLVao& operator=(OpenGLVao&& other) noexcept;

    ~OpenGLVao() noexcept;

    GLuint gl_handle{};
    GLuint vbo_handle{};
    GLuint ibo_handle{};

  private:
    void destroy();
};
} // namespace cer::details
