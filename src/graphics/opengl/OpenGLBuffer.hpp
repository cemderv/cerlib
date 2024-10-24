// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "OpenGLPrerequisites.hpp"
#include <cerlib/CopyMoveMacros.hpp>
#include <cstddef>
#include <cstdint>

namespace cer::details
{
class OpenGLBuffer final
{
  public:
    explicit OpenGLBuffer() = default;

    explicit OpenGLBuffer(GLenum target, size_t size_in_bytes, GLenum usage, const void* data);

    forbid_copy(OpenGLBuffer);

    OpenGLBuffer(OpenGLBuffer&& other) noexcept;

    auto operator=(OpenGLBuffer&& other) noexcept -> OpenGLBuffer&;

    ~OpenGLBuffer() noexcept;

    GLuint gl_handle{};

  private:
    void destroy();
};
} // namespace cer::details
