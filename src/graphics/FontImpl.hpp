// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "cerlib/Content.hpp"
#include "cerlib/Image.hpp"
#include "util/BinPack.hpp"
#include "util/Object.hpp"

#include <gsl/pointers>
#include <stb_truetype.h>
#include <unordered_map>
#include <unordered_set>
#include <utf8.h>

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

    static gsl::owner<FontImpl*> built_in(bool bold);

    Vector2 measure(std::string_view text, uint32_t font_size) const;

    template <bool ComputeExtras = false, typename TAction>
    void for_each_glyph(std::string_view text, uint32_t font_size, const TAction& action) const
    {
        const auto font_sizef = static_cast<float>(font_size);

        auto pen_x = 0.0;
        auto pen_y = 0.0;

        const auto scale = stbtt_ScaleForPixelHeight(&m_font_info, font_sizef);

        const auto ascent   = static_cast<double>(m_ascent) * scale;
        const auto descent  = static_cast<double>(m_descent) * scale;
        const auto line_gap = static_cast<double>(m_line_gap) * scale;

        auto it     = utf8::iterator(text.begin(), text.begin(), text.end());
        auto it_end = utf8::iterator(text.end(), text.begin(), text.end());

        auto codepoint = static_cast<utf8::utfchar32_t>(0);
        if (it != it_end)
        {
            codepoint = *it;
        }

        const auto line_increment = ascent - descent + line_gap;

        auto extras = GlyphIterationExtras();

        if constexpr (ComputeExtras)
        {
            extras.line_increment = static_cast<float>(line_increment);
            extras.ascent         = static_cast<float>(ascent);
            extras.descent        = static_cast<float>(descent);
        }

        constexpr auto newline = static_cast<utf8::utfchar32_t>('\n');

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

            int bleft = 0, btop = 0, bright = 0, bbottom = 0;
            stbtt_GetCodepointBitmapBox(&m_font_info,
                                        static_cast<int>(codepoint),
                                        scale,
                                        scale,
                                        &bleft,
                                        &btop,
                                        &bright,
                                        &bbottom);

            const float x = static_cast<float>(pen_x);
            const float y = static_cast<float>(pen_y + ascent + btop);

            int advance_x = 0;
            stbtt_GetCodepointHMetrics(&m_font_info,
                                       static_cast<int>(codepoint),
                                       &advance_x,
                                       nullptr);

            const float width  = static_cast<float>(bright - bleft);
            const float height = static_cast<float>(bbottom - btop);

            const Rectangle rect{x, y, width, height};

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

                    extras.line_rect_thus_far = Rectangle(l, t, r - l, b - t);
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

    const FontPage& page(uint32_t index) const;

    const RasterizedGlyph& rasterized_glyph(uint32_t codepoint, uint32_t font_size);

    float line_height(uint32_t size) const;

  private:
    struct RasterizedGlyphKey
    {
        uint32_t codepoint;
        uint32_t font_size;
    };

    struct RasterizedGlyphKeyHash
    {
        size_t operator()(const RasterizedGlyphKey& key) const
        {
            return key.codepoint ^ key.font_size;
        }
    };

    struct RasterizedGlyphKeyEqual
    {
        bool operator()(const RasterizedGlyphKey& lhs, const RasterizedGlyphKey& rhs) const
        {
            return lhs.codepoint == rhs.codepoint && lhs.font_size == rhs.font_size;
        }
    };

    using RasterizedGlyphsMap = std::unordered_map<RasterizedGlyphKey,
                                                   RasterizedGlyph,
                                                   RasterizedGlyphKeyHash,
                                                   RasterizedGlyphKeyEqual>;

    void initialize();

    const RasterizedGlyph& rasterize_glyph(const RasterizedGlyphKey& key,
                                           bool                      update_page_image_immediately);

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
