// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "OpenGLPrerequisites.hpp"
#include "graphics/VertexElement.hpp"
#include <cerlib/CopyMoveMacros.hpp>
#include <span>

namespace cer::details
{
class OpenGLVao final
{
  public:
    explicit OpenGLVao() = default;

    explicit OpenGLVao(GLuint vbo, GLuint ibo, std::span<const VertexElement> vertex_elements);

    forbid_copy(OpenGLVao);

    OpenGLVao(OpenGLVao&&) noexcept;

    auto operator=(OpenGLVao&& other) noexcept -> OpenGLVao&;

    ~OpenGLVao() noexcept;

    GLuint gl_handle{};
    GLuint vbo_handle{};
    GLuint ibo_handle{};

  private:
    void destroy();
};
} // namespace cer::details
