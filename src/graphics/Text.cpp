// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "cerlib/Text.hpp"
#include "graphics/TextImpl.hpp"
#include "util/Util.hpp"

namespace cer
{
CERLIB_IMPLEMENT_OBJECT(Text);

Text::Text(std::string_view                     text,
           const Font&                          font,
           uint32_t                             font_size,
           const std::optional<TextDecoration>& decoration)
    : m_impl(nullptr)
{
    std::unique_ptr<details::TextImpl> impl =
        std::make_unique<details::TextImpl>(text, font, font_size, decoration);

    set_impl(*this, impl.release());
}
} // namespace cer
