// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <cerlib/Color.hpp>
#include <cerlib/Export.hpp>
#include <cerlib/GraphicsResource.hpp>
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
class CERLIB_API Image : public GraphicsResource
{
    CERLIB_DECLARE_DERIVED_OBJECT(GraphicsResource, Image);

  public:
    /**
     * Represents a callback function that is used when constructing a image.
     * Images can be constructed by passing initial data to them directly or by passing a
     * function of this type. In that case, the image calls this function to retrieve data
     * for a specific array and mipmap index.
     *
     * @param mipmap The mipmap for which data is requested.
     *
     * @return The data for the specific array and mipmap index. The data must remain
     * valid throughout the image's construction. Afterwards, it may be freed; the image
     * does not own the data.
     */
    using DataCallback = std::function<const void*(uint32_t mipmap)>;

    /**
     * Creates a 2D image from raw data.
     *
     * @param width The width of the image, in pixels.
     * @param height The height of the image, in pixels.
     * @param format The pixel format of the image.
     * @param mipmap_count The number of mipmaps in the image.
     * @param data The initial data of the image.
     */
    explicit Image(uint32_t    width,
                   uint32_t    height,
                   ImageFormat format,
                   uint32_t    mipmap_count,
                   const void* data);

    /**
     * Creates a 2D image from raw data.
     *
     * @param width The width of the image, in pixels.
     * @param height The height of the image, in pixels.
     * @param format The pixel format of the image.
     * @param mipmap_count The number of mipmaps in the image.
     * @param data_callback The data function for the image.
     */
    explicit Image(uint32_t            width,
                   uint32_t            height,
                   ImageFormat         format,
                   uint32_t            mipmap_count,
                   const DataCallback& data_callback);

    /**
     * Loads a 2D image from memory.
     * Supported file formats are:
     *  - jpg, bmp, png, tga, gif, hdr, dds
     *
     * @param memory The data to load.
     * @param generate_mipmaps If true, mipmaps are automatically generated for the image.
     * If the image already had mipmaps, none are generated instead.
     */
    explicit Image(std::span<const std::byte> memory, bool generate_mipmaps);

    /**
     * Loads a 2D image from a file.
     * Supported file formats are:
     *  - jpg, bmp, png, tga, gif, hdr, dds
     *
     * @param filename The data to load.
     * @param generate_mipmaps If true, mipmaps are automatically generated for the image.
     * If the image already had mipmaps, none are generated instead.
     *
     * @attention This constructor is only available on desktop platforms.
     * On non-desktop platforms, the cer::LoadImage() function must be used to load
     * images. Calling this on non-desktop platforms will raise an error.
     */
    explicit Image(std::string_view filename, bool generate_mipmaps = false);

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
    bool is_canvas() const;

    /** Gets the width of the image, in pixels. */
    uint32_t width() const;

    /** Gets the height of the image, in pixels. */
    uint32_t height() const;

    /** Gets the width of the image, in pixels. */
    float widthf() const;

    /** Gets the height of the image, in pixels. */
    float heightf() const;

    /** Gets the size of the image as a 2D vector, in pixels. */
    Vector2 size() const;

    /** Gets the underlying pixel format of the image. */
    ImageFormat format() const;

    /** Gets the number of mipmaps in the image. */
    uint32_t mipmap_count() const;

    /** Gets the clear color of the image when it is set as a canvas. */
    std::optional<Color> canvas_clear_color() const;

    /** Sets the clear color of the image when it is set as a canvas. */
    void set_canvas_clear_color(std::optional<Color> value);

    /** Gets the size of the image's pixel data, in bytes. */
    uint32_t size_in_bytes() const;
};

/**
 * Gets the number of bits per pixel of a image format.
 *
 * @param format The format of which to get the number of bits per pixel.
 *
 * @ingroup Graphics
 */
CERLIB_API uint32_t image_format_bits_per_pixel(ImageFormat format);

/**
 * Gets the number of bytes in a row of a specific image format.
 *
 * @param width The row width, in pixels.
 * @param format The image format.
 *
 * @ingroup Graphics
 */
CERLIB_API uint32_t image_row_pitch(uint32_t width, ImageFormat format);

/**
 * Gets the number of bytes in a slice of a specific image format.
 *
 * @param width The width of the slice, in pixels.
 * @param height The height of the slice, in pixels.
 * @param format The image format.
 *
 * @ingroup Graphics
 */
CERLIB_API uint32_t image_slice_pitch(uint32_t width, uint32_t height, ImageFormat format);

/**
 * Gets the name of an image format.
 *
 * @param format The format whose name is to be returned.
 *
 * @ingroup Graphics
 */
CERLIB_API std::string_view image_format_name(ImageFormat format);
} // namespace cer
