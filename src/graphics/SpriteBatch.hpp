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
#include "graphics/TextImpl.hpp"
#include "util/NonCopyable.hpp"
#include "util/inplace_vector.hpp"

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

    static constexpr auto max_batch_size      = 2048u;
    static constexpr auto min_batch_size      = 128u;
    static constexpr auto initial_queue_size  = 512u;
    static constexpr auto vertices_per_sprite = 4u;
    static constexpr auto indices_per_sprite  = 6u;

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

    void draw_text(const Text& text, const Vector2& position, const Color& color);

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

    auto parent_device() const -> gsl::not_null<GraphicsDevice*>
    {
        return m_parent_device;
    }

    auto current_transformation() const -> Matrix
    {
        return m_transformation;
    }

    auto current_blend_state() const -> const BlendState&
    {
        return m_blend_state;
    }

    auto current_sampler() const -> const Sampler&
    {
        return m_sampler;
    }

    auto frame_stats() -> FrameStats&
    {
        return *m_frame_stats;
    }

    auto sprite_shader() const -> const Shader&
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
        Vector2          origin;
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

    void do_draw_text(std::span<const PreshapedGlyph>     glyphs,
                      std::span<const TextDecorationRect> decoration_rects,
                      const Vector2&                      offset,
                      const Color&                        color);

    bool                           m_is_in_begin_end_pair{};
    gsl::not_null<GraphicsDevice*> m_parent_device;
    gsl::not_null<FrameStats*>     m_frame_stats;
    std::vector<InternalSprite>    m_sprite_queue;
    uint32_t                       m_vertex_buffer_position;
    Image                          m_white_image;
    Matrix                         m_transformation;
    BlendState                     m_blend_state;
    Shader                         m_sprite_shader;
    Sampler                        m_sampler;

    // Used in draw_string() as temporary buffers for text shaping results.
    inplace_vector<PreshapedGlyph>     m_tmp_glyphs;
    inplace_vector<TextDecorationRect> m_tmp_decoration_rects;
};
} // namespace cer::details
