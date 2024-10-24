// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "cerlib/Drawing.hpp"
#include "FontImpl.hpp"
#include "GraphicsDevice.hpp"
#include "cerlib/Font.hpp"
#include "cerlib/Image.hpp"
#include "cerlib/Logging.hpp"
#include "game/GameImpl.hpp"
#include "stb_image_write.hpp"
#include "util/narrow_cast.hpp"
#include <cassert>

#define DECLARE_THIS_IMPL_CANVAS                                                                   \
    const auto impl = static_cast<details::CanvasImpl*>(impl());                                   \
    VERIFY_IMPL_ACCESS

void cer::set_scissor_rects(std::span<const Rectangle> scissor_rects)
{
    LOAD_DEVICE_IMPL;
    device_impl.set_scissor_rects(scissor_rects);
}

auto cer::current_canvas() -> Image
{
    LOAD_DEVICE_IMPL;
    return device_impl.current_canvas();
}

void cer::set_canvas(const Image& canvas)
{
    if (canvas && !canvas.is_canvas())
    {
        throw std::invalid_argument{"The specified image is not a canvas."};
    }

    LOAD_DEVICE_IMPL;
    device_impl.set_canvas(canvas, false);
}

void cer::set_transformation(const Matrix& transformation)
{
    LOAD_DEVICE_IMPL;
    device_impl.set_transformation(transformation);
}

auto cer::current_sprite_shader() -> Shader
{
    LOAD_DEVICE_IMPL;
    return device_impl.current_sprite_shader();
}

auto cer::set_sprite_shader(const Shader& shader) -> void
{
    LOAD_DEVICE_IMPL;
    device_impl.set_sprite_shader(shader);
}

void cer::set_sampler(const Sampler& sampler)
{
    LOAD_DEVICE_IMPL;
    device_impl.set_sampler(sampler);
}

void cer::set_blend_state(const BlendState& blend_state)
{
    LOAD_DEVICE_IMPL;
    device_impl.set_blend_state(blend_state);
}

void cer::draw_sprite(const Image& image, Vector2 position, Color color)
{
    if (!image)
    {
        return;
    }

    LOAD_DEVICE_IMPL;

    device_impl.draw_sprite(Sprite{
        .image    = image,
        .dst_rect = {position, image.size()},
        .color    = color,
    });
}

void cer::draw_sprite(const Sprite& sprite)
{
    if (!sprite.image)
    {
        return;
    }

    LOAD_DEVICE_IMPL;
    device_impl.draw_sprite(sprite);
}

void cer::draw_string(std::string_view              text,
                      const Font&                   font,
                      uint32_t                      font_size,
                      Vector2                       position,
                      Color                         color,
                      const Option<TextDecoration>& decoration)
{
    LOAD_DEVICE_IMPL;
    device_impl.draw_string(text, font, font_size, position, color, decoration);
}

void cer::draw_text(const Text& text, Vector2 position, const Color& color)
{
    LOAD_DEVICE_IMPL;
    device_impl.draw_text(text, position, color);
}

void cer::fill_rectangle(Rectangle rectangle, Color color, float rotation, Vector2 origin)
{
    LOAD_DEVICE_IMPL;
    device_impl.fill_rectangle(rectangle, color, rotation, origin);
}

void cer::draw_particles(const ParticleSystem& particle_system)
{
    LOAD_DEVICE_IMPL;
    device_impl.draw_particles(particle_system);
}

auto cer::frame_stats() -> FrameStats
{
    LOAD_DEVICE_IMPL;
    return device_impl.frame_stats_ref();
}

auto cer::current_canvas_size() -> Vector2
{
    LOAD_DEVICE_IMPL;
    return device_impl.current_canvas_size();
}

void cer::read_canvas_data_into(
    const Image& canvas, uint32_t x, uint32_t y, uint32_t width, uint32_t height, void* destination)
{
    if (!canvas)
    {
        throw std::invalid_argument{"No canvas specified."};
    }

    if (!canvas.is_canvas())
    {
        throw std::invalid_argument{"The specified image does not represent a canvas."};
    }

    if (canvas == current_canvas())
    {
        throw std::logic_error{"The specified canvas is currently being drawn to. Please "
                               "unset it first before reading from it."};
    }

    const auto canvas_width  = canvas.width();
    const auto canvas_height = canvas.height();

    if (x + width > canvas_width)
    {
        throw std::invalid_argument{
            fmt::format("The specified x-coordinate ({}) and width ({}) would exceed "
                        "the canvas bounds ({})",
                        x,
                        width,
                        canvas_width)};
    }

    if (y + height > canvas_height)
    {
        throw std::invalid_argument{
            fmt::format("The specified y-coordinate ({}) and height ({}) would exceed "
                        "the canvas bounds ({})",
                        y,
                        height,
                        canvas_height)};
    }

    LOAD_DEVICE_IMPL;

    device_impl.read_canvas_data_into(canvas, x, y, width, height, destination);
}

