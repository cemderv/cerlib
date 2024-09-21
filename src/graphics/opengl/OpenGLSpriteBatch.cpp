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

    auto psDefault = OpenGLPrivateShader("SpriteBatchPSDefault",
                                         GL_FRAGMENT_SHADER,
                                         SpriteBatchPSDefault_frag_string_view());

    auto psMonochromatic = OpenGLPrivateShader("SpriteBatchPSMonochromatic",
                                               GL_FRAGMENT_SHADER,
                                               SpriteBatchPSMonochromatic_frag_string_view());

    m_default_sprite_shader_program = OpenGLShaderProgram(m_sprite_vertex_shader, psDefault);
    m_default_sprite_shader_program_u_transformation =
        GL_CALL(glGetUniformLocation(m_default_sprite_shader_program.gl_handle, "Transformation"));

    m_monochromatic_shader_program = OpenGLShaderProgram(m_sprite_vertex_shader, psMonochromatic);
    m_monochromatic_shader_program_u_transformation =
        GL_CALL(glGetUniformLocation(m_monochromatic_shader_program.gl_handle, "Transformation"));

    log_verbose("  - Success");

    // Vertex buffer
    {
        constexpr auto howManyVertices = max_batch_size * vertices_per_sprite;
        m_vbo                          = OpenGLBuffer(GL_ARRAY_BUFFER,
                             sizeof(Vertex) * howManyVertices,
                             GL_DYNAMIC_DRAW,
                             nullptr);
    }

    // Index buffer
    {
        constexpr auto howManyIndices = max_batch_size * indices_per_sprite;

        std::vector<uint16_t> indices;
        indices.reserve(howManyIndices);

        for (size_t j = 0; j < max_batch_size * vertices_per_sprite; j += vertices_per_sprite)
        {
            const auto i = uint16_t(j);

            indices.push_back(i);
            indices.push_back(i + 1);
            indices.push_back(i + 2);

            indices.push_back(i + 1);
            indices.push_back(i + 3);
            indices.push_back(i + 2);
        }

        assert(indices.size() == howManyIndices);

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
    const auto openGLDevice = static_cast<OpenGLGraphicsDevice*>(parent_device().get());

    set_default_render_state();
    apply_blend_state_to_gl_context(current_blend_state());

    openGLDevice->bind_vao(m_vao);

    const auto spriteShader = static_cast<const OpenGLUserShader*>(sprite_shader().impl());
    if (spriteShader)
    {
        auto it = m_custom_shader_programs.find(spriteShader);
        if (it == m_custom_shader_programs.cend())
        {
            auto program = OpenGLShaderProgram(m_sprite_vertex_shader,
                                               spriteShader->gl_handle,
                                               spriteShader->name(),
                                               true,
                                               spriteShader->all_parameters());

            it = m_custom_shader_programs.emplace(spriteShader, std::move(program)).first;
        }

        m_current_custom_shader_program = &it->second;
    }
    else
    {
        m_current_custom_shader_program = nullptr;
    }
}

