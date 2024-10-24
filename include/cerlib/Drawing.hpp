// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <cerlib/Image.hpp>
#include <cerlib/List.hpp>
#include <cerlib/Matrix.hpp>
#include <cerlib/Rectangle.hpp>
#include <cerlib/Vector2.hpp>
#include <optional>
#include <variant>

namespace cer
{
class Font;
class Shader;
class Text;
class ParticleSystem;
struct BlendState;
struct Sampler;

/**
 * Defines various flip factors for 2D sprites that are drawn using draw_sprite().
 *
 * @ingroup Graphics
 */
enum class SpriteFlip
{
    /**
     * The sprite is drawn normally, without any flipping.
     */
    None = 0,

    /**
     * The sprite is flipped horizontally around its center.
     */
    Horizontally = 1,

    /**
     * The sprite is flipped vertically around its center.
     */
    Vertically = 2,

    /**
     * The sprite is flipped both horizontally and vertically around its center.
     */
    Both = Horizontally | Vertically,
};

static auto operator|(SpriteFlip lhs, SpriteFlip rhs) -> SpriteFlip
{
    return SpriteFlip(int(lhs) | int(rhs));
}

static auto operator|=(SpriteFlip& lhs, SpriteFlip rhs) -> SpriteFlip&
{
    lhs = lhs | rhs;
    return lhs;
}

/**
 * Represents a drawable 2D sprite.
 *
 * @ingroup Graphics
 */
struct Sprite
{
    /** Default comparison */
    auto operator==(const Sprite&) const -> bool = default;

    /** Default comparison */
    auto operator!=(const Sprite&) const -> bool = default;

    /** The image of the sprite. */
    Image image = {};

    /** The destination area of the sprite, in pixels. */
    Rectangle dst_rect = {};

    /** The image coordinates of the sprite, in pixels. */
    std::optional<Rectangle> src_rect = {};

    /** The multiplicative color of the sprite. */
    Color color = white;

    /** The rotation of the sprite, in radians. */
    float rotation = 0.0f;

    /** The top-left origin of the sprite, in pixels. */
    Vector2 origin = {};

    /** The scaling factor of the sprite. */
    Vector2 scale = {1.0f, 1.0f};

    /** Flip flags of the sprite. */
    SpriteFlip flip = SpriteFlip::None;
};

/**
 * Represents options to draw text together with an underline.
 *
 * @ingroup Graphics
 */
struct TextUnderline
{
    /** The optional thickness of the line. If not specified, an ideal thickness is
     * calculated. */
    std::optional<float> thickness;

    /** The optional color of the line. If not specified, the text color is used. */
    std::optional<Color> color;
};

/**
 * Represents options to draw text together with a strikethrough line.
 *
 * @ingroup Graphics
 */
struct TextStrikethrough
{
    /** The optional thickness of the line. If not specified, an ideal thickness is
     * calculated. */
    std::optional<float> thickness;

