// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "ShaderImpl.hpp"
#include "cerlib/BlendState.hpp"
#include "cerlib/Drawing.hpp"
#include "cerlib/Image.hpp"
#include "cerlib/Matrix.hpp"
#include "cerlib/Rectangle.hpp"
#include "cerlib/Sampler.hpp"
#include "cerlib/Shader.hpp"
#include "cerlib/Window.hpp"
#include "util/NonCopyable.hpp"
#include <gsl/pointers>
#include <optional>
#include <span>

#define LOAD_DEVICE_IMPL auto& device_impl = details::GameImpl::instance().graphics_device()

namespace cer
{
class Font;
} // namespace cer

namespace cer::details
{
class WindowImpl;
class SpriteBatch;

class GraphicsDevice
{
  protected:
    explicit GraphicsDevice();

  public:
    NON_COPYABLE_NON_MOVABLE(GraphicsDevice);

    virtual ~GraphicsDevice() noexcept;

    void start_frame(const Window& window);

    void end_frame(const Window& window, const std::function<void()>& post_draw_callback);

    void start_imgui_frame(const Window& window);

    void end_imgui_frame(const Window& window);

    auto demand_create_shader(std::string_view                  name,
                              std::string_view                  source_code,
                              std::span<const std::string_view> defines)
        -> gsl::not_null<ShaderImpl*>;

    virtual auto create_canvas(const Window& window,
                               uint32_t      width,
                               uint32_t      height,
                               ImageFormat   format) -> gsl::not_null<ImageImpl*> = 0;

    virtual auto create_image(uint32_t                   width,
                              uint32_t                   height,
                              ImageFormat                format,
                              uint32_t                   mipmap_count,
                              const Image::DataCallback& data_callback)
        -> gsl::not_null<ImageImpl*> = 0;

    void notify_resource_created(gsl::not_null<GraphicsResourceImpl*> resource);

    virtual void notify_resource_destroyed(gsl::not_null<GraphicsResourceImpl*> resource);

    virtual void notify_user_shader_destroyed(gsl::not_null<ShaderImpl*> resource);

    auto all_resources() const -> const std::vector<gsl::not_null<GraphicsResourceImpl*>>&;

    auto current_canvas() const -> const Image&;

    void set_canvas(const Image& canvas, bool force);

    void set_scissor_rects(std::span<const Rectangle> scissor_rects);

    void set_transformation(const Matrix& transformation);

    auto current_sprite_shader() const -> const Shader&;

    void set_sprite_shader(const Shader& pixel_shader);

    void set_sampler(const Sampler& sampler);

    auto current_blend_state() const -> const BlendState&;

    void set_blend_state(const BlendState& blend_state);

    void draw_sprite(const Sprite& sprite);

    void fill_rectangle(const Rectangle& rectangle,
                        const Color&     color,
                        float            rotation,
                        const Vector2&   origin);

    void draw_string(std::string_view                     text,
                     const Font&                          font,
                     uint32_t                             font_size,
                     const Vector2&                       position,
                     const Color&                         color,
                     const std::optional<TextDecoration>& decoration);

    void draw_text(const Text& text, Vector2 position, const Color& color);

    void draw_particles(const ParticleSystem& particle_system);

    auto frame_stats() const -> FrameStats;

    auto current_canvas_size() const -> Vector2;

    virtual void read_canvas_data_into(const Image& canvas,
                                       uint32_t     x,
                                       uint32_t     y,
                                       uint32_t     width,
                                       uint32_t     height,
                                       void*        destination) = 0;

  protected:
    void post_init(std::unique_ptr<SpriteBatch> sprite_batch);

    virtual auto create_native_user_shader(std::string_view          native_code,
                                           ShaderImpl::ParameterList parameters)
        -> std::unique_ptr<ShaderImpl> = 0;

    virtual void on_start_frame(const Window& window) = 0;

    virtual void on_end_frame(const Window& window) = 0;

    virtual void on_start_imgui_frame(const Window& window) = 0;

    virtual void on_end_imgui_frame(const Window& window) = 0;

    virtual void on_set_canvas(const Image& canvas, const Rectangle& viewport) = 0;

    virtual void on_set_scissor_rects(std::span<const Rectangle> scissor_rects) = 0;

    auto current_window() const -> const Window&;

    auto frame_stats_ptr() -> gsl::not_null<FrameStats*>;

  private:
    enum class Category
    {
        SpriteBatch,
    };

    void ensure_category(Category category);

    void flush_draw_calls();

    static auto compute_viewport_transformation(const Rectangle& viewport) -> Matrix;

    void compute_combined_transformation();

    std::vector<gsl::not_null<GraphicsResourceImpl*>> m_resources;
    std::unique_ptr<SpriteBatch>                      m_sprite_batch;
    Window                                            m_current_window;
    bool                                              m_must_flush_draw_calls;
    FrameStats                                        m_draw_stats;
    Image                                             m_canvas;
    Rectangle                                         m_viewport;
    Matrix                                            m_viewport_transformation;
    Matrix                                            m_combined_transformation;
    Matrix                                            m_transformation;
    BlendState                                        m_blend_state;
    Sampler                                           m_sampler;
    Shader                                            m_sprite_shader;
    std::optional<Category>                           m_current_category;
};
} // namespace cer::details
