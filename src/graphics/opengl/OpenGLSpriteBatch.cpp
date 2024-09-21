// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "OpenGLSpriteBatch.hpp"
#include "OpenGLGraphicsDevice.hpp"
#include "OpenGLImage.hpp"
#include "cerlib/Logging.hpp"

#include <cassert>

#include "SpriteBatchPSDefault.frag.hpp"
#include "SpriteBatchPSMonochromatic.frag.hpp"
#include "SpriteBatchVS.vert.hpp"

namespace cer::details
{
OpenGLSpriteBatch::OpenGLSpriteBatch(gsl::not_null<GraphicsDevice*> device_impl,
                                     gsl::not_null<FrameStats*>     draw_stats)
    : SpriteBatch(device_impl, draw_stats)
{
    log_verbose("Initializing OpenGLSpriteBatch, but verifying OpenGL state first");
    verify_opengl_state_x();
    log_verbose("  - State is clean");

    log_verbose("Creating OpenGLSpriteBatch shaders");

    m_sprite_vertex_shader = OpenGLPrivateShader("SpriteBatchVSMain",
                                                 GL_VERTEX_SHADER,
                                                 SpriteBatchVS_vert_string_view());

    OpenGLPrivateShader ps_default{"SpriteBatchPSDefault",
                                   GL_FRAGMENT_SHADER,
                                   SpriteBatchPSDefault_frag_string_view()};

    OpenGLPrivateShader ps_monochromatic{"SpriteBatchPSMonochromatic",
                                         GL_FRAGMENT_SHADER,
                                         SpriteBatchPSMonochromatic_frag_string_view()};

    m_default_sprite_shader_program = OpenGLShaderProgram(m_sprite_vertex_shader, ps_default);
    m_default_sprite_shader_program_u_transformation =
        GL_CALL(glGetUniformLocation(m_default_sprite_shader_program.gl_handle, "Transformation"));

    m_monochromatic_shader_program = OpenGLShaderProgram(m_sprite_vertex_shader, ps_monochromatic);
    m_monochromatic_shader_program_u_transformation =
        GL_CALL(glGetUniformLocation(m_monochromatic_shader_program.gl_handle, "Transformation"));

    log_verbose("  - Success");

    // Vertex buffer
    {
        constexpr uint32_t how_many_vertices = max_batch_size * vertices_per_sprite;

        m_vbo = OpenGLBuffer(GL_ARRAY_BUFFER,
                             sizeof(Vertex) * how_many_vertices,
                             GL_DYNAMIC_DRAW,
                             nullptr);
    }

    // Index buffer
    {
        constexpr uint32_t how_many_indices = max_batch_size * indices_per_sprite;

        std::vector<uint16_t> indices;
        indices.reserve(how_many_indices);

        for (uint32_t j = 0; j < max_batch_size * vertices_per_sprite; j += vertices_per_sprite)
        {
            const uint16_t i = static_cast<uint16_t>(j);

            indices.push_back(i);
            indices.push_back(i + 1);
            indices.push_back(i + 2);

            indices.push_back(i + 1);
            indices.push_back(i + 3);
            indices.push_back(i + 2);
        }

        assert(indices.size() == how_many_indices);

        m_ibo = OpenGLBuffer(GL_ELEMENT_ARRAY_BUFFER,
                             sizeof(uint16_t) * indices.size(),
                             GL_STATIC_DRAW,
                             indices.data());
    }

    // VAO
    m_vao = OpenGLVao(m_vbo.gl_handle,
                      m_ibo.gl_handle,
                      {{
                          VertexElement::Vector4,
                          VertexElement::Vector4,
                          VertexElement::Vector2,
                      }});
}

OpenGLSpriteBatch::~OpenGLSpriteBatch() noexcept = default;

void OpenGLSpriteBatch::prepare_for_rendering()
{
    OpenGLGraphicsDevice* opengl_device = static_cast<OpenGLGraphicsDevice*>(parent_device().get());

    set_default_render_state();
    apply_blend_state_to_gl_context(current_blend_state());

    opengl_device->bind_vao(m_vao);

    const OpenGLUserShader* sprite_shader =
        static_cast<const OpenGLUserShader*>(this->sprite_shader().impl());

    if (sprite_shader != nullptr)
    {
        auto it = m_custom_shader_programs.find(sprite_shader);
        if (it == m_custom_shader_programs.cend())
        {
            OpenGLShaderProgram program{m_sprite_vertex_shader,
                                        sprite_shader->gl_handle,
                                        sprite_shader->name(),
                                        true,
                                        sprite_shader->all_parameters()};

            it = m_custom_shader_programs.emplace(sprite_shader, std::move(program)).first;
        }

        m_current_custom_shader_program = &it->second;
    }
    else
    {
        m_current_custom_shader_program = nullptr;
    }
}

void OpenGLSpriteBatch::set_up_batch(const Image&              image,
                                     SpriteShaderKind          shader_kind,
                                     [[maybe_unused]] uint32_t start,
                                     [[maybe_unused]] uint32_t count)
{
    OpenGLGraphicsDevice* opengl_device = static_cast<OpenGLGraphicsDevice*>(parent_device().get());

    OpenGLShaderProgram* shader_program{};
    GLint                u_transformation = -1;

    switch (shader_kind)
    {
        case SpriteShaderKind::Default: {
            if (m_current_custom_shader_program != nullptr)
            {
                shader_program = m_current_custom_shader_program;
                u_transformation =
                    m_current_custom_shader_program->uniform_location("Transformation");
            }
            else
            {
                shader_program   = &m_default_sprite_shader_program;
                u_transformation = m_default_sprite_shader_program_u_transformation;
            }
            break;
        }
        case SpriteShaderKind::Monochromatic: {
            shader_program   = &m_monochromatic_shader_program;
            u_transformation = m_monochromatic_shader_program_u_transformation;
            break;
        }
    }

    assert(shader_program != nullptr);

    opengl_device->use_program(shader_program->gl_handle);

    if (shader_program == m_current_custom_shader_program)
    {
        // Apply custom shader uniforms
        ShaderImpl*    shader_impl  = static_cast<ShaderImpl*>(sprite_shader().impl());
        const uint8_t* cbuffer_data = shader_impl->cbuffer_data();

        for (const ShaderParameter* param : shader_impl->dirty_scalar_parameters())
        {
            const GLint location = shader_program->uniform_location(param->name);
            if (location == -1)
            {
                continue;
            }

            const uint8_t* const param_data          = cbuffer_data + param->offset; // NOLINT
            const GLfloat* const param_data_as_float = reinterpret_cast<const GLfloat*>(param_data);
            const GLint* const   param_data_as_int   = reinterpret_cast<const GLint*>(param_data);

            switch (param->type)
            {
                case ShaderParameterType::Float: {
                    glUniform1f(location, *param_data_as_float);
                    break;
                }
                case ShaderParameterType::Int: {
                    glUniform1i(location, *param_data_as_int);
                    break;
                }
                case ShaderParameterType::Bool: {
                    glUniform1i(location, *param_data_as_int);
                    break;
                }
                case ShaderParameterType::Vector2: {
                    glUniform2fv(location, 1, param_data_as_float);
                    break;
                }
                case ShaderParameterType::Vector3: {
                    glUniform3fv(location, 1, param_data_as_float);
                    break;
                }
                case ShaderParameterType::Vector4: {
                    glUniform4fv(location, 1, param_data_as_float);
                    break;
                }
                case ShaderParameterType::Matrix: {
                    glUniformMatrix4fv(location, 1, GL_FALSE, param_data_as_float);
                    break;
                }
                case ShaderParameterType::Image: {
                    assert(false && "should not happen");
                    break;
                }
                case ShaderParameterType::FloatArray: {
                    glUniform1fv(location,
                                 static_cast<GLsizei>(param->array_size),
                                 param_data_as_float);
                    break;
                }
                case ShaderParameterType::IntArray: {
                    glUniform1iv(location,
                                 static_cast<GLsizei>(param->array_size),
                                 param_data_as_int);
                    break;
                }
                case ShaderParameterType::BoolArray: {
                    glUniform1iv(location,
                                 static_cast<GLsizei>(param->array_size),
                                 param_data_as_int);
                    break;
                }
                case ShaderParameterType::Vector2Array: {
                    glUniform2fv(location,
                                 static_cast<GLsizei>(param->array_size),
                                 param_data_as_float);
                    break;
                }
                case ShaderParameterType::Vector3Array: {
                    glUniform3fv(location,
                                 static_cast<GLsizei>(param->array_size),
                                 param_data_as_float);
                    break;
                }
                case ShaderParameterType::Vector4Array: {
                    glUniform4fv(location,
                                 static_cast<GLsizei>(param->array_size),
                                 param_data_as_float);
                    break;
                }
                case ShaderParameterType::MatrixArray: {
                    glUniformMatrix4fv(location,
                                       static_cast<GLsizei>(param->array_size),
                                       GL_FALSE,
                                       param_data_as_float);
                    break;
                }
            }
        }

        shader_impl->clear_dirty_scalar_parameters();

        for (const auto& param : shader_impl->dirty_image_parameters())
        {
            // We don't have to update the shader program's uniforms, because they were
            // already set during construction.
            // Instead, we have to figure out which texture slots those parameters correspond
            // to and bind the parameter's images to those slots.
            OpenGLImage* opengl_image = static_cast<OpenGLImage*>(param->image.impl());

            glActiveTexture(GL_TEXTURE0 + texture_slot_base_offset + param->offset);
            glBindTexture(GL_TEXTURE_2D, opengl_image != nullptr ? opengl_image->gl_handle : 0);

            if (opengl_image != nullptr)
            {
                // TODO: make the sampler a parameter-based setting
                const Sampler sampler = Sampler::linear_repeat();
                apply_sampler_to_gl_context(sampler);
                opengl_image->last_applied_sampler = sampler;
            }
        }

        shader_impl->clear_dirty_image_parameters();
    }

    const Matrix transformation = current_transformation();
    GL_CALL(glUniformMatrix4fv(u_transformation, 1, GL_FALSE, transformation.data()));

    OpenGLImage* opengl_image = static_cast<OpenGLImage*>(image.impl());

    GL_CALL(glActiveTexture(GL_TEXTURE0));
    GL_CALL(glBindTexture(GL_TEXTURE_2D, opengl_image->gl_handle));

    const Sampler sampler = current_sampler();

    if (*shader_program == m_monochromatic_shader_program)
    {
        // We're drawing text. Use nearest neighbor interpolation.
        apply_sampler_to_gl_context(Sampler::point_clamp());
    }
    else if (opengl_image->last_applied_sampler != sampler)
    {
        // We're drawing sprites.
        apply_sampler_to_gl_context(sampler);
        opengl_image->last_applied_sampler = sampler;
    }
}

void OpenGLSpriteBatch::fill_vertices_and_draw(uint32_t         batch_start,
                                               uint32_t         batch_size,
                                               const Rectangle& texture_size_and_inverse,
                                               bool             flip_image_up_down)
{
    constexpr bool use_buffer_sub_data = true;

    if (use_buffer_sub_data && !m_vertex_data)
    {
        m_vertex_data = std::make_unique<Vertex[]>(static_cast<size_t>(max_batch_size) *
                                                   static_cast<size_t>(vertices_per_sprite));
    }

    const uint32_t start_vertex = batch_start * vertices_per_sprite;
    const uint32_t vertex_count = batch_size * vertices_per_sprite;

    Vertex* vertices = nullptr;

    if (use_buffer_sub_data)
    {
        vertices = m_vertex_data.get() + start_vertex;
    }
    else
    {
        GLbitfield map_flags{GL_MAP_WRITE_BIT};
        if (batch_start == 0)
        {
            map_flags |= GL_MAP_INVALIDATE_BUFFER_BIT;
        }

        map_flags |= GL_MAP_UNSYNCHRONIZED_BIT;

        vertices = static_cast<Vertex*>(glMapBufferRange(
            GL_ARRAY_BUFFER,
            static_cast<GLintptr>(static_cast<size_t>(start_vertex) * sizeof(Vertex)),
            static_cast<GLintptr>(static_cast<size_t>(vertex_count) * sizeof(Vertex)),
            map_flags));
    }

    fill_sprite_vertices(vertices,
                         batch_start,
                         batch_size,
                         texture_size_and_inverse,
                         flip_image_up_down);

    if (use_buffer_sub_data)
    {
        GL_CALL(glBufferSubData(GL_ARRAY_BUFFER,
                                GLintptr(start_vertex * sizeof(Vertex)),
                                GLsizeiptr(vertex_count * sizeof(Vertex)),
                                vertices));
    }
    else
    {
        GL_CALL(glUnmapBuffer(GL_ARRAY_BUFFER));
    }

    const uint32_t start_index = batch_start * indices_per_sprite;
    const uint32_t index_count = batch_size * indices_per_sprite;

    GL_CALL(glDrawElements(GL_TRIANGLES,
                           static_cast<GLsizei>(index_count),
                           GL_UNSIGNED_SHORT,
                           reinterpret_cast<const void*>(start_index * sizeof(uint16_t))));

    ++frame_stats().draw_calls;
}

void OpenGLSpriteBatch::on_end_rendering()
{
    verify_opengl_state();
}

void OpenGLSpriteBatch::set_default_render_state()
{
    GL_CALL(glDisable(GL_DEPTH_TEST));
    GL_CALL(glDepthMask(GL_FALSE));
    GL_CALL(glEnable(GL_CULL_FACE));
    GL_CALL(glCullFace(GL_FRONT));
}

static GLenum convert(ImageAddressMode mode)
{
    switch (mode)
    {
        case ImageAddressMode::Repeat: return GL_REPEAT;
        case ImageAddressMode::ClampToEdgeTexels: return GL_CLAMP_TO_EDGE;
#ifdef CERLIB_GFX_IS_GLES
        case ImageAddressMode::ClampToSamplerBorderColor:
            CER_THROW_INVALID_ARG_STR("ClampToSamplerBorderColor address mode is not "
                                      "supported on the current system.");
#else
        case ImageAddressMode::ClampToSamplerBorderColor: return GL_CLAMP_TO_BORDER;
#endif
        case ImageAddressMode::Mirror: return GL_MIRRORED_REPEAT;
    }

    return GL_CLAMP_TO_EDGE;
}

void OpenGLSpriteBatch::apply_sampler_to_gl_context(const Sampler& sampler)
{
    switch (sampler.filter)
    {
        case ImageFilter::Point:
            GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
            GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
            break;
        case ImageFilter::Linear:
            GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
            GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
            break;
    }

    GL_CALL(glTexParameteri(GL_TEXTURE_2D,
                            GL_TEXTURE_WRAP_S,
                            static_cast<GLint>(convert(sampler.address_u))));

    GL_CALL(glTexParameteri(GL_TEXTURE_2D,
                            GL_TEXTURE_WRAP_T,
                            static_cast<GLint>(convert(sampler.address_v))));

    // TODO: sampler.textureComparison
    // TODO: sampler.maxAnisotropy
    // TODO: sampler.borderColor
}

static GLenum convert(BlendFunction function)
{
    switch (function)
    {
        case BlendFunction::Add: return GL_FUNC_ADD;
        case BlendFunction::Subtract: return GL_FUNC_SUBTRACT;
        case BlendFunction::ReverseSubtract: return GL_FUNC_REVERSE_SUBTRACT;
        case BlendFunction::Min: return GL_MIN;
        case BlendFunction::Max: return GL_MAX;
    }

    return GL_FUNC_ADD;
}

static GLenum convert(Blend blend)
{
    switch (blend)
    {
        case Blend::One: return GL_ONE;
        case Blend::Zero: return GL_ZERO;
        case Blend::SourceColor: return GL_SRC_COLOR;
        case Blend::InverseSourceColor: return GL_ONE_MINUS_SRC_COLOR;
        case Blend::SourceAlpha: return GL_SRC_ALPHA;
        case Blend::InverseSourceAlpha: return GL_ONE_MINUS_SRC_ALPHA;
        case Blend::DestColor: return GL_DST_COLOR;
        case Blend::InverseDestColor: return GL_ONE_MINUS_DST_COLOR;
        case Blend::DestAlpha: return GL_DST_ALPHA;
        case Blend::InverseDestAlpha: return GL_ONE_MINUS_DST_ALPHA;
        case Blend::BlendFactor: return GL_CONSTANT_COLOR;
        case Blend::InverseBlendFactor: return GL_ONE_MINUS_CONSTANT_COLOR;
        case Blend::SourceAlphaSaturation: return GL_SRC_ALPHA_SATURATE;
    }

    return GL_ONE;
}

void OpenGLSpriteBatch::apply_blend_state_to_gl_context(const BlendState& blend_state)
{
    if (m_last_applied_blend_state.has_value() && blend_state == m_last_applied_blend_state)
    {
        return;
    }

    if (blend_state.blending_enabled)
    {
        GL_CALL(glEnable(GL_BLEND));
    }
    else
    {
        GL_CALL(glDisable(GL_BLEND));
    }

    GL_CALL(glBlendColor(blend_state.blend_factor.r,
                         blend_state.blend_factor.g,
                         blend_state.blend_factor.b,
                         blend_state.blend_factor.a));

    GL_CALL(glBlendFuncSeparate(convert(blend_state.color_src_blend),
                                convert(blend_state.color_dst_blend),
                                convert(blend_state.alpha_src_blend),
                                convert(blend_state.alpha_dst_blend)));

    GL_CALL(glBlendEquationSeparate(convert(blend_state.color_blend_function),
                                    convert(blend_state.alpha_blend_function)));

    const ColorWriteMask color_mask = blend_state.color_write_mask;

    const auto gl_channel_mask = [color_mask](ColorWriteMask m) -> GLboolean {
        return (static_cast<int>(color_mask) & static_cast<int>(m)) == static_cast<int>(m)
                   ? GL_TRUE
                   : GL_FALSE;
    };

    GL_CALL(glColorMask(gl_channel_mask(ColorWriteMask::Red),
                        gl_channel_mask(ColorWriteMask::Green),
                        gl_channel_mask(ColorWriteMask::Blue),
                        gl_channel_mask(ColorWriteMask::Alpha)));

    m_last_applied_blend_state = blend_state;
}

auto OpenGLSpriteBatch::on_shader_destroyed(gsl::not_null<ShaderImpl*> shader) -> void
{
    const OpenGLUserShader* opengl_shader = static_cast<const OpenGLUserShader*>(shader.get());

    if (const auto it = m_custom_shader_programs.find(opengl_shader);
        it != m_custom_shader_programs.cend())
    {
        log_verbose("Erasing OpenGLShaderProgram with user-origin '{}'", shader->name());
        m_custom_shader_programs.erase(it);
    }

    SpriteBatch::on_shader_destroyed(shader);
}
} // namespace cer::details