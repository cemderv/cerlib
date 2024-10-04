// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "OpenGLPrerequisites.hpp"
#include "cerlib/Image.hpp"
#include "util/InternalError.hpp"

void cer::details::verify_opengl_state_x()
{
    if (auto error = glGetError(); error != GL_NO_ERROR)
    {
        auto error_string = std::string{};

        while (error != GL_NO_ERROR)
        {
            error_string += std::to_string(error);
            error_string += ';';
            error = glGetError();
        }

        error_string.pop_back();

        CER_THROW_RUNTIME_ERROR("OpenGL error(s) occurred: {}", error_string);
    }
}

auto cer::details::convert_to_opengl_pixel_format(ImageFormat format) -> OpenGLFormatTriplet
{
#ifdef GL_RGBA8
    constexpr auto rgba8 = GL_RGBA8;
#else
    constexpr auto rgba8 = GL_RGBA8_OES;
#endif

#ifdef GL_SRGB8_ALPHA8
    constexpr auto srgb = GL_SRGB8_ALPHA8;
#else
    constexpr auto srgb = GL_SRGB8_ALPHA8_EXT;
#endif

#ifdef GL_R8
    constexpr auto r8 = GL_R8;
#else
    constexpr auto r8 = GL_R8_EXT;
#endif

#ifdef GL_RED
    constexpr auto red_gl = GL_RED;
#else
    constexpr auto red_gl = GL_RED_EXT;
#endif

    switch (format)
    {
        case ImageFormat::R8G8B8A8_UNorm:
            return OpenGLFormatTriplet{
                .internal_format = rgba8,
                .base_format     = GL_RGBA,
                .type            = GL_UNSIGNED_BYTE,
            };
        case ImageFormat::R8G8B8A8_Srgb:
            return OpenGLFormatTriplet{
                .internal_format = srgb,
                .base_format     = GL_RGBA,
                .type            = GL_UNSIGNED_BYTE,
            }; // glEnable(GL_FRAMEBUFFER_SRGB) necessary

        case ImageFormat::R8_UNorm:
            return OpenGLFormatTriplet{
                .internal_format = r8,
                .base_format     = red_gl,
                .type            = GL_UNSIGNED_BYTE,
            };

        default: CER_THROW_INTERNAL_ERROR("Unsupported texture format {}", int(format));
    }
}

auto cer::details::compare_opengl_version_to_min_required_version(int major, int minor) -> int
{
    const auto compare = [](int lhs, int rhs) {
        return lhs < rhs ? -1 : lhs > rhs ? 1 : 0;
    };

    constexpr auto rhs_major = min_required_gl_major_version;
    constexpr auto rhs_minor = min_required_gl_minor_version;

    if (major != rhs_major)
    {
        return compare(major, rhs_major);
    }

    if (minor != rhs_minor)
    {
        return compare(minor, rhs_minor);
    }

    return 0;
}