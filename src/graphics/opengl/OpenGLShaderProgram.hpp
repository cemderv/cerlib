// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "OpenGLPrerequisites.hpp"
#include "OpenGLPrivateShader.hpp"
#include "graphics/ShaderParameter.hpp"
#include "util/NonCopyable.hpp"
#include "util/SmallVector.hpp"

#include <string>

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

    NON_COPYABLE(OpenGLShaderProgram);

    OpenGLShaderProgram(OpenGLShaderProgram&& other) noexcept;

    OpenGLShaderProgram& operator=(OpenGLShaderProgram&& other) noexcept;

    ~OpenGLShaderProgram() noexcept;

    GLint uniform_location(std::string_view name) const;

    bool operator==(const OpenGLShaderProgram& other) const
    {
        return gl_handle == other.gl_handle;
    }

    bool operator!=(const OpenGLShaderProgram& other) const
    {
        return gl_handle == other.gl_handle;
    }

    std::string                                name;
    GLuint                                     gl_handle;
    SmallVector<std::pair<std::string, GLint>> uniform_locations;

  private:
    void destroy();
};
} // namespace cer::details