auto cer::read_canvas_data(
    const Image& canvas, uint32_t x, uint32_t y, uint32_t width, uint32_t height) -> List<std::byte>
{
    if (!canvas)
    {
        throw std::invalid_argument{"No canvas specified."};
    }

    if (!canvas.is_canvas())
    {
        throw std::invalid_argument{"The specified image does not represent a canvas."};
    }

    const auto size_in_bytes = image_slice_pitch(width, height, canvas.format());

    if (size_in_bytes == 0)
    {
        throw std::invalid_argument{
            "Invalid canvas specified; failed to determine pixel data size"};
    }

    auto data = List<std::byte>{size_in_bytes};
    read_canvas_data_into(canvas, x, y, width, height, data.data());

    return data;
}

void cer::save_canvas_to_file(const Image&     canvas,
                              std::string_view filename,
                              ImageFileFormat  format)
{
    if (!canvas)
    {
        throw std::invalid_argument{"No canvas specified."};
    }

    if (!canvas.is_canvas())
    {
        throw std::invalid_argument{"The specified image does not represent a canvas."};
    }

    const auto canvas_width  = canvas.width();
    const auto canvas_height = canvas.height();
    const auto pixel_data    = read_canvas_data(canvas, 0, 0, canvas_width, canvas_height);
    const auto row_pitch     = image_row_pitch(canvas_width, canvas.format());
    const auto filename_str  = String{filename};

    const int result = [&] {
        switch (format)
        {
            case ImageFileFormat::Png:
                return stbi_write_png(filename_str.c_str(),
                                      int(canvas_width),
                                      int(canvas_height),
                                      4,
                                      pixel_data.data(),
                                      int(row_pitch));
            case ImageFileFormat::Jpeg:
                return stbi_write_jpg(filename_str.c_str(),
                                      int(canvas_width),
                                      int(canvas_height),
                                      4,
                                      pixel_data.data(),
                                      90);
            case ImageFileFormat::Bmp:
                return stbi_write_bmp(filename_str.c_str(),
                                      int(canvas_width),
                                      int(canvas_height),
                                      4,
                                      pixel_data.data());
        }

        return 0;
    }();

    if (result == 0)
    {
        throw std::runtime_error{"Failed to save the canvas data."};
    }
}

auto cer::save_canvas_to_memory(const Image& canvas, ImageFileFormat format) -> List<std::byte>
{
    if (!canvas)
    {
        throw std::invalid_argument{"No canvas specified."};
    }

    if (!canvas.is_canvas())
    {
        throw std::invalid_argument{"The specified image does not represent a canvas."};
    }

    const auto canvas_width  = canvas.width();
    const auto canvas_height = canvas.height();
    const auto pixel_data    = read_canvas_data(canvas, 0, 0, canvas_width, canvas_height);
    const auto row_pitch     = image_row_pitch(canvas_width, canvas.format());

    struct Context
    {
        List<std::byte> saved_data;
    };

    const auto write_func = [](void* context, void* data, int size) {
        auto*      context_t = static_cast<Context*>(context);
        const auto span      = std::span{static_cast<const std::byte*>(data), narrow<size_t>(size)};

        context_t->saved_data.insert(context_t->saved_data.end(), span.begin(), span.end());
    };

    Context my_context{};

    const int result = [&] {
        switch (format)
        {
            case ImageFileFormat::Png:
                return stbi_write_png_to_func(write_func,
                                              &my_context,
                                              int(canvas_width),
                                              int(canvas_height),
                                              4,
                                              pixel_data.data(),
                                              int(row_pitch));
            case ImageFileFormat::Jpeg:
                return stbi_write_jpg_to_func(write_func,
                                              &my_context,
                                              int(canvas_width),
                                              int(canvas_height),
                                              4,
                                              pixel_data.data(),
                                              90);
            case ImageFileFormat::Bmp:
                return stbi_write_bmp_to_func(write_func,
                                              &my_context,
                                              int(canvas_width),
                                              int(canvas_height),
                                              4,
                                              pixel_data.data());
        }

        return 0;
    }();

    if (result == 0)
    {
        throw std::runtime_error{"Failed to save the canvas data."};
    }

    return my_context.saved_data;
}
