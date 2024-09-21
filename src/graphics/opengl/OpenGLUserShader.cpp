// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "OpenGLUserShader.hpp"

#include <array>
#include <gsl/narrow>

namespace cer::details
{
OpenGLUserShader::OpenGLUserShader(gsl::not_null<GraphicsDevice*> parent_device,
                                   std::string_view               glsl_code,
                                   ParameterList                  parameters)
    : ShaderImpl(parent_device, std::move(parameters))
{
    gl_handle = GL_CALL(glCreateShader(GL_FRAGMENT_SHADER)); // NOLINT(*-prefer-member-initializer)

    if (gl_handle == 0)
    {
        CER_THROW_RUNTIME_ERROR_STR("Failed to create the internal shader handle.");
    }

    const std::array codes{
        (glsl_code.data()),
    };

    const std::array code_lengths{
        gsl::narrow<GLint>(glsl_code.size()),
    };

    GL_CALL(glShaderSource(gl_handle, 1, codes.data(), code_lengths.data()));
    GL_CALL(glCompileShader(gl_handle));

    GLint compile_status = 0;
    GL_CALL(glGetShaderiv(gl_handle, GL_COMPILE_STATUS, &compile_status));

    if (compile_status != GL_TRUE)
    {
        auto    buffer = std::make_unique<GLchar[]>(shader_log_max_length);
        GLsizei length = shader_log_max_length;

        GL_CALL(glGetShaderInfoLog(gl_handle, shader_log_max_length, &length, buffer.get()));

        const std::string_view msg{buffer.get(), static_cast<size_t>(length)};

        CER_THROW_RUNTIME_ERROR("Failed to compile the generated internal shader: {}", msg);
    }
}
} // namespace cer::details