// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "OpenGLShaderProgram.hpp"
#include "OpenGLSpriteBatch.hpp"
#include "cerlib/Logging.hpp"
#include "shadercompiler/Naming.hpp"
#include "util/InternalError.hpp"
#include <cassert>

namespace cer::details
{
OpenGLShaderProgram::OpenGLShaderProgram()
    : gl_handle()
{
}

OpenGLShaderProgram::OpenGLShaderProgram(const OpenGLPrivateShader& vertex_shader,
                                         const OpenGLPrivateShader& fragment_shader)
    : OpenGLShaderProgram(vertex_shader, fragment_shader.gl_handle, fragment_shader.name, false, {})
{
}

OpenGLShaderProgram::OpenGLShaderProgram(const OpenGLPrivateShader&       vertex_shader,
                                         GLuint                           fragment_shader,
                                         std::string_view                 fragment_shader_name,
                                         [[maybe_unused]] bool            is_user_shader,
                                         std::span<const ShaderParameter> parameters)
    : gl_handle(0)
{
    name = cer_fmt::format("VS({})_PS({})", vertex_shader.name, fragment_shader_name);

    log_verbose("Compiling OpenGL shader program '{}'", name);

    verify_opengl_state();

    gl_handle = GL_CALL(glCreateProgram()); // NOLINT(*-prefer-member-initializer)

    if (gl_handle == 0)
    {
        CER_THROW_RUNTIME_ERROR_STR("Failed to create the OpenGL shader program handle.");
    }

    GL_CALL(glAttachShader(gl_handle, vertex_shader.gl_handle));

    if (fragment_shader != 0u)
    {
        GL_CALL(glAttachShader(gl_handle, fragment_shader));
    }

    if (vertex_shader.gl_handle != 0)
    {
        GLuint index = 0;

        for (const auto& attrib : vertex_shader.attributes)
        {
            GL_CALL(glBindAttribLocation(gl_handle, index, attrib.c_str()));
            ++index;
        }
    }

    GL_CALL(glLinkProgram(gl_handle));

    GLint link_status = 0;
    GL_CALL(glGetProgramiv(gl_handle, GL_LINK_STATUS, &link_status));

    if (link_status != GL_TRUE)
    {
        constexpr auto max_length = 512;
        auto           buffer     = std::make_unique<GLchar[]>(max_length);
        GL_CALL(glGetProgramInfoLog(gl_handle, max_length, nullptr, buffer.get()));

        log_debug("Program linking error:\n{}", buffer.get());

        const auto msg = std::string{reinterpret_cast<const char*>(buffer.get())};

        CER_THROW_RUNTIME_ERROR_STR(msg);
    }

    if (vertex_shader.gl_handle != 0)
    {
        GL_CALL(glDetachShader(gl_handle, vertex_shader.gl_handle));
    }

    if (fragment_shader != 0u)
    {
        GL_CALL(glDetachShader(gl_handle, fragment_shader));
    }

    GLint uniform_count = 0;
    GL_CALL(glGetProgramiv(gl_handle, GL_ACTIVE_UNIFORMS, &uniform_count));

    if (uniform_count > 0)
    {
        GLuint previous_program = 0;
        GL_CALL(glGetIntegerv(GL_CURRENT_PROGRAM, reinterpret_cast<GLint*>(&previous_program)));

        GL_CALL(glUseProgram(gl_handle));

        uniform_locations.reserve(size_t(uniform_count));

        GLint max_name_length = 0;
        GL_CALL(glGetProgramiv(gl_handle, GL_ACTIVE_UNIFORM_MAX_LENGTH, &max_name_length));

        auto name_buffer = gch::small_vector<GLchar, 32>{};
        name_buffer.resize(max_name_length);

        for (GLint i = 0; i < uniform_count; ++i)
        {
            std::ranges::fill(name_buffer, '\0');

            GLsizei name_length = 0;
            GLint   size        = 0;
            GLenum  type        = 0;

            GL_CALL(glGetActiveUniform(gl_handle,
                                       GLuint(i),
                                       max_name_length,
                                       &name_length,
                                       &size,
                                       &type,
                                       name_buffer.data()));

            GLint location = 0;
            GL_CALL(location = glGetUniformLocation(gl_handle, name_buffer.data()));
            assert(location != -1);

            uniform_locations.emplace_back(std::string{name_buffer.data()}, location);
        }

        std::ranges::sort(uniform_locations, [](const auto& lhs, const auto& rhs) {
            return lhs.first < rhs.first;
        });

        auto image_counter = OpenGLSpriteBatch::texture_slot_base_offset;

        // Bind sprite batch image to slot 0 implicitly.
        if (const auto location =
                uniform_location(shadercompiler::naming::sprite_batch_image_param);
            location != -1)
        {
            glUniform1i(location, 0);
        }

        for (const auto& param : parameters)
        {
            if (param.is_image)
            {
                const auto location = uniform_location(param.name);
                assert(location != -1);
                glUniform1i(location, image_counter);
                ++image_counter;
            }
        }

        glUseProgram(previous_program);
    }

    verify_opengl_state();
}

OpenGLShaderProgram::OpenGLShaderProgram(OpenGLShaderProgram&& other) noexcept
    : name(std::move(other.name))
    , gl_handle(other.gl_handle)
    , uniform_locations(std::move(other.uniform_locations))
{
    other.gl_handle = 0;
}

auto OpenGLShaderProgram::operator=(OpenGLShaderProgram&& other) noexcept -> OpenGLShaderProgram&
{
    if (&other != this)
    {
        destroy();
        name              = std::move(other.name);
        gl_handle         = other.gl_handle;
        uniform_locations = std::move(other.uniform_locations);
        other.gl_handle   = 0;
    }

    return *this;
}

OpenGLShaderProgram::~OpenGLShaderProgram() noexcept
{
    destroy();
}

auto OpenGLShaderProgram::uniform_location(std::string_view name) const -> GLint
{
    const auto it = std::ranges::find_if(uniform_locations, [name](const auto& pair) {
        return pair.first == name;
    });

    return it != uniform_locations.cend() ? it->second : -1;
}

auto OpenGLShaderProgram::destroy() -> void
{
    if (gl_handle != 0)
    {
        glDeleteProgram(gl_handle);
        gl_handle = 0;
    }
}
} // namespace cer::details