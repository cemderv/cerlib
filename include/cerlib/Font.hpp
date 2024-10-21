// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <cerlib/Rectangle.hpp>
#include <cerlib/details/ObjectMacros.hpp>
#include <functional>
#include <span>

namespace cer
{
namespace details
{
class FontImpl;
}

struct Vector2;

/**
 * Represents a font to draw simple text.
 *
 * Fonts can be drawn using the cer::draw_string() function.
 *
 * @ingroup Graphics
 */
class Font final
{
    CERLIB_DECLARE_OBJECT(Font);

  public:
    /**
     * Lazily loads a Font object from the storage.
     *
     * @param asset_name The name of the font in the asset storage.
     *
     * @throw std::runtime_error If the asset does not exist or could not be read or
     * loaded.
     */
    explicit Font(std::string_view asset_name);

    /**
     * Loads a font from memory.
     *
     * @param data he font data to load.
     */
    explicit Font(std::span<const std::byte> data);

    /**
     * Gets a reference to cerlib's built-in font.
     *
     * @param bold If true, returns the bold version of the built-in font.
     */
    static auto built_in(bool bold = false) -> Font;

    /**
     * Measures the size of a text when it would be drawn using the font at a
     * specific size.
     *
     * @param text The text to measure.
     * @param size The font size, in pixels.
     */
    auto measure(std::string_view text, uint32_t size) const -> Vector2;

    /**
     * Gets the uniform height of a line in the font at a specific size.
     *
     * @param size The font size, in pixels.
     */
    auto line_height(uint32_t size) const -> float;

    /**
     * Performs an action for each glyph in a specific text.
     *
     * @param text The text to iterate.
     * @param size The size of the font, in pixels.
     * @param action The action to perform for each glyph.
     */
    void for_each_glyph(
        std::string_view                                               text,
        uint32_t                                                       size,
        const std::function<bool(uint32_t codepoint, Rectangle rect)>& action) const;
};
} // namespace cer