void OpenGLSpriteBatch::set_up_batch(const Image&     image,
                                     SpriteShaderKind shader_kind,
                                     uint32_t         start,
                                     uint32_t         count)
{
    const auto openGLDevice = static_cast<OpenGLGraphicsDevice*>(parent_device().get());

    auto shaderProgram   = static_cast<OpenGLShaderProgram*>(nullptr);
    auto uTransformation = GLint(-1);

    switch (shader_kind)
    {
        case SpriteShaderKind::Default: {
            if (m_current_custom_shader_program)
            {
                shaderProgram = m_current_custom_shader_program;
                uTransformation =
                    m_current_custom_shader_program->uniform_location("Transformation");
            }
            else
            {
                shaderProgram   = &m_default_sprite_shader_program;
                uTransformation = m_default_sprite_shader_program_u_transformation;
            }
            break;
        }
        case SpriteShaderKind::Monochromatic: {
            shaderProgram   = &m_monochromatic_shader_program;
            uTransformation = m_monochromatic_shader_program_u_transformation;
            break;
        }
    }

    assert(shaderProgram);

    openGLDevice->use_program(shaderProgram->gl_handle);

    if (shaderProgram == m_current_custom_shader_program)
    {
        // Apply custom shader uniforms
        const auto shaderImpl  = static_cast<ShaderImpl*>(sprite_shader().impl());
        const auto cbufferData = shaderImpl->cbuffer_data();

        for (const auto& param : shaderImpl->dirty_scalar_parameters())
        {
            const auto location = shaderProgram->uniform_location(param->name);
            if (location == -1)
            {
                continue;
            }

            const auto paramData        = cbufferData + param->offset;
            const auto paramDataAsFloat = reinterpret_cast<const GLfloat*>(paramData);
            const auto paramDataAsInt   = reinterpret_cast<const GLint*>(paramData);

            switch (param->type)
            {
                case ShaderParameterType::Float: glUniform1f(location, *paramDataAsFloat); break;
                case ShaderParameterType::Int: glUniform1i(location, *paramDataAsInt); break;
                case ShaderParameterType::Bool: glUniform1i(location, *paramDataAsInt); break;
                case ShaderParameterType::Vector2:
                    glUniform2fv(location, 1, paramDataAsFloat);
                    break;
                case ShaderParameterType::Vector3:
                    glUniform3fv(location, 1, paramDataAsFloat);
                    break;
                case ShaderParameterType::Vector4:
                    glUniform4fv(location, 1, paramDataAsFloat);
                    break;
                case ShaderParameterType::Matrix:
                    glUniformMatrix4fv(location, 1, GL_FALSE, paramDataAsFloat);
                    break;
                case ShaderParameterType::Image: assert(false && "should not happen"); break;
                case ShaderParameterType::FloatArray:
                    glUniform1fv(location, GLsizei(param->array_size), paramDataAsFloat);
                    break;
                case ShaderParameterType::IntArray:
                    glUniform1iv(location, GLsizei(param->array_size), paramDataAsInt);
                    break;
                case ShaderParameterType::BoolArray:
                    glUniform1iv(location, GLsizei(param->array_size), paramDataAsInt);
                    break;
                case ShaderParameterType::Vector2Array:
                    glUniform2fv(location, GLsizei(param->array_size), paramDataAsFloat);
                    break;
                case ShaderParameterType::Vector3Array:
                    glUniform3fv(location, GLsizei(param->array_size), paramDataAsFloat);
                    break;
                case ShaderParameterType::Vector4Array:
                    glUniform4fv(location, GLsizei(param->array_size), paramDataAsFloat);
                    break;
                case ShaderParameterType::MatrixArray:
                    glUniformMatrix4fv(location,
                                       GLsizei(param->array_size),
                                       GL_FALSE,
                                       paramDataAsFloat);
                    break;
            }
        }

        shaderImpl->clear_dirty_scalar_parameters();

        for (const auto& param : shaderImpl->dirty_image_parameters())
        {
            // We don't have to update the shader program's uniforms, because they were
            // already set during construction.
            // Instead, we have to figure out which texture slots those parameters correspond
            // to and bind the parameter's images to those slots.
            const auto openGLImage = static_cast<OpenGLImage*>(param->image.impl());

            glActiveTexture(GL_TEXTURE0 + OpenGLSpriteBatch::texture_slot_base_offset +
                            param->offset);
            glBindTexture(GL_TEXTURE_2D, openGLImage ? openGLImage->gl_handle : 0);

            if (openGLImage)
            {
                // TODO: make the sampler a parameter-based setting
                const auto sampler = Sampler::linear_repeat();
                apply_sampler_to_gl_context(sampler);
                openGLImage->last_applied_sampler = sampler;
            }
        }

        shaderImpl->clear_dirty_image_parameters();
    }

    const auto transformation = current_transformation();
    GL_CALL(glUniformMatrix4fv(uTransformation, 1, GL_FALSE, transformation.data()));

    const auto openGLImage = static_cast<OpenGLImage*>(image.impl());

    GL_CALL(glActiveTexture(GL_TEXTURE0));
    GL_CALL(glBindTexture(GL_TEXTURE_2D, openGLImage->gl_handle));

    const auto sampler = current_sampler();

    if (*shaderProgram == m_monochromatic_shader_program)
    {
        // We're drawing text. Use nearest neighbor interpolation.
        apply_sampler_to_gl_context(Sampler::point_clamp());
    }
    else if (openGLImage->last_applied_sampler != sampler)
    {
        // We're drawing sprites.
        apply_sampler_to_gl_context(sampler);
        openGLImage->last_applied_sampler = sampler;
    }
}

