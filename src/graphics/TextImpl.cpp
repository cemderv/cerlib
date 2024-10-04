// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "TextImpl.hpp"

#include "cerlib/Font.hpp"
#include <cassert>

namespace cer::details
{
TextImpl::TextImpl(std::string_view                     text,
                   const Font&                          font,
                   uint32_t                             font_size,
                   const std::optional<TextDecoration>& decoration)
{
    shape_text(text,
               font ? font : Font::built_in(false),
               font_size,
               decoration,
               m_glyphs,
               m_decoration_rects);
}

auto TextImpl::glyphs() const -> std::span<const PreshapedGlyph>
{
    return m_glyphs;
}

auto TextImpl::decoration_rects() const -> std::span<const TextDecorationRect>
{
    return m_decoration_rects;
}
} // namespace cer::details
