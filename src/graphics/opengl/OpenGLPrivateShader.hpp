// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "OpenGLPrerequisites.hpp"
#include "util/NonCopyable.hpp"
#include "util/SmallVector.hpp"
#include <span>
#include <string>
#include <string_view>

namespace cer::details
{
class OpenGLPrivateShader final
{
  public:
    explicit OpenGLPrivateShader() = default;

    explicit OpenGLPrivateShader(std::string_view name, GLenum type, std::string_view glsl_code);

    NON_COPYABLE(OpenGLPrivateShader);

    OpenGLPrivateShader(OpenGLPrivateShader&& other) noexcept;

    OpenGLPrivateShader& operator=(OpenGLPrivateShader&& other) noexcept;

    ~OpenGLPrivateShader() noexcept;

    std::string              name;
    GLuint                   gl_handle{};
    SmallVector<std::string> attributes;
};
} // namespace cer::details