void OpenGLSpriteBatch::fill_vertices_and_draw(uint32_t         batch_start,
                                               uint32_t         batch_size,
                                               const Rectangle& texture_size_and_inverse,
                                               bool             flip_image_up_down)
{
    constexpr auto useBufferSubData = true;

    if (useBufferSubData && !m_vertex_data)
    {
        m_vertex_data = std::make_unique<Vertex[]>(max_batch_size * vertices_per_sprite);
    }

    const auto startVertex = batch_start * vertices_per_sprite;
    const auto vertexCount = batch_size * vertices_per_sprite;

    Vertex* vertices = nullptr;

    if (useBufferSubData)
    {
        vertices = m_vertex_data.get() + startVertex;
    }
    else
    {
        auto mapFlags = GLbitfield(GL_MAP_WRITE_BIT);
        if (batch_start == 0)
        {
            mapFlags |= GL_MAP_INVALIDATE_BUFFER_BIT;
        }

        mapFlags |= GL_MAP_UNSYNCHRONIZED_BIT;

        vertices = static_cast<Vertex*>(glMapBufferRange(GL_ARRAY_BUFFER,
                                                         startVertex * sizeof(Vertex),
                                                         vertexCount * sizeof(Vertex),
                                                         mapFlags));
    }

    fill_sprite_vertices(vertices,
                         batch_start,
                         batch_size,
                         texture_size_and_inverse,
                         flip_image_up_down);

    if (useBufferSubData)
    {
        GL_CALL(glBufferSubData(GL_ARRAY_BUFFER,
                                GLintptr(startVertex * sizeof(Vertex)),
                                GLsizeiptr(vertexCount * sizeof(Vertex)),
                                vertices));
    }
    else
    {
        GL_CALL(glUnmapBuffer(GL_ARRAY_BUFFER));
    }

    const auto startIndex = batch_start * indices_per_sprite;
    const auto indexCount = batch_size * indices_per_sprite;

    GL_CALL(glDrawElements(GL_TRIANGLES,
                           GLsizei(indexCount),
                           GL_UNSIGNED_SHORT,
                           reinterpret_cast<const void*>(startIndex * sizeof(uint16_t))));

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

static GLenum Convert(ImageAddressMode mode)
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

    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GLint(Convert(sampler.address_u))));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GLint(Convert(sampler.address_v))));

    // TODO: sampler.textureComparison
    // TODO: sampler.maxAnisotropy
    // TODO: sampler.borderColor
}

static GLenum Convert(BlendFunction function)
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

static GLenum Convert(Blend blend)
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

    GL_CALL(glBlendFuncSeparate(Convert(blend_state.color_src_blend),
                                Convert(blend_state.color_dst_blend),
                                Convert(blend_state.alpha_src_blend),
                                Convert(blend_state.alpha_dst_blend)));

    GL_CALL(glBlendEquationSeparate(Convert(blend_state.color_blend_function),
                                    Convert(blend_state.alpha_blend_function)));

    const auto colorMask = blend_state.color_write_mask;

    GL_CALL(glColorMask(
        (int(colorMask) & int(ColorWriteMask::Red)) == int(ColorWriteMask::Red) ? GL_TRUE
                                                                                : GL_FALSE,
        (int(colorMask) & int(ColorWriteMask::Green)) == int(ColorWriteMask::Green) ? GL_TRUE
                                                                                    : GL_FALSE,
        (int(colorMask) & int(ColorWriteMask::Blue)) == int(ColorWriteMask::Blue) ? GL_TRUE
                                                                                  : GL_FALSE,
        (int(colorMask) & int(ColorWriteMask::Alpha)) == int(ColorWriteMask::Alpha) ? GL_TRUE
                                                                                    : GL_FALSE));

    m_last_applied_blend_state = blend_state;
}

auto OpenGLSpriteBatch::on_shader_destroyed(gsl::not_null<ShaderImpl*> shader) -> void
{
    const auto openGLShader = static_cast<const OpenGLUserShader*>(shader.get());
    const auto it           = m_custom_shader_programs.find(openGLShader);

    if (it != m_custom_shader_programs.cend())
    {
        log_verbose("Erasing OpenGLShaderProgram with user-origin '{}'", shader->name());
        m_custom_shader_programs.erase(it);
    }

    SpriteBatch::on_shader_destroyed(shader);
}
} // namespace cer::details