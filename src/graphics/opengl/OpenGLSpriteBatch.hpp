// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "OpenGLBuffer.hpp"
#include "OpenGLPrivateShader.hpp"
#include "OpenGLShaderProgram.hpp"
#include "OpenGLUserShader.hpp"
#include "OpenGLVao.hpp"
#include "graphics/SpriteBatch.hpp"
#include "util/NonCopyable.hpp"

namespace cer::details
{
class OpenGLSpriteBatch final : public SpriteBatch
{
  public:
    // The SpriteBatch occupies texture slot 0 (the sprite texture).
    // So image parameters in user-defined shaders must begin after that.
    static constexpr int texture_slot_base_offset = 1;

    explicit OpenGLSpriteBatch(gsl::not_null<GraphicsDevice*> device_impl,
                               gsl::not_null<FrameStats*>     draw_stats);

    NON_COPYABLE_NON_MOVABLE(OpenGLSpriteBatch);

    ~OpenGLSpriteBatch() noexcept override;

  protected:
    void prepare_for_rendering() override;

    void set_up_batch(const Image&     image,
                      SpriteShaderKind shader_kind,
                      uint32_t         start,
                      uint32_t         count) override;

    void fill_vertices_and_draw(uint32_t         batch_start,
                                uint32_t         batch_size,
                                const Rectangle& texture_size_and_inverse,
                                bool             flip_image_up_down) override;

    void on_end_rendering() override;

  private:
    void set_default_render_state();

    static void apply_sampler_to_gl_context(const Sampler& sampler);

    void apply_blend_state_to_gl_context(const BlendState& blend_state);

    void on_shader_destroyed(gsl::not_null<ShaderImpl*> shader) override;

    std::unique_ptr<Vertex[]> m_vertex_data;

    OpenGLPrivateShader m_sprite_vertex_shader;
    OpenGLShaderProgram m_default_sprite_shader_program;
    OpenGLShaderProgram m_monochromatic_shader_program;

    // Uniform locations for built-in shader programs.
    GLint m_default_sprite_shader_program_u_transformation{-1};
    GLint m_monochromatic_shader_program_u_transformation{-1};

    std::unordered_map<const OpenGLUserShader*, OpenGLShaderProgram> m_custom_shader_programs;

    OpenGLShaderProgram* m_current_custom_shader_program{};

    OpenGLBuffer              m_vbo;
    OpenGLBuffer              m_ibo;
    OpenGLVao                 m_vao;
    std::optional<BlendState> m_last_applied_blend_state;
};
} // namespace cer::details