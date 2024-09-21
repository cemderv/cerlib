// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "OpenGLGraphicsDevice.hpp"
#include "OpenGLImage.hpp"
#include "OpenGLSpriteBatch.hpp"
#include "OpenGLUserShader.hpp"
#include "OpenGLWindow.hpp"
#include "cerlib/Game.hpp"
#include "cerlib/Logging.hpp"
#include <array>
#include <cassert>
#include <cstring>

namespace cer::details
{
#ifdef USE_OPENGL_DEBUGGING
static void OpenGlDebugMessageCallback(GLenum        source,
                                       GLenum        type,
                                       GLuint        id,
                                       GLenum        severity,
                                       GLsizei       length,
                                       const GLchar* message,
                                       const void*   user_param)
{
    if (type == GL_DEBUG_TYPE_ERROR_ARB)
    {
        CER_THROW_RUNTIME_ERROR("Internal OpenGL error: {}", message);
    }
    else if (type == GL_DEBUG_TYPE_PERFORMANCE_ARB)
    {
        log_debug("OpenGL performance warning: {}", message);
    }
    else if (type == GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB)
    {
        log_debug("OpenGL undefined behavior warning: {}", message);
    }
    else if (type == GL_DEBUG_TYPE_PORTABILITY_ARB)
    {
        log_debug("OpenGL portability warning: {}", message);
    }

    if (severity >= GL_DEBUG_SEVERITY_LOW_ARB)
    {
        log_debug("OpenGL low severity message: {}", message);
    }
}
#endif

void OpenGLGraphicsDevice::on_start_frame(const Window& window)
{
    auto opengl_window = dynamic_cast<OpenGLWindow*>(window.impl());
    assert(opengl_window != nullptr);

    opengl_window->make_context_current();

    {
        auto it = m_per_open_gl_context_states.find(opengl_window);

        if (it == m_per_open_gl_context_states.cend())
        {
            it = m_per_open_gl_context_states.emplace(opengl_window, PerOpenGLContextState()).first;
        }

        m_open_gl_context_state = &it->second;
    }

#ifndef __EMSCRIPTEN__
    const auto sync_interval = int(opengl_window->sync_interval());

    if (m_open_gl_context_state->last_applied_gl_swap_interval != sync_interval)
    {
        SDL_GL_SetSwapInterval(sync_interval);
        m_open_gl_context_state->last_applied_gl_swap_interval = sync_interval;
    }
#endif

    if (const auto canvas = current_canvas())
    {
        const auto opengl_canvas = dynamic_cast<OpenGLImage*>(canvas.impl());
        GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, opengl_canvas->gl_framebuffer_handle));
    }
    else
    {
        GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, 0));
    }
}

void OpenGLGraphicsDevice::on_end_frame(const Window& window)
{
    const auto sdl_window = window.impl()->sdl_window();
    SDL_GL_SwapWindow(sdl_window);
}

void OpenGLGraphicsDevice::on_set_canvas(const Image& canvas, const Rectangle& viewport)
{
    if (canvas)
    {
        const auto opengl_canvas = dynamic_cast<OpenGLImage*>(canvas.impl());
        GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, opengl_canvas->gl_framebuffer_handle));
    }
    else
    {
        GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, 0));
    }

    GL_CALL(glViewport(static_cast<GLint>(viewport.x),
                       static_cast<GLint>(viewport.y),
                       static_cast<GLsizei>(viewport.width),
                       static_cast<GLsizei>(viewport.height)));

    if (const std::optional<Color> clearColor =
            canvas ? canvas.canvas_clear_color() : current_window().clear_color())
    {
        auto previous_mask = std::array<GLint, 4>();
        GL_CALL(glGetIntegerv(GL_COLOR_WRITEMASK, previous_mask.data()));

        auto has_color_write_mask_changed = false;
        if (previous_mask[0] == 0 || previous_mask[1] == 0 || previous_mask[2] == 0 ||
            previous_mask[3] == 0)
        {
            GL_CALL(glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE));
            has_color_write_mask_changed = true;
        }

        const Color color = *clearColor;
        GL_CALL(glClearColor(color.r, color.g, color.b, color.a));
        GL_CALL(glClear(GL_COLOR_BUFFER_BIT));

        if (has_color_write_mask_changed)
        {
            GL_CALL(glColorMask(static_cast<GLboolean>(previous_mask[0]),
                                static_cast<GLboolean>(previous_mask[1]),
                                static_cast<GLboolean>(previous_mask[2]),
                                static_cast<GLboolean>(previous_mask[3])));
        }
    }
}

