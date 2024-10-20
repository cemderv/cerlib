// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "OpenGLPrerequisites.hpp"
#include <cerlib/CopyMoveMacros.hpp>
#include <cerlib/List.hpp>
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

    forbid_copy(OpenGLPrivateShader);

    OpenGLPrivateShader(OpenGLPrivateShader&& other) noexcept;

    auto operator=(OpenGLPrivateShader&& other) noexcept -> OpenGLPrivateShader&;

    ~OpenGLPrivateShader() noexcept;

    std::string               name;
    GLuint                    gl_handle{};
    small_vector<std::string> attributes;
};
} // namespace cer::details
