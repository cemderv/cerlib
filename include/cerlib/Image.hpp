// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <cerlib/Color.hpp>
#include <cerlib/GraphicsResource.hpp>
#include <cerlib/details/ObjectMacros.hpp>
#include <functional>
#include <optional>
#include <span>

namespace cer
{
struct Vector2;
class Window;

namespace details
{
class ImageImpl;
}

/**
 * Defines the pixel format of an image.
 *
 * @ingroup Graphics
 */
enum class ImageFormat
{
    /** Unsigned 8-bit red channel, normalized to `[0.0 .. 1.0]` */
    R8_UNorm = 1,

    /** Unsigned 32-bit RGBA, 8 bits per channel, normalized to `[0.0 .. 1.0]` */
    R8G8B8A8_UNorm = 2,

    /** 32-bit RGBA in sRGB space, 8 bits per channel */
    R8G8B8A8_Srgb = 3,

    /** 128-bit RGBA floating-point, 32 bits per channel */
    R32G32B32A32_Float = 4,
};

/**
 * Defines the format of an image when it is saved.
 *
 * @ingroup Graphics
 */
enum class ImageFileFormat
{
    /** A PNG file */
    Png = 1,

    /** A JPEG file */
    Jpeg = 2,

    /** A BMP file */
    Bmp = 3,
};

/**
 * Represents a 2D image.
 *
 * @ingroup Graphics
 */
class Image : public GraphicsResource
{
    CERLIB_DECLARE_DERIVED_OBJECT(GraphicsResource, Image);

  public:
    /**
     * Creates a 2D image from raw data.
     *
     * @param width The width of the image, in pixels.
     * @param height The height of the image, in pixels.
     * @param format The pixel format of the image.
     * @param data The initial data of the image.
     */
    explicit Image(uint32_t width, uint32_t height, ImageFormat format, const void* data);

    /**
     * Loads a 2D image from memory.
     * Supported file formats are:
     *  - jpg, bmp, png, tga, gif, hdr, dds
     *
     * @param memory The data to load.
     */
    explicit Image(std::span<const std::byte> memory);

    /**
     * Lazily loads an Image object from the storage.
     *
     * @param asset_name The name of the image in the asset storage.
     *
     * @throw std::runtime_error If the asset does not exist or could not be read or
     * loaded.
     */
    explicit Image(std::string_view asset_name);

    /**
     * Creates a 2D image to be used as a canvas.
     *
     * @param window The window in which the canvas is going to be used.
     * @param width The width of the image, in pixels.
     * @param height The height of the image, in pixels.
     * @param format The pixel format of the image.
     */
    explicit Image(uint32_t width, uint32_t height, ImageFormat format, const Window& window);

    /** Gets a value indicating whether the image is a canvas. */
    auto is_canvas() const -> bool;

    /** Gets the width of the image, in pixels. */
    auto width() const -> uint32_t;

    /** Gets the height of the image, in pixels. */
    auto height() const -> uint32_t;

    /** Gets the width of the image, in pixels. */
    auto widthf() const -> float;

    /** Gets the height of the image, in pixels. */
    auto heightf() const -> float;

    /** Gets the size of the image as a 2D vector, in pixels. */
    auto size() const -> Vector2;

    /** Gets the underlying pixel format of the image. */
    auto format() const -> ImageFormat;

    /** Gets the clear color of the image when it is set as a canvas. */
    auto canvas_clear_color() const -> std::optional<Color>;

    /** Sets the clear color of the image when it is set as a canvas. */
    void set_canvas_clear_color(std::optional<Color> value);

    /** Gets the size of the image's pixel data, in bytes. */
    auto size_in_bytes() const -> uint32_t;
};

/**
 * Gets the number of bits per pixel of a image format.
 *
 * @param format The format of which to get the number of bits per pixel.
 *
 * @ingroup Graphics
 */
auto image_format_bits_per_pixel(ImageFormat format) -> uint32_t;

/**
 * Gets the number of bytes in a row of a specific image format.
 *
 * @param width The row width, in pixels.
 * @param format The image format.
 *
 * @ingroup Graphics
 */
auto image_row_pitch(uint32_t width, ImageFormat format) -> uint32_t;

/**
 * Gets the number of bytes in a slice of a specific image format.
 *
 * @param width The width of the slice, in pixels.
 * @param height The height of the slice, in pixels.
 * @param format The image format.
 *
 * @ingroup Graphics
 */
auto image_slice_pitch(uint32_t width, uint32_t height, ImageFormat format) -> uint32_t;

/**
 * Gets the name of an image format.
 *
 * @param format The format whose name is to be returned.
 *
 * @ingroup Graphics
 */
auto image_format_name(ImageFormat format) -> std::string_view;
} // namespace cer
