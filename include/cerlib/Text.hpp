// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <cerlib/Drawing.hpp>
#include <cerlib/String.hpp>
#include <cerlib/details/ObjectMacros.hpp>

namespace cer
{
namespace details
{
class TextImpl;
}

class Font;

/**
 * Represents a pre-shaped, immutable text object that can be used in draw_text().
 *
 * Use a (cached) Text object instead of draw_string() when the text rarely changes,
 * since pre-shaping text is more efficient than shaping it in every draw operation.
 *
 * @ingroup Graphics
 */
class Text
{
    CERLIB_DECLARE_OBJECT(Text);

  public:
    /**
     * Creates an immutable text object that is immediately shaped.
     *
     * @param text The text to draw.
     * @param font The font to draw the text with.
     * @param font_size The size of the font to use, in pixels.
     * @param decoration The text decorations.
     */
    explicit Text(std::string_view              text,
                  const Font&                   font,
                  uint32_t                      font_size,
                  const Option<TextDecoration>& decoration = std::nullopt);
};
} // namespace cer
