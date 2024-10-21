// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <glad.h>

namespace cer
{
enum class ImageFormat;
}

namespace cer::details
{
#ifdef CERLIB_GFX_IS_GLES
constexpr int min_required_gl_major_version = 3;
constexpr int min_required_gl_minor_version = 0;
#else
constexpr int min_required_gl_major_version = 3;
constexpr int min_required_gl_minor_version = 0;
#endif

constexpr GLsizei shader_log_max_length = 256;

#if !defined(NDEBUG) && defined(GL_DEBUG_TYPE_ERROR_ARB)
#define USE_OPENGL_DEBUGGING
#endif

#ifdef USE_OPENGL_DEBUGGING
#define GL_CALL(expr)                                                                              \
    expr;                                                                                          \
    verify_opengl_state()
#else
#define GL_CALL(expr) expr
#endif

struct OpenGLFeatures
{
    bool flush_buffer_range{};
    bool buffer_storage{};
    bool texture_storage{};
    bool bindless_textures{};
};

struct OpenGLFormatTriplet
{
    GLint  internal_format{};
    GLenum base_format{};
    GLenum type{};
};

void verify_opengl_state_x();

#ifndef NDEBUG
static void verify_opengl_state()
{
    verify_opengl_state_x();
}
#else
static inline auto verify_opengl_state() -> void
{
}
#endif

auto convert_to_opengl_pixel_format(ImageFormat format) -> OpenGLFormatTriplet;

auto compare_opengl_version_to_min_required_version(int major, int minor) -> int;
} // namespace cer::details
