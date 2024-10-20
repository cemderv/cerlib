// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "FontImpl.hpp"
#include "cerlib/Logging.hpp"
#include "game/GameImpl.hpp"
#include "util/narrow_cast.hpp"
#include <cassert>
#include <cerlib/InternalError.hpp>

#ifdef CERLIB_HAVE_OPENGL
#include "opengl/OpenGLImage.hpp"
#endif

#include "VeraBold.ttf.hpp"
#include "VeraRegular.ttf.hpp"

namespace cer::details
{
static std::unique_ptr<FontImpl> s_built_in_font_regular;
static std::unique_ptr<FontImpl> s_built_in_font_bold;

FontImpl::FontImpl(std::span<const std::byte> data, bool create_copy_of_data)
    : m_owns_font_data(create_copy_of_data)
{
    if (m_owns_font_data)
    {
        m_font_data = std::make_unique<std::byte[]>(data.size()).release();
        std::memcpy(m_font_data, data.data(), data.size());
    }
    else
    {
        m_font_data = const_cast<std::byte*>(data.data()); // NOLINT
    }

    initialize();
}

FontImpl::FontImpl(std::unique_ptr<std::byte[]> data)
    : m_font_data(data.release())
    , m_owns_font_data(true)
{
    initialize();
}

FontImpl::~FontImpl() noexcept
{
    if (m_owns_font_data)
    {
        delete[] m_font_data;
    }
}

void FontImpl::create_built_in_fonts()
{
    log_verbose("Creating built-in font objects");

    s_built_in_font_regular = std::make_unique<FontImpl>(VeraRegular_ttf_span(), false);
    s_built_in_font_regular->add_ref();

    s_built_in_font_bold = std::make_unique<FontImpl>(VeraBold_ttf_span(), false);
    s_built_in_font_bold->add_ref();
}

void FontImpl::destroy_built_in_fonts()
{
    s_built_in_font_regular.reset();
    s_built_in_font_bold.reset();
}

auto FontImpl::built_in(bool bold) -> FontImpl&
{
    return bold ? *s_built_in_font_bold : *s_built_in_font_regular;
}

auto FontImpl::measure(std::string_view text, uint32_t font_size) const -> Vector2
{
    auto left   = 0.0f;
    auto right  = 0.0f;
    auto top    = 0.0f;
    auto bottom = 0.0f;

    for_each_glyph(text, font_size, [&](uint32_t, const Rectangle& rect) {
        left   = min(left, rect.left());
        right  = max(right, rect.right());
        top    = min(top, rect.top());
        bottom = max(bottom, rect.bottom());
        return true;
    });

    return {right - left, bottom - top};
}

auto FontImpl::page(uint32_t index) const -> const FontImpl::FontPage&
{
    return m_pages[index];
}

auto FontImpl::rasterized_glyph(uint32_t codepoint, uint32_t font_size)
    -> const FontImpl::RasterizedGlyph&
{
    if (!m_initialized_sizes.contains(font_size))
    {
        // This is the first time we're encountering this font size.

        for (uint32_t c = 32; c < 255; ++c)
        {
            rasterize_glyph(
                RasterizedGlyphKey{
                    .codepoint = c,
                    .font_size = font_size,
                },
                false);
        }

        m_initialized_sizes.insert(font_size);

        for (const auto page_index : m_page_images_to_update)
        {
            update_page_atlas_image(m_pages[page_index]);
        }

        m_page_images_to_update.clear();
    }

    const auto key = RasterizedGlyphKey{
        .codepoint = codepoint,
        .font_size = font_size,
    };

    const auto it_rasterized_glyph = m_rasterized_glyphs.find(key);

    if (it_rasterized_glyph != m_rasterized_glyphs.cend())
    {
        return it_rasterized_glyph->second;
    }

    return rasterize_glyph(key, true);
}

auto FontImpl::line_height(uint32_t size) const -> float
{
    const auto scale    = stbtt_ScaleForPixelHeight(&m_font_info, float(size));
    const auto ascent   = double(m_ascent) * scale;
    const auto descent  = double(m_descent) * scale;
    const auto line_gap = double(m_line_gap) * scale;

    return float(ascent - descent + line_gap);
}

void FontImpl::initialize()
{
    if (stbtt_InitFont(&m_font_info, reinterpret_cast<const unsigned char*>(m_font_data), 0) == 0)
    {
        CER_THROW_RUNTIME_ERROR_STR("Failed to load the font.");
    }

    stbtt_GetFontVMetrics(&m_font_info, &m_ascent, &m_descent, &m_line_gap);
}

auto FontImpl::rasterize_glyph(const RasterizedGlyphKey& key, bool update_page_image_immediately)
    -> const FontImpl::RasterizedGlyph&
{
    if (m_pages.empty())
    {
        append_new_page();
    }

    assert(m_current_page_iterator != m_pages.cend());

    const auto font_size_f = float(key.font_size);
    const auto scale       = stbtt_ScaleForPixelHeight(&m_font_info, font_size_f);

    auto cx1 = 0;
    auto cy1 = 0;
    auto cx2 = 0;
    auto cy2 = 0;

    stbtt_GetCodepointBitmapBox(&m_font_info,
                                int(key.codepoint),
                                scale,
                                scale,
                                &cx1,
                                &cy1,
                                &cx2,
                                &cy2);

    const auto bitmap_width  = cx2 - cx1;
    const auto bitmap_height = cy2 - cy1;

    auto inserted_rect = m_current_page_iterator->pack.insert(bitmap_width, bitmap_height);

    if (!inserted_rect.has_value())
    {
        append_new_page();
        inserted_rect = m_current_page_iterator->pack.insert(bitmap_width, bitmap_height);
    }

    assert(inserted_rect.has_value());

    const auto x_in_page    = uint32_t(inserted_rect->x);
    const auto y_in_page    = uint32_t(inserted_rect->y);
    const auto page_width   = m_current_page_iterator->width;
    const auto dst_data_idx = (size_t(y_in_page) * size_t(page_width)) + size_t(x_in_page);

    auto* dst_data = m_current_page_iterator->atlas_data.get() + dst_data_idx;

    stbtt_MakeCodepointBitmap(&m_font_info,
                              reinterpret_cast<unsigned char*>(dst_data),
                              bitmap_width,
                              bitmap_height,
                              narrow<int>(page_width),
                              scale,
                              scale,
                              narrow<int>(key.codepoint));

    if (update_page_image_immediately)
    {
        update_page_atlas_image(*m_current_page_iterator);
    }
    else
    {
        m_page_images_to_update.insert(
            size_t(std::distance(m_pages.begin(), m_current_page_iterator)));
    }

    const auto it =
        m_rasterized_glyphs
            .emplace(
                key,
                RasterizedGlyph{
                    .uv_rect    = inserted_rect->to_rectangle(),
                    .page_index = uint32_t(std::distance(m_pages.begin(), m_current_page_iterator)),
                })
            .first;

    return it->second;
}

void FontImpl::append_new_page()
{
    //  const auto caps = cer::capabilities();
    //  const auto width = cer::min(1024u, caps.maxImage2DExtent());
    constexpr auto width  = 1024u;
    constexpr auto height = width;

    m_pages.push_back(FontPage{
        .width      = width,
        .height     = height,
        .pack       = {width, height},
        .atlas_data = std::make_unique<std::byte[]>(size_t(width) * size_t(height)),
        .atlas      = Image(),
    });

    m_current_page_iterator = m_pages.end() - 1;
}

void FontImpl::update_page_atlas_image(FontPage& page)
{
    log_verbose("Updating font page image of size {}x{}", page.width, page.height);

    if (!page.atlas)
    {
        log_verbose("  Reallocating page image");
        page.atlas = Image(page.width, page.height, ImageFormat::R8_UNorm, page.atlas_data.get());
    }
    else
    {
        log_verbose("  Writing directly to page image");

#ifdef CERLIB_HAVE_OPENGL
        verify_opengl_state();

        const auto gl_handle = static_cast<OpenGLImage*>(page.atlas.impl())->gl_handle;

        auto previous_handle = GLuint{};
        glGetIntegerv(GL_TEXTURE_BINDING_2D, reinterpret_cast<GLint*>(&previous_handle));

        if (previous_handle != gl_handle)
        {
            glBindTexture(GL_TEXTURE_2D, gl_handle);
        }

        const auto format_triplet = convert_to_opengl_pixel_format(page.atlas.format());

        glTexSubImage2D(GL_TEXTURE_2D,
                        0,
                        0,
                        0,
                        GLsizei(page.width),
                        GLsizei(page.height),
                        format_triplet.base_format,
                        format_triplet.type,
                        page.atlas_data.get());

        glBindTexture(GL_TEXTURE_2D, previous_handle);

        verify_opengl_state();
#else
        CER_THROW_INTERNAL_ERROR_STR("Not implemented.");
#endif
    }
}
} // namespace cer::details
