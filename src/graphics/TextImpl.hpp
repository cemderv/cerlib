// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "cerlib/Drawing.hpp"
#include "cerlib/Font.hpp"
#include "graphics/FontImpl.hpp"
#include "util/Object.hpp"
#include <cassert>
#include <cerlib/List.hpp>
#include <span>

namespace cer::details
{
struct PreshapedGlyph
{
    Image     image;
    Rectangle dst_rect;
    Rectangle src_rect;
};

struct TextDecorationRect
{
    Rectangle            rect;
    std::optional<Color> color;
};

static void shape_text(std::string_view                     text,
                       const Font&                          font,
                       uint32_t                             font_size,
                       const std::optional<TextDecoration>& decoration,
                       List<PreshapedGlyph>&        dst_glyphs,
                       List<TextDecorationRect>&    dst_decoration_rects)
{
    assert(font);

    dst_glyphs.clear();
    dst_decoration_rects.clear();

    auto& font_impl = *font.impl();

    const auto line_height  = font_impl.line_height(font_size);
    const auto stroke_width = line_height * 0.1f;

    if (!decoration.has_value())
    {
        font_impl.for_each_glyph<false>(text, font_size, [&](uint32_t codepoint, Rectangle rect) {
            const auto& glyph = font_impl.rasterized_glyph(codepoint, font_size);
            const auto& page  = font_impl.page(glyph.page_index);

            dst_glyphs.push_back({
                .image    = page.atlas,
                .dst_rect = rect,
                .src_rect = glyph.uv_rect,
            });

            return true;
        });
    }
    else
    {
        assert(decoration.has_value());

        font_impl.for_each_glyph<true>(
            text,
            font_size,
            [&](uint32_t codepoint, Rectangle rect, const FontImpl::GlyphIterationExtras& extras) {
                const auto& glyph = font_impl.rasterized_glyph(codepoint, font_size);
                const auto& page  = font_impl.page(glyph.page_index);

                dst_glyphs.push_back({
                    .image    = page.atlas,
                    .dst_rect = rect,
                    .src_rect = glyph.uv_rect,
                });

                if (extras.is_last_on_line)
                {
                    if (const auto* underline = std::get_if<TextUnderline>(&*decoration))
                    {
                        auto deco_rect = extras.line_rect_thus_far;
                        deco_rect.y += deco_rect.height;
                        deco_rect.height = clamp(underline->thickness.value_or(stroke_width),
                                                 1.0f,
                                                 line_height * 0.5f);
                        deco_rect.y += deco_rect.height / 2.0f;

                        dst_decoration_rects.push_back({
                            .rect  = deco_rect,
                            .color = underline->color,
                        });
                    }
                    else if (const auto* strikethrough =
                                 std::get_if<TextStrikethrough>(&*decoration))
                    {
                        auto deco_rect = extras.line_rect_thus_far;
                        deco_rect.y += deco_rect.height / 2;
                        deco_rect.height = clamp(strikethrough->thickness.value_or(stroke_width),
                                                 1.0f,
                                                 line_height * 0.5f);
                        deco_rect.y -= deco_rect.height / 2.0f;

                        dst_decoration_rects.push_back({
                            .rect  = deco_rect,
                            .color = strikethrough->color,
                        });
                    }
                }

                return true;
            });
    }
}

class TextImpl final : public Object
{
  public:
    TextImpl(std::string_view                     text,
             const Font&                          font,
             uint32_t                             font_size,
             const std::optional<TextDecoration>& decoration);

    auto glyphs() const -> std::span<const PreshapedGlyph>;

    auto decoration_rects() const -> std::span<const TextDecorationRect>;

  private:
    List<PreshapedGlyph>     m_glyphs;
    List<TextDecorationRect> m_decoration_rects;
};
} // namespace cer::details