void OpenGLGraphicsDevice::on_set_scissor_rects(std::span<const Rectangle> scissor_rects)
{
    // TODO: check extensions (glScissorArrayNV etc)
#ifdef CERLIB_GFX_IS_GLES
    if (scissor_rects.size() > 1)
    {
        CER_THROW_INVALID_ARG(
            "{} scissor rects were specified, but the current system only supports 1",
            scissor_rects.size());
    }
#endif

    SmallVector<GLint, 8> scissor_rects_gl{};
    scissor_rects_gl.reserve(scissor_rects.size());

    for (const Rectangle& rect : scissor_rects)
    {
        scissor_rects_gl.emplace_back(static_cast<GLint>(rect.left()));
        scissor_rects_gl.emplace_back(static_cast<GLint>(rect.bottom()));
        scissor_rects_gl.emplace_back(static_cast<GLint>(rect.width));
        scissor_rects_gl.emplace_back(static_cast<GLint>(rect.height));
    }

    if (scissor_rects.empty())
    {
        GL_CALL(glDisable(GL_SCISSOR_TEST));
        return;
    }

    GL_CALL(glEnable(GL_SCISSOR_TEST));

#ifdef CERLIB_GFX_IS_GLES
    GL_CALL(glScissor(scissor_rects_gl[0],
                      scissor_rects_gl[1],
                      static_cast<GLsizei>(scissor_rects_gl[2]),
                      static_cast<GLsizei>(scissor_rects_gl[3])));
#else
    GL_CALL(
        glScissorArrayv(0, static_cast<GLsizei>(scissor_rects.size()), scissor_rects_gl.data()));
#endif
}

gsl::not_null<ImageImpl*> OpenGLGraphicsDevice::create_canvas(const Window& window,
                                                              uint32_t      width,
                                                              uint32_t      height,
                                                              ImageFormat   format)
{
    return std::make_unique<OpenGLImage>(this, window.impl(), width, height, format).release();
}

gsl::not_null<ImageImpl*> OpenGLGraphicsDevice::create_image(
    uint32_t                   width,
    uint32_t                   height,
    ImageFormat                format,
    uint32_t                   mipmap_count,
    const Image::DataCallback& data_callback)
{
    return std::make_unique<OpenGLImage>(this, width, height, format, mipmap_count, data_callback)
        .release();
}

const OpenGLFeatures& OpenGLGraphicsDevice::opengl_features() const
{
    return m_features;
}

void OpenGLGraphicsDevice::bind_vao(const OpenGLVao& vao)
{
    GL_CALL(glBindVertexArray(vao.gl_handle));
}

void OpenGLGraphicsDevice::use_program(GLuint program)
{
    if (program != m_open_gl_context_state->last_used_shader_program)
    {
        glUseProgram(program);
        m_open_gl_context_state->last_used_shader_program = program;
    }
}

void OpenGLGraphicsDevice::read_canvas_data_into(
    const Image& canvas, uint32_t x, uint32_t y, uint32_t width, uint32_t height, void* destination)
{
    assert(canvas);

    const auto opengl_image = dynamic_cast<OpenGLImage*>(canvas.impl());
    assert(opengl_image != nullptr);

    GLuint previously_bound_fbo{};
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, reinterpret_cast<GLint*>(&previously_bound_fbo));

    const GLuint fbo_handle = opengl_image->gl_framebuffer_handle;

    // A canvas cannot be bound while we're trying to read from it.
    // This is ensured by the top-lvel GetCanvasDataInto() function (via exception).
    assert(previously_bound_fbo != fbo_handle);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo_handle);

    const OpenGLFormatTriplet opengl_format_triplet = opengl_image->gl_format_triplet;
    const ImageFormat         canvas_format         = canvas.format();
    const auto                row_pitch             = image_row_pitch(width, canvas_format);
    const auto                pixel_data_size       = row_pitch * height;
    const auto                tmp_buffer = std::make_unique<std::byte[]>(pixel_data_size);

    glReadPixels(static_cast<GLint>(x),
                 static_cast<GLint>(y),
                 static_cast<GLsizei>(width),
                 static_cast<GLsizei>(height),
                 opengl_format_triplet.base_format,
                 opengl_format_triplet.type,
                 tmp_buffer.get());

    // Flip data vertically because OpenGL.
    auto dst_row = static_cast<std::byte*>(destination);

    for (uint32_t row = 0; row < height; ++row)
    {
        const auto src_row = tmp_buffer.get() + row_pitch * (height - row - 1);
        std::memcpy(dst_row, src_row, row_pitch);
        dst_row += row_pitch;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, previously_bound_fbo);
}

