// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "OpenGLPrerequisites.hpp"
#include "OpenGLPrivateShader.hpp"
#include "graphics/ShaderParameter.hpp"
#include <cerlib/CopyMoveMacros.hpp>
#include <cerlib/List.hpp>
#include <cerlib/String.hpp>

namespace cer::details
{
class OpenGLShaderProgram final
{
  public:
    explicit OpenGLShaderProgram();

    explicit OpenGLShaderProgram(const OpenGLPrivateShader& vertex_shader,
                                 const OpenGLPrivateShader& fragment_shader);

    explicit OpenGLShaderProgram(const OpenGLPrivateShader&       vertex_shader,
                                 GLuint                           fragment_shader,
                                 std::string_view                 fragment_shader_name,
                                 bool                             is_user_shader,
                                 std::span<const ShaderParameter> parameters);

    forbid_copy(OpenGLShaderProgram);

    OpenGLShaderProgram(OpenGLShaderProgram&& other) noexcept;

    auto operator=(OpenGLShaderProgram&& other) noexcept -> OpenGLShaderProgram&;

    ~OpenGLShaderProgram() noexcept;

    auto uniform_location(std::string_view name) const -> GLint;

    auto operator==(const OpenGLShaderProgram& other) const -> bool
    {
        return gl_handle == other.gl_handle;
    }

    auto operator!=(const OpenGLShaderProgram& other) const -> bool
    {
        return gl_handle == other.gl_handle;
    }

    String                               name;
    GLuint                                    gl_handle;
    PairList<String, GLint> uniform_locations;

  private:
    void destroy();
};
} // namespace cer::details
