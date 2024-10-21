// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "OpenGLPrerequisites.hpp"
#include "OpenGLVao.hpp"
#include "graphics/GraphicsDevice.hpp"
#include <optional>

namespace cer::details
{
class OpenGLGraphicsDevice final : public GraphicsDevice
{
  public:
    explicit OpenGLGraphicsDevice(WindowImpl& main_window);

    forbid_copy_and_move(OpenGLGraphicsDevice);

    ~OpenGLGraphicsDevice() noexcept override;

    void on_start_frame(const Window& window) override;

    void on_end_frame(const Window& window) override;

    void on_start_imgui_frame(const Window& window) override;

    void on_end_imgui_frame(const Window& window) override;

    void on_set_canvas(const Image& canvas, const Rectangle& viewport) override;

    void on_set_scissor_rects(std::span<const Rectangle> scissor_rects) override;

    auto create_canvas(const Window& window, uint32_t width, uint32_t height, ImageFormat format)
        -> std::unique_ptr<ImageImpl> override;

    auto create_image(uint32_t width, uint32_t height, ImageFormat format, const void* data)
        -> std::unique_ptr<ImageImpl> override;

    auto opengl_features() const -> const OpenGLFeatures&;

    void bind_vao(const OpenGLVao& vao);

    void use_program(GLuint program);

    void read_canvas_data_into(const Image& canvas,
                               uint32_t     x,
                               uint32_t     y,
                               uint32_t     width,
                               uint32_t     height,
                               void*        destination) override;

  protected:
    auto create_native_user_shader(std::string_view          native_code,
                                   ShaderImpl::ParameterList parameters)
        -> std::unique_ptr<ShaderImpl> override;

  private:
    struct PerOpenGLContextState
    {
        int                   last_applied_gl_swap_interval{-1};
        std::optional<GLuint> last_used_shader_program;

        // When VAOs are not supported, this counts how many vertex attributes we have
        // currently enabled. Used to only enable/disable attributes that changed.
        size_t enabled_vertex_attrib_count{};
    };

    OpenGLFeatures                                         m_features;
    std::unordered_map<WindowImpl*, PerOpenGLContextState> m_per_open_gl_context_states;
    PerOpenGLContextState*                                 m_open_gl_context_state{};
};
} // namespace cer::details
