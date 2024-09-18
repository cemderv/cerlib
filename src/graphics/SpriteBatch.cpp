// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "SpriteBatch.hpp"
#include "FontImpl.hpp"
#include "GraphicsDevice.hpp"
#include "ImageImpl.hpp"
#include "Tessellation2D.hpp"
#include "cerlib/Font.hpp"
#include "cerlib/Image.hpp"
#include "cerlib/Logging.hpp"
#include "util/Util.hpp"
#include <array>
#include <cassert>
#include <gsl/narrow>

namespace cer::details
{
SpriteBatch::SpriteBatch(gsl::not_null<GraphicsDevice*> device_impl,
                         gsl::not_null<FrameStats*>     draw_stats)
    : m_parent_device(device_impl)
    , m_frame_stats(draw_stats)
    , m_vertex_buffer_position(0)
{
    log_debug("Creating SpriteBatch");

    m_sprite_queue.reserve(initial_queue_size);

    // White image
    {
        constexpr auto size = 1;

        auto data = std::array<uint8_t, 4 * size * size>();
        std::ranges::fill(data, 255);

        m_white_image = Image(m_parent_device
                                  ->create_image(size,
                                                 size,
                                                 ImageFormat::R8G8B8A8_UNorm,
                                                 1,
                                                 [&](uint32_t) { return data.data(); })
                                  .get());
    }

    log_debug("Created SpriteBatch");
}

SpriteBatch::~SpriteBatch() noexcept = default;

void SpriteBatch::begin(const Matrix&     transformation,
                        const BlendState& blend_state,
                        const Shader&     pixel_shader,
                        const Sampler&    sampler)
{
    assert(!m_is_in_begin_end_pair);

    m_transformation = transformation;
    m_blend_state    = blend_state;
    m_sprite_shader  = pixel_shader;
    m_sampler        = sampler;

    m_is_in_begin_end_pair = true;

    assert(m_sprite_queue.empty());
}

void SpriteBatch::draw_sprite(const Sprite& sprite, SpriteShaderKind sprite_shader)
{
    verify_has_begun();

    Rectangle src_rect;
    if (sprite.src_rect)
    {
        src_rect = *sprite.src_rect;
    }
    else
    {
        src_rect = Rectangle(0, 0, sprite.image.size());
    }

    m_sprite_queue.push_back(InternalSprite{
        .image       = sprite.image,
        .dst         = sprite.dst_rect,
        .src         = src_rect,
        .color       = sprite.color,
        .origin      = sprite.origin,
        .rotation    = sprite.rotation,
        .flip        = sprite.flip,
        .shader_kind = sprite_shader,
    });
}

void SpriteBatch::draw_string(std::string_view                     text,
                              const Font&                          font,
                              uint32_t                             font_size,
                              const Vector2&                       position,
                              const Color&                         color,
                              const std::optional<TextDecoration>& decoration)
{
    verify_has_begun();

    FontImpl* font_impl = font ? font.impl() : FontImpl::built_in(false);

    const float line_height  = font_impl->line_height(font_size);
    const float stroke_width = line_height * 0.1f;

    if (!decoration.has_value())
    {
        font_impl->for_each_glyph<false>(text, font_size, [&](uint32_t codepoint, Rectangle rect) {
            const FontImpl::RasterizedGlyph& glyph =
                font_impl->rasterized_glyph(codepoint, font_size);

            const FontImpl::FontPage& page = font_impl->page(glyph.page_index);

            rect.x += position.x;
            rect.y += position.y;

            draw_sprite(
                Sprite{
                    .image    = page.atlas,
                    .dst_rect = rect,
                    .src_rect = glyph.uv_rect,
                    .color    = color,
                    .rotation = 0.0f,
                    .origin   = Vector2(),
                    .flip     = SpriteFlip::None,
                },
                SpriteShaderKind::Monochromatic);

            return true;
        });
    }
    else
    {
        assert(decoration.has_value());

        font_impl->for_each_glyph<true>(
            text,
            font_size,
            [&](uint32_t codepoint, Rectangle rect, const FontImpl::GlyphIterationExtras& extras) {
                const FontImpl::RasterizedGlyph& glyph =
                    font_impl->rasterized_glyph(codepoint, font_size);

                const FontImpl::FontPage& page = font_impl->page(glyph.page_index);

                rect.x += position.x;
                rect.y += position.y;

                draw_sprite(
                    Sprite{
                        .image    = page.atlas,
                        .dst_rect = rect,
                        .src_rect = glyph.uv_rect,
                        .color    = color,
                        .rotation = 0.0f,
                        .origin   = Vector2(),
                        .flip     = SpriteFlip::None,
                    },
                    SpriteShaderKind::Monochromatic);

                if (extras.is_last_on_line)
                {
                    if (const TextUnderline* underline = std::get_if<TextUnderline>(&*decoration))
                    {
                        Rectangle underline_rect = extras.line_rect_thus_far;
                        underline_rect.x += position.x;
                        underline_rect.y += position.y;
                        underline_rect.y += underline_rect.height;
                        underline_rect.height = clamp(underline->thickness.value_or(stroke_width),
                                                      1.0f,
                                                      line_height * 0.5f);
                        underline_rect.y += underline_rect.height / 2.0f;

                        fill_rectangle(underline_rect,
                                       underline->color.value_or(color),
                                       0.0f,
                                       Vector2());
                    }
                    else if (const auto* strikethrough =
                                 std::get_if<TextStrikethrough>(&*decoration))
                    {
                        Rectangle underline_rect = extras.line_rect_thus_far;
                        underline_rect.x += position.x;
                        underline_rect.y += position.y;
                        underline_rect.y += underline_rect.height / 2;
                        underline_rect.height =
                            clamp(strikethrough->thickness.value_or(stroke_width),
                                  1.0f,
                                  line_height * 0.5f);
                        underline_rect.y -= underline_rect.height / 2.0f;

                        fill_rectangle(underline_rect,
                                       strikethrough->color.value_or(color),
                                       0.0f,
                                       Vector2());
                    }
                }

                return true;
            });
    }
}

void SpriteBatch::fill_rectangle(const Rectangle& rectangle,
                                 const Color&     color,
                                 float            rotation,
                                 const Vector2&   origin)
{
    verify_has_begun();

    draw_sprite(Sprite{
        .image    = m_white_image,
        .dst_rect = rectangle,
        .src_rect = {},
        .color    = color,
        .rotation = rotation,
        .origin   = origin,
        .flip     = SpriteFlip::None,
    });
}

void SpriteBatch::end()
{
    assert(m_is_in_begin_end_pair);

    if (!m_sprite_queue.empty())
    {
        prepare_for_rendering();
        flush();
    }

    on_end_rendering();

    m_is_in_begin_end_pair = false;
}

void SpriteBatch::on_shader_destroyed(gsl::not_null<ShaderImpl*> shader)
{
    CERLIB_UNUSED(shader);
}

void SpriteBatch::release_resources()
{
    m_sprite_queue.clear();
    m_white_image   = {};
    m_sprite_shader = {};
}

void SpriteBatch::verify_has_begun() const
{
    assert(m_is_in_begin_end_pair);
}

auto SpriteBatch::flush() -> void
{
    Image            batch_image;
    uint32_t         batch_start{};
    SpriteShaderKind batch_shader = SpriteShaderKind::Default;
    const uint32_t   sprite_count = gsl::narrow_cast<uint32_t>(m_sprite_queue.size());

    for (uint32_t i = 0; i < sprite_count; ++i)
    {
        const InternalSprite&  sprite      = m_sprite_queue[i];
        const Image&           image       = sprite.image;
        const SpriteShaderKind shader_kind = sprite.shader_kind;

        if (image != batch_image || shader_kind != batch_shader)
        {
            if (i > batch_start)
            {
                render_batch(batch_image, batch_shader, batch_start, i - batch_start);
            }

            batch_image  = image;
            batch_shader = shader_kind;
            batch_start  = i;
        }
    }

    render_batch(batch_image, batch_shader, batch_start, sprite_count - batch_start);

    m_sprite_queue.clear();
}

void SpriteBatch::render_batch(const Image&     image,
                               SpriteShaderKind shader,
                               uint32_t         start,
                               uint32_t         count)
{
    constexpr bool are_canvases_flipped_up_down = true;

    assert(image);

    set_up_batch(image, shader, start, count);

    const float image_width  = image.widthf();
    const float image_height = image.heightf();

    assert(!is_zero(image_width));
    assert(!is_zero(image_height));

    const float inverse_image_width  = 1.0f / image_width;
    const float inverse_image_height = 1.0f / image_height;

    const Rectangle texture_size_and_inverse{image_width,
                                             image_height,
                                             inverse_image_width,
                                             inverse_image_height};

    const bool flip_image_up_down = are_canvases_flipped_up_down && image.is_canvas();

    while (count > 0)
    {
        uint32_t       batch_size      = count;
        const uint32_t remaining_space = max_batch_size - m_vertex_buffer_position;

        if (batch_size > remaining_space)
        {
            if (remaining_space < min_batch_size)
            {
                m_vertex_buffer_position = 0;
                batch_size               = min(count, max_batch_size);
            }
            else
            {
                batch_size = remaining_space;
            }
        }

        fill_vertices_and_draw(start, batch_size, texture_size_and_inverse, flip_image_up_down);

        m_vertex_buffer_position += batch_size;
        start += batch_size;
        count -= batch_size;
    }
}

void SpriteBatch::fill_sprite_vertices(Vertex*          dst,
                                       uint32_t         batch_start,
                                       uint32_t         batch_size,
                                       const Rectangle& texture_size_and_inverse,
                                       bool             flip_image_up_down) const
{
    for (uint32_t i = 0; i < batch_size; ++i)
    {
        render_sprite(m_sprite_queue[batch_start + i],
                      dst,
                      texture_size_and_inverse,
                      flip_image_up_down);

        dst += vertices_per_sprite;
    }
}

void SpriteBatch::render_sprite(const InternalSprite& sprite,
                                Vertex*               dst_vertices,
                                const Rectangle&      texture_size_and_inverse,
                                bool                  flip_image_up_down)
{
    const Rectangle destination = sprite.dst;
    const Rectangle source      = sprite.src.scaled(texture_size_and_inverse.size());
    const Color     color       = sprite.color;

    Vector2 origin = sprite.origin;
    if (sprite.src.width != 0.0f)
    {
        origin.x /= sprite.src.width;
    }
    else
    {
        origin.x *= texture_size_and_inverse.width;
    }

    if (sprite.src.height != 0.0f)
    {
        origin.y /= sprite.src.height;
    }
    else
    {
        origin.y *= texture_size_and_inverse.height;
    }

    const float rotation = sprite.rotation;

    const Vector2 destination_pos{destination.x, destination.y};
    const Vector2 destination_size{destination.width, destination.height};

    Vector2 rotation_matrix_row1;
    Vector2 rotation_matrix_row2;

    if (is_zero(rotation))
    {
        rotation_matrix_row1 = Vector2{1, 0};
        rotation_matrix_row2 = Vector2{0, 1};
    }
    else
    {
        const float s = sin(rotation);
        const float c = cos(rotation);

        rotation_matrix_row1 = Vector2{c, s};
        rotation_matrix_row2 = Vector2{-s, c};
    }

    constexpr auto corner_offsets = std::array{
        Vector2{0, 0},
        Vector2{1, 0},
        Vector2{0, 1},
        Vector2{1, 1},
    };

    int flip_flags = static_cast<int>(sprite.flip);

    if (flip_image_up_down)
    {
        flip_flags |= static_cast<int>(SpriteFlip::Vertically);
    }

    const int mirror_bits = flip_flags & 3;

    const Vector2 source_pos  = source.position();
    const Vector2 source_size = source.size();

    for (uint32_t i = 0; i < vertices_per_sprite; ++i)
    {
        const Vector2 corner_offset = (corner_offsets[i] - origin) * destination_size;
        const Vector2 position1 = Vector2{corner_offset.x} * rotation_matrix_row1 + destination_pos;
        const Vector2 position2 = Vector2{corner_offset.y} * rotation_matrix_row2 + position1;

        const Vector4 position = Vector4{position2.x, position2.y, 0.0f, 1.0f};
        const Vector2 uv       = (corner_offsets[i ^ mirror_bits] * source_size) + source_pos;

        dst_vertices[i] = Vertex{
            .position = position,
            .color    = color,
            .uv       = uv,
        };
    }
}
} // namespace cer::details
