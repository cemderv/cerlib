// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "SpriteBatch.hpp"
#include "FontImpl.hpp"
#include "GraphicsDevice.hpp"
#include "ImageImpl.hpp"
#include "Tessellation2D.hpp"
#include "TextImpl.hpp"
#include "cerlib/Font.hpp"
#include "cerlib/Image.hpp"
#include "cerlib/Logging.hpp"
#include "cerlib/Text.hpp"
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
    log_verbose("Creating SpriteBatch");

    m_sprite_queue.reserve(initial_queue_size);

    // White image
    {
        constexpr auto size = size_t(1);

        auto data = std::array<uint8_t, 4 * size * size>{};
        std::ranges::fill(data, 255);

        m_white_image = Image{
            m_parent_device->create_image(size, size, ImageFormat::R8G8B8A8_UNorm, data.data())
                .get()};
    }

    log_verbose("Created SpriteBatch");
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

    m_sprite_queue.push_back({
        .image       = sprite.image,
        .dst         = sprite.dst_rect,
        .src         = sprite.src_rect.value_or(Rectangle{0, 0, sprite.image.size()}),
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
    assert(font);

    shape_text(text, font, font_size, decoration, m_tmp_glyphs, m_tmp_decoration_rects);

    do_draw_text(m_tmp_glyphs, m_tmp_decoration_rects, position, color);
}

void SpriteBatch::draw_text(const Text& text, const Vector2& position, const Color& color)
{
    verify_has_begun();
    assert(text);

    const TextImpl& text_impl = *text.impl();

    do_draw_text(text_impl.glyphs(), text_impl.decoration_rects(), position, color);
}

void SpriteBatch::fill_rectangle(const Rectangle& rectangle,
                                 const Color&     color,
                                 float            rotation,
                                 const Vector2&   origin)
{
    verify_has_begun();

    draw_sprite({
        .image    = m_white_image,
        .dst_rect = rectangle,
        .color    = color,
        .rotation = rotation,
        .origin   = origin,
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
    auto       batch_image  = Image{};
    auto       batch_start  = 0u;
    auto       batch_shader = SpriteShaderKind::Default;
    const auto sprite_count = gsl::narrow_cast<uint32_t>(m_sprite_queue.size());

    for (uint32_t i = 0; i < sprite_count; ++i)
    {
        const auto& sprite      = m_sprite_queue[i];
        const auto& image       = sprite.image;
        const auto  shader_kind = sprite.shader_kind;

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
    constexpr auto are_canvases_flipped_up_down = true;

    assert(image);

    set_up_batch(image, shader, start, count);

    const auto image_width  = image.widthf();
    const auto image_height = image.heightf();

    assert(!is_zero(image_width));
    assert(!is_zero(image_height));

    const auto inverse_image_width  = 1.0f / image_width;
    const auto inverse_image_height = 1.0f / image_height;

    const auto texture_size_and_inverse = Rectangle{
        image_width,
        image_height,
        inverse_image_width,
        inverse_image_height,
    };

    const auto flip_image_up_down = are_canvases_flipped_up_down && image.is_canvas();

    while (count > 0)
    {
        auto       batch_size      = count;
        const auto remaining_space = max_batch_size - m_vertex_buffer_position;

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

        dst += vertices_per_sprite; // NOLINT
    }
}

void SpriteBatch::render_sprite(const InternalSprite& sprite,
                                Vertex*               dst_vertices,
                                const Rectangle&      texture_size_and_inverse,
                                bool                  flip_image_up_down)
{
    const auto destination = sprite.dst;
    const auto source      = sprite.src.scaled(texture_size_and_inverse.size());
    const auto color       = sprite.color;

    auto origin = sprite.origin;
    if (!is_zero(sprite.src.width))
    {
        origin.x /= sprite.src.width;
    }
    else
    {
        origin.x *= texture_size_and_inverse.width;
    }

    if (!is_zero(sprite.src.height))
    {
        origin.y /= sprite.src.height;
    }
    else
    {
        origin.y *= texture_size_and_inverse.height;
    }

    const auto rotation = sprite.rotation;

    const auto destination_pos  = Vector2{destination.x, destination.y};
    const auto destination_size = Vector2{destination.width, destination.height};

    const auto [rot_matrix_row1, rot_matrix_row2] = [rotation] {
        if (is_zero(rotation))
        {
            return std::pair{Vector2{1, 0}, Vector2{0, 1}};
        }

        const auto s = sin(rotation);
        const auto c = cos(rotation);

        return std::pair{Vector2{c, s}, Vector2{-s, c}};
    }();

    constexpr auto corner_offsets = std::array{
        Vector2{0, 0},
        Vector2{1, 0},
        Vector2{0, 1},
        Vector2{1, 1},
    };

    auto flip_flags = int(sprite.flip);

    if (flip_image_up_down)
    {
        flip_flags |= int(SpriteFlip::Vertically);
    }

    const auto mirror_bits = flip_flags & 3;
    const auto source_pos  = source.position();
    const auto source_size = source.size();

    for (uint32_t i = 0; i < vertices_per_sprite; ++i)
    {
        const auto corner_offset = (corner_offsets[i] - origin) * destination_size;
        const auto position1     = Vector2{corner_offset.x} * rot_matrix_row1 + destination_pos;
        const auto position2     = Vector2{corner_offset.y} * rot_matrix_row2 + position1;

        const auto position = Vector4{position2.x, position2.y, 0.0f, 1.0f};
        const auto uv       = (corner_offsets[i ^ mirror_bits] * source_size) + source_pos;

        // NOLINTBEGIN
        dst_vertices[i] = {
            .position = position,
            .color    = color,
            .uv       = uv,
        };
        // NOLINTEND
    }
}

void SpriteBatch::do_draw_text(std::span<const PreshapedGlyph>     glyphs,
                               std::span<const TextDecorationRect> decoration_rects,
                               const Vector2&                      offset,
                               const Color&                        color)
{
    for (const auto& glyph : glyphs)
    {
        draw_sprite(
            {
                .image    = glyph.image,
                .dst_rect = glyph.dst_rect.offset(offset),
                .src_rect = glyph.src_rect,
                .color    = color,
            },
            SpriteShaderKind::Monochromatic);
    }

    for (const auto& deco : decoration_rects)
    {
        fill_rectangle(deco.rect.offset(offset), deco.color.value_or(color), 0.0f, {});
    }
}
} // namespace cer::details
