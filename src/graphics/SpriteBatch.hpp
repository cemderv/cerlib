// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "cerlib/BlendState.hpp"
#include "cerlib/Color.hpp"
#include "cerlib/Drawing.hpp"
#include "cerlib/Image.hpp"
#include "cerlib/Matrix.hpp"
#include "cerlib/Rectangle.hpp"
#include "cerlib/Sampler.hpp"
#include "cerlib/Shader.hpp"
#include "cerlib/Vector2.hpp"
#include "cerlib/Vector4.hpp"
#include "util/NonCopyable.hpp"

#include <gsl/pointers>

namespace cer
{
class Font;
struct FrameStats;
} // namespace cer

namespace cer::details
{
class GraphicsDevice;

class SpriteBatch
{
  public:
    struct Vertex
    {
        Vector4 position;
        Color   color;
        Vector2 uv;
    };

    static constexpr uint32_t max_batch_size      = 2048;
    static constexpr uint32_t min_batch_size      = 128;
    static constexpr uint32_t initial_queue_size  = 512;
    static constexpr uint32_t vertices_per_sprite = 4;
    static constexpr uint32_t indices_per_sprite  = 6;

    enum class SpriteShaderKind
    {
        Default       = 1, // default rgba sprite shader
        Monochromatic = 2, // splats .r to .rrrr (e.g. for monochromatic bitmap fonts)
    };

    explicit SpriteBatch(gsl::not_null<GraphicsDevice*> device_impl,
                         gsl::not_null<FrameStats*>     draw_stats);

    NON_COPYABLE_NON_MOVABLE(SpriteBatch);

    virtual ~SpriteBatch() noexcept;

    void begin(const Matrix&     transformation,
               const BlendState& blend_state,
               const Shader&     shader,
               const Sampler&    sampler);

    void draw_sprite(const Sprite& sprite, SpriteShaderKind shader = SpriteShaderKind::Default);

    void draw_string(std::string_view                     text,
                     const Font&                          font,
                     uint32_t                             font_size,
                     const Vector2&                       position,
                     const Color&                         color,
                     const std::optional<TextDecoration>& decoration);

    void fill_rectangle(const Rectangle& rectangle,
                        const Color&     color,
                        float            rotation,
                        const Vector2&   origin);

    void end();

    virtual void on_shader_destroyed(gsl::not_null<ShaderImpl*> shader);

    void release_resources();

  protected:
    virtual void prepare_for_rendering() = 0;

    virtual void set_up_batch(const Image&     image,
                              SpriteShaderKind shader_kind,
                              uint32_t         start,
                              uint32_t         count) = 0;

    virtual void fill_vertices_and_draw(uint32_t         batch_start,
                                        uint32_t         batch_size,
                                        const Rectangle& texture_size_and_inverse,
                                        bool             flip_image_up_down) = 0;

    virtual void on_end_rendering() = 0;

    void fill_sprite_vertices(Vertex*          dst,
                              uint32_t         batch_start,
                              uint32_t         batch_size,
                              const Rectangle& texture_size_and_inverse,
                              bool             flip_image_up_down) const;

    gsl::not_null<GraphicsDevice*> parent_device() const
    {
        return m_parent_device;
    }

    Matrix current_transformation() const
    {
        return m_transformation;
    }

    const BlendState& current_blend_state() const
    {
        return m_blend_state;
    }

    const Sampler& current_sampler() const
    {
        return m_sampler;
    }

    FrameStats& frame_stats()
    {
        return *m_frame_stats;
    }

    const Shader& sprite_shader() const
    {
        return m_sprite_shader;
    }

  private:
    struct InternalSprite
    {
        Image            image;
        Rectangle        dst;
        Rectangle        src;
        Color            color;
        Vector2          origin{};
        float            rotation{};
        SpriteFlip       flip{SpriteFlip::None};
        SpriteShaderKind shader_kind{SpriteShaderKind::Default};
    };

    void verify_has_begun() const;

    void flush();

    void render_batch(const Image& image, SpriteShaderKind shader, uint32_t start, uint32_t count);

    static void render_sprite(const InternalSprite& sprite,
                              Vertex*               dst_vertices,
                              const Rectangle&      texture_size_and_inverse,
                              bool                  flip_image_up_down);

    gsl::not_null<GraphicsDevice*> m_parent_device;
    gsl::not_null<FrameStats*>     m_frame_stats;
    std::vector<InternalSprite>    m_sprite_queue;
    uint32_t                       m_vertex_buffer_position;
    Image                          m_white_image;
    Matrix                         m_transformation;
    BlendState                     m_blend_state;
    Shader                         m_sprite_shader;
    Sampler                        m_sampler;
    bool                           m_is_in_begin_end_pair{};
};
} // namespace cer::details
