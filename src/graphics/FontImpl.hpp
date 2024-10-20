// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "cerlib/Content.hpp"
#include "cerlib/Image.hpp"
#include "util/BinPack.hpp"
#include "util/Object.hpp"

#include "stb_truetype.hpp"
#include "util/utf8.hpp"
#include <unordered_map>
#include <unordered_set>

namespace cer::details
{
class FontImpl final : public Object, public Asset
{
  public:
    struct RasterizedGlyph
    {
        Rectangle uv_rect;
        uint32_t  page_index{};
    };

    struct FontPage
    {
        uint32_t                     width;
        uint32_t                     height;
        BinPack                      pack;
        std::unique_ptr<std::byte[]> atlas_data;
        Image                        atlas;
    };

    struct GlyphIterationExtras
    {
        float     line_increment{};
        float     ascent{};
        float     descent{};
        Rectangle line_rect_thus_far{};
        bool      is_last_on_line{};
    };

    explicit FontImpl(std::span<const std::byte> data, bool create_copy_of_data);

    explicit FontImpl(std::unique_ptr<std::byte[]> data);

    ~FontImpl() noexcept override;

    static void create_built_in_fonts();

    static void destroy_built_in_fonts();

    static auto built_in(bool bold) -> FontImpl&;

    auto measure(std::string_view text, uint32_t font_size) const -> Vector2;

    template <bool ComputeExtras = false, typename TAction>
    void for_each_glyph(std::string_view text, uint32_t font_size, const TAction& action) const
    {
        const auto font_sizef = float(font_size);

        auto pen_x = 0.0;
        auto pen_y = 0.0;

        const auto scale = stbtt_ScaleForPixelHeight(&m_font_info, font_sizef);

        const auto ascent   = double(m_ascent) * scale;
        const auto descent  = double(m_descent) * scale;
        const auto line_gap = double(m_line_gap) * scale;

        auto it     = utf8::iterator(text.begin(), text.begin(), text.end());
        auto it_end = utf8::iterator(text.end(), text.begin(), text.end());

        auto codepoint = char32_t(0);
        if (it != it_end)
        {
            codepoint = *it;
        }

        const auto line_increment = ascent - descent + line_gap;

        auto extras = GlyphIterationExtras();

        if constexpr (ComputeExtras)
        {
            extras.line_increment = float(line_increment);
            extras.ascent         = float(ascent);
            extras.descent        = float(descent);
        }

        constexpr auto newline = char32_t('\n');

        while (it != it_end)
        {
            if (codepoint == newline)
            {
                pen_x = 0.0;
                pen_y += line_increment;

                ++it;
                if (it != it_end)
                {
                    codepoint = *it;
                }

                if constexpr (ComputeExtras)
                {
                    extras.line_rect_thus_far = {};
                }

                continue;
            }

            auto bleft   = 0;
            auto btop    = 0;
            auto bright  = 0;
            auto bbottom = 0;

            stbtt_GetCodepointBitmapBox(&m_font_info,
                                        int(codepoint),
                                        scale,
                                        scale,
                                        &bleft,
                                        &btop,
                                        &bright,
                                        &bbottom);

            const auto x = float(pen_x);
            const auto y = float(pen_y + ascent + btop);

            auto advance_x = 0;

            stbtt_GetCodepointHMetrics(&m_font_info, int(codepoint), &advance_x, nullptr);

            const auto width  = float(bright - bleft);
            const auto height = float(bbottom - btop);

            const auto rect = Rectangle{x, y, width, height};

            if constexpr (ComputeExtras)
            {
                if (extras.line_rect_thus_far == Rectangle{})
                {
                    extras.line_rect_thus_far = rect;
                }
                else
                {
                    const auto l = min(extras.line_rect_thus_far.left(), rect.left());
                    const auto r = max(extras.line_rect_thus_far.right(), rect.right());
                    const auto t = min(extras.line_rect_thus_far.top(), rect.top());
                    const auto b = max(extras.line_rect_thus_far.bottom(), rect.bottom());

                    extras.line_rect_thus_far = Rectangle{l, t, r - l, b - t};
                }
            }

            ++it;
            const auto is_last        = it == it_end;
            const auto next_codepoint = is_last ? 0 : *it;

            if constexpr (ComputeExtras)
            {
                extras.is_last_on_line = is_last || next_codepoint == newline;

                if (!action(codepoint, rect, extras))
                {
                    break;
                }
            }
            else
            {
                if (!action(codepoint, rect))
                {
                    break;
                }
            }

            pen_x += advance_x * scale;

            if (!is_last)
            {
                int kern = 0;
                kern     = stbtt_GetCodepointKernAdvance(&m_font_info, codepoint, next_codepoint);
                pen_x += kern * scale;
            }

            codepoint = next_codepoint;
        }
    }

    auto page(uint32_t index) const -> const FontPage&;

    auto rasterized_glyph(uint32_t codepoint, uint32_t font_size) -> const RasterizedGlyph&;

    auto line_height(uint32_t size) const -> float;

  private:
    struct RasterizedGlyphKey
    {
        uint32_t codepoint;
        uint32_t font_size;
    };

    struct RasterizedGlyphKeyHash
    {
        auto operator()(const RasterizedGlyphKey& key) const -> size_t
        {
            return key.codepoint ^ key.font_size;
        }
    };

    struct RasterizedGlyphKeyEqual
    {
        auto operator()(const RasterizedGlyphKey& lhs, const RasterizedGlyphKey& rhs) const -> bool
        {
            return lhs.codepoint == rhs.codepoint && lhs.font_size == rhs.font_size;
        }
    };

    using RasterizedGlyphsMap = std::unordered_map<RasterizedGlyphKey,
                                                   RasterizedGlyph,
                                                   RasterizedGlyphKeyHash,
                                                   RasterizedGlyphKeyEqual>;

    void initialize();

    auto rasterize_glyph(const RasterizedGlyphKey& key, bool update_page_image_immediately)
        -> const RasterizedGlyph&;

    void append_new_page();

    static void update_page_atlas_image(FontPage& page);

    std::byte*                      m_font_data{};
    bool                            m_owns_font_data{};
    stbtt_fontinfo                  m_font_info{};
    int                             m_ascent{};
    int                             m_descent{};
    int                             m_line_gap{};
    RasterizedGlyphsMap             m_rasterized_glyphs;
    std::vector<FontPage>           m_pages;
    std::vector<FontPage>::iterator m_current_page_iterator;
    std::unordered_set<uint32_t>    m_initialized_sizes;
    std::unordered_set<size_t>      m_page_images_to_update;
};
} // namespace cer::details
