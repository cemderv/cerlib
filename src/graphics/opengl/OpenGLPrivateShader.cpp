// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "OpenGLPrivateShader.hpp"
#include "cerlib/Logging.hpp"
#include <cassert>
#include <cerlib/InternalError.hpp>
#include <memory>

namespace cer::details
{
OpenGLPrivateShader::OpenGLPrivateShader(std::string_view name,
                                         GLenum           type,
                                         std::string_view glsl_code)
    : name(name)
{
    log_verbose("Compiling OpenGL shader '{}'", name);

    if (type == GL_VERTEX_SHADER)
    {
        constexpr auto str = std::string_view{"vsin"};
        auto           idx = glsl_code.find("in ");

        while (idx != std::string::npos)
        {
            const auto semicolon_idx = glsl_code.find(';', idx + str.size());
            assert(semicolon_idx != std::string::npos);

            idx = glsl_code.find(str, idx);
            assert(idx != std::string::npos);

            auto var_name = glsl_code.substr(idx, semicolon_idx - idx);
            while (var_name.back() == ' ')
            {
                var_name.remove_suffix(1);
            }

            while (var_name.front() == ' ')
            {
                var_name.remove_prefix(1);
            }

            attributes.emplace_back(var_name);

            idx = glsl_code.find("in ", semicolon_idx);
        }
    }

    verify_opengl_state();

    gl_handle = GL_CALL(glCreateShader(type));

    if (gl_handle == 0)
    {
        CER_THROW_RUNTIME_ERROR_STR("Failed to create the OpenGL shader handle.");
    }

    auto code_strings = small_vector<std::string_view>{};

    // https://en.wikipedia.org/wiki/OpenGL_Shading_Language#Versions
#ifdef CERLIB_GFX_IS_GLES
    code_strings.emplace_back("#version 300 es\n\n");
#else
    code_strings.emplace_back("#version 140\n\n");
#endif

    if (type == GL_FRAGMENT_SHADER)
    {
        code_strings.emplace_back("precision highp float;\n");
        code_strings.emplace_back("precision highp sampler2D;\n");
    }

    code_strings.push_back(glsl_code);

    auto code_strings_gl        = small_vector<const GLchar*>{};
    auto code_string_lengths_gl = small_vector<GLint>{};

    code_strings_gl.reserve(code_strings.size());
    code_string_lengths_gl.reserve(code_strings.size());

    for (const auto& str : code_strings)
    {
        code_strings_gl.push_back(str.data());
        code_string_lengths_gl.emplace_back(GLint(str.size()));
    }

#ifdef CERLIB_ENABLE_VERBOSE_LOGGING
    {
        std::string total_code;

        for (const std::string_view str : code_strings)
        {
            total_code += str;
        }

        log_verbose("Sending GLSL code to driver:\n{}", total_code);
    }
#endif

    GL_CALL(glShaderSource(gl_handle,
                           GLsizei(code_strings.size()),
                           code_strings_gl.data(),
                           code_string_lengths_gl.data()));

    GL_CALL(glCompileShader(gl_handle));

    GLint compile_status = 0;
    GL_CALL(glGetShaderiv(gl_handle, GL_COMPILE_STATUS, &compile_status));

    if (compile_status != GL_TRUE)
    {
        const auto buffer = std::make_unique<GLchar[]>(shader_log_max_length);
        auto       length = shader_log_max_length;

        GL_CALL(glGetShaderInfoLog(gl_handle, shader_log_max_length, &length, buffer.get()));

        const auto msg =
            std::string_view{reinterpret_cast<const char*>(buffer.get()), size_t(length)};

        CER_THROW_RUNTIME_ERROR("Failed to compile the generated OpenGL shader: {}", msg);
    }

    verify_opengl_state();
}

OpenGLPrivateShader::OpenGLPrivateShader(OpenGLPrivateShader&& other) noexcept
    : name(std::move(other.name))
    , gl_handle(other.gl_handle)
    , attributes(std::move(other.attributes))
{
    other.gl_handle = 0;
}

auto OpenGLPrivateShader::operator=(OpenGLPrivateShader&& other) noexcept -> OpenGLPrivateShader&
{
    if (&other != this)
    {
        if (gl_handle != 0)
        {
            glDeleteShader(gl_handle);
        }

        name            = std::move(other.name);
        gl_handle       = other.gl_handle;
        attributes      = std::move(other.attributes);
        other.gl_handle = 0;
    }

    return *this;
}

OpenGLPrivateShader::~OpenGLPrivateShader() noexcept
{
    if (gl_handle != 0)
    {
        glDeleteShader(gl_handle);
        gl_handle = 0;
    }
}
} // namespace cer::details