    /** The optional color of the line. If not specified, the text color is used. */
    std::optional<Color> color;
};

/**
 * Defines various styles for 2D text objects that are drawn using draw_string() and draw_text().
 *
 * @ingroup Graphics
 */
using TextDecoration = std::variant<TextUnderline, TextStrikethrough>;

/**
 * Represents drawing statistics of a frame.
 *
 * @ingroup Graphics
 */
struct FrameStats
{
    /** The number of draw calls that were performed in total. */
    uint32_t draw_calls = 0;
};

/**
 * Sets the active set of scissor rectangles.
 *
 * @param scissor_rects The scissor rectangles to set for subsequent drawing.
 *
 * @ingroup Graphics
 */
void set_scissor_rects(std::span<const Rectangle> scissor_rects);

/**
 * Gets the currently bound canvas.
 *
 * @ingroup Graphics
 */
auto current_canvas() -> Image;

/**
 * Sets the active canvas to use as a rendering destination.
 *
 * @param canvas The canvas to draw into in subsequent calls. To render to the current
 * window, set an empty canvas object.
 *
 * Example:
 * @code{.cpp}
 * cer::Image canvas = {256, 256, ImageFormat::R8G8B8A8_UNorm};
 *
 * cer::set_canvas(canvas);                 // Set canvas as active.
 * cer::draw_sprite(my_sprite, {0, 0});     // Draws the sprite into the canvas.
 *
 * cer::set_canvas({});                     // Set current window as render target.
 * cer::draw_sprite(canvas, {0, 0});        // Draws the canvas into the window.
 * cer::draw_sprite(my_sprite, {100, 100}); // Draws the sprite on top.
 * @endcode
 *
 * @ingroup Graphics
 */
void set_canvas(const Image& canvas);

/**
 * Sets the transformation to apply to all subsequent 2D objects.
 *
 * @param transformation The transformation to use for subsequent drawing.
 *
 * @ingroup Graphics
 */
void set_transformation(const Matrix& transformation);

/**
 * Gets the currently set sprite shader.
 *
 * @ingroup Graphics
 */
auto current_sprite_shader() -> Shader;

/**
 * Sets the active custom shader to use for sprite rendering.
 * To deactivate custom sprite shading, set an empty shader object.
 *
 * @param shader The sprite shader to use for subsequent drawing.
 *
 * @ingroup Graphics
 */
void set_sprite_shader(const Shader& shader);

/**
 * Sets the image sampler to use for sprite rendering.
 * The default sampler is Sampler::LinearClamp().
 *
 * @param sampler The sampler to use for subsequent drawing.
 *
 * @ingroup Graphics
 */
void set_sampler(const Sampler& sampler);

/**
 * Sets the blend state to use for sprite rendering.
 * The default blend state is BlendState::NonPremultiplied();
 *
 * @param blend_state The blend state to use for subsequent drawing.
 *
 * @ingroup Graphics
 */
void set_blend_state(const BlendState& blend_state);

/**
 * Draws a 2D sprite.
 *
 * @remark This is a shortcut for draw_sprite(const Sprite&).
 *
 * @param image The image of the sprite.
 * @param position The position of the sprite.
 * @param color The color of the sprite.
 *
 * @ingroup Graphics
 */
void draw_sprite(const Image& image, Vector2 position, Color color = white);

/**
 * Draws a 2D sprite.
 *
 * @param sprite The sprite to draw.
 *
 * @ingroup Graphics
 */
void draw_sprite(const Sprite& sprite);

/**
 * Draws 2D text.
 *
 * @param text The text to draw.
 * @param font The font to draw the text with.
 * @param font_size The size of the font to use, in pixels.
 * @param position The top-left position of the text.
 * @param color The color of the text.
 * @param decoration The text decorations.
 *
 * @ingroup Graphics
 */
void draw_string(std::string_view                     text,
                 const Font&                          font,
                 uint32_t                             font_size,
                 Vector2                              position,
                 Color                                color      = white,
                 const std::optional<TextDecoration>& decoration = std::nullopt);

/**
 * Draws 2D text from a pre-created Text object.
 *
 * @param text The text object to draw.
 * @param position The top-left position of the text.
 * @param color The color of the text.
 */
void draw_text(const Text& text, Vector2 position, const Color& color = white);

/**
 * Draws a filled solid color rectangle.
 *
 * @param rectangle The rectangle to draw.
 * @param color The color of the rectangle.
 * @param rotation The rotation of the rectangle, in degrees.
 * @param origin The origin of the sprite.
 *
 * @ingroup Graphics
 */
void fill_rectangle(Rectangle rectangle,
                    Color     color    = white,
                    float     rotation = 0.0f,
                    Vector2   origin   = Vector2());

/**
 * Draws a 2D particle system.
 *
 * @param particle_system The particle system to draw
 */
void draw_particles(const ParticleSystem& particle_system);

/**
 * Gets statistics about the previous frame.
 *
 * @ingroup Graphics
 */
auto frame_stats() -> FrameStats;

/**
 * Gets the size of the current canvas, in pixels.
 *
 * If no canvas is set, the size of the current window is returned, in pixels.
 *
 * @ingroup Graphics
 */
auto current_canvas_size() -> Vector2;

/**
 * Gets the pixel data that is currently stored in a canvas.
 *
 * The data is written directly to a user-specified data pointer.
 * The caller therefore has to ensure that the destination buffer is large enough
 * to store the data of the canvas.
 *
 * @remark A convenience version of this function is available called
 * cer::read_canvas_data().
 *
 * @param canvas The canvas image to read data from.
 * @param x The x-coordinate within the canvas to start reading from.
 * @param y The y-coordinate within the canvas to start reading from.
 * @param width The width of the area within the canvas to read, in pixels.
 * @param height The height of the area within the canvas to read, in pixels.
 * @param destination A pointer to the buffer that receives the canvas data.
 *
 * @ingroup Graphics
 */
void read_canvas_data_into(const Image& canvas,
                           uint32_t     x,
                           uint32_t     y,
                           uint32_t     width,
                           uint32_t     height,
                           void*        destination);

/**
 * Gets the pixel data that is currently stored in a canvas.
 *
 * @remark This is a convenience version of the cer::read_canvas_data_into() function.
 *
 * @param canvas The canvas image to read data from.
 * @param x The x-coordinate within the canvas to start reading from.
 * @param y The y-coordinate within the canvas to start reading from.
 * @param width The width of the area within the canvas to read, in pixels.
 * @param height The height of the area within the canvas to read, in pixels.
 *
 * @return A buffer that contains the pixel data of the canvas.
 *
 * @ingroup Graphics
 */
auto read_canvas_data(const Image& canvas, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
    -> List<std::byte>;

/**
 * Saves the pixel data of a canvas to a file.
 *
 * @param canvas The canvas image to save to a file.
 * @param filename The destination filename.
 * @param format The format to which to convert and save the canvas data.
 *
 * @ingroup Graphics
 */
void save_canvas_to_file(const Image&     canvas,
                         std::string_view filename,
                         ImageFileFormat  format = ImageFileFormat::Png);

/**
 * Saves the pixel data of a canvas to a buffer in memory.
 *
 * @param canvas The canvas image to save to a buffer.
 * @param format The format to which to convert and save the canvas data.
 *
 * @return The converted pixel data of the canvas.
 *
 * @ingroup Graphics
 */
auto save_canvas_to_memory(const Image& canvas, ImageFileFormat format = ImageFileFormat::Png)
    -> List<std::byte>;
} // namespace cer
