// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "cerlib/Font.hpp"

#include "FontImpl.hpp"
#include "cerlib/Vector2.hpp"
#include "contentmanagement/ContentManager.hpp"
#include "game/GameImpl.hpp"
#include <cassert>
#include <cerlib/Util2.hpp>

namespace cer
{
CERLIB_IMPLEMENT_OBJECT(Font);

Font::Font(std::string_view asset_name)
    : m_impl(nullptr)
{
    auto& content = details::GameImpl::instance().content_manager();
    *this         = content.load_font(asset_name);
}

Font::Font(std::span<const std::byte> data)
    : m_impl(nullptr)
{
    set_impl(*this, std::make_unique<details::FontImpl>(data, true).release());
}

auto Font::built_in(bool bold) -> Font
{
    return Font{&details::FontImpl::built_in(bold)};
}

auto Font::measure(std::string_view text, uint32_t size) const -> Vector2
{
    assert(m_impl);
    return m_impl->measure(text, size);
}

auto Font::line_height(uint32_t size) const -> float
{
    assert(m_impl);
    return m_impl->line_height(size);
}

void Font::for_each_glyph(
    std::string_view                                               text,
    uint32_t                                                       size,
    const std::function<bool(uint32_t codepoint, Rectangle rect)>& action) const
{
    assert(m_impl);

    m_impl->for_each_glyph<false>(text, size, [&](uint32_t codepoint, Rectangle rect) {
        return action(codepoint, rect);
    });
}
} // namespace cer