std::unique_ptr<ShaderImpl> OpenGLGraphicsDevice::create_native_user_shader(
    std::string_view native_code, ShaderImpl::ParameterList parameters)
{
    return std::make_unique<OpenGLUserShader>(this, native_code, std::move(parameters));
}

OpenGLGraphicsDevice::OpenGLGraphicsDevice(WindowImpl& main_window)
    : GraphicsDevice(main_window)
{
    OpenGLWindow& opengl_window = dynamic_cast<OpenGLWindow&>(main_window);
    opengl_window.make_context_current();

    // Load OpenGL function pointers
    {
        const auto get_proc = reinterpret_cast<GLADloadproc>(SDL_GL_GetProcAddress);

        int gl_loading_success =
#ifdef CERLIB_GFX_IS_GLES
            gladLoadGLES2Loader(get_proc);
#else
            gladLoadGLLoader(get_proc);
#endif

        if (gl_loading_success == 0)
        {
            throw std::runtime_error{"Failed to load OpenGL functions."};
        }
    }

    // Verify clean OpenGL state
    verify_opengl_state();

#if defined(GL_MAJOR_VERSION) && defined(GL_MINOR_VERSION)
    // Verify required OpenGL version
    GLint gl_major_version{};
    GLint gl_minor_version{};
    GL_CALL(glGetIntegerv(GL_MAJOR_VERSION, &gl_major_version));
    GL_CALL(glGetIntegerv(GL_MINOR_VERSION, &gl_minor_version));
#else
    const auto gl_major_version = GLVersion.major;
    const auto gl_minor_version = GLVersion.minor;
#endif

    if (compare_opengl_version_to_min_required_version(gl_major_version, gl_minor_version) < 0)
    {
        CER_THROW_RUNTIME_ERROR(
            "The system does not support the minimum required OpenGL version ({}.{}). The "
            "current OpenGL version of the system is {}.{}.",
            min_required_gl_major_version,
            min_required_gl_minor_version,
            gl_major_version,
            gl_minor_version);
    }

    // Log OpenGL information
#if !defined(NDEBUG) && defined(CERLIB_ENABLE_VERBOSE_LOGGING)
    {
        const auto renderer_name =
            std::string_view(reinterpret_cast<const char*>(glGetString(GL_RENDERER)));

        log_verbose("Initialized OpenGL Device");
        log_verbose("  OpenGL version: {}.{}", gl_major_version, gl_minor_version);
        log_verbose("  OpenGL renderer: {}", renderer_name);
    }
#endif

#ifdef USE_OPENGL_DEBUGGING
    // Hook to OpenGL debug log
    if (GLAD_GL_ARB_debug_output != 0 && glDebugMessageCallbackARB)
    {
        log_verbose("  Device supports GL_debug_output; enabling it");
        GL_CALL(glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB));
        GL_CALL(glDebugMessageCallbackARB(OpenGlDebugMessageCallback, this));
    }
#endif

    m_features.flush_buffer_range = [] {
        if (glFlushMappedBufferRange)
        {
            return true;
        }

#ifdef __APPLE__
        if (glFlushMappedBufferRangeAPPLE)
        {
            return true;
        }
#endif

        return false;
    }();

    if (m_features.flush_buffer_range)
    {
        log_verbose("  Device supports OpenGL feature FlushBufferRange");
    }

    // TODO: check glBufferStorageExt

#ifndef CERLIB_GFX_IS_GLES
    if (GLAD_GL_ARB_buffer_storage != 0 && glBufferStorage)
    {
        log_verbose("  Device supports OpenGL feature BufferStorage");
        m_features.buffer_storage = true;
    }

    if ((GLAD_GL_ARB_texture_storage != 0 || GLAD_GL_EXT_texture_storage != 0) &&
        glTexStorage2D != nullptr)
    {
        log_verbose("  Device supports OpenGL feature TextureStorage");
        m_features.texture_storage = true;
    }

    if (GLAD_GL_ARB_bindless_texture != 0 && glCreateTextures)
    {
        log_verbose("  Device supports OpenGL feature BindlessTextures");
        m_features.bindless_textures = true;
    }
#endif

    log_verbose("Initialized OpenGL device. Now calling PostInit().");

    post_init(std::make_unique<OpenGLSpriteBatch>(this, frame_stats_ptr()));
}

OpenGLGraphicsDevice::~OpenGLGraphicsDevice() noexcept = default;
} // namespace cer::details
