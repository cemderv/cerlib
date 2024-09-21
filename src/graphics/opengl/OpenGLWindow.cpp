// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "OpenGLWindow.hpp"
#include "OpenGLPrerequisites.hpp"
#include "cerlib/Logging.hpp"
#include "game/GameImpl.hpp"
#include "util/InternalError.hpp"

namespace cer::details
{
OpenGLWindow::OpenGLWindow(std::string_view        title,
                           uint32_t                id,
                           std::optional<int32_t>  position_x,
                           std::optional<int32_t>  position_y,
                           std::optional<uint32_t> width,
                           std::optional<uint32_t> height,
                           bool                    allow_high_dpi)
    : WindowImpl(title, id, position_x, position_y, width, height, allow_high_dpi)
    , m_gl_context(nullptr)
{
#ifdef CERLIB_GFX_IS_GLES
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#endif

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, min_required_gl_major_version);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, min_required_gl_minor_version);

#ifdef USE_OPENGL_DEBUGGING
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);

    create_sdl_window(SDL_WINDOW_OPENGL);

    auto&      gameImpl = GameImpl::instance();
    const auto windows  = gameImpl.windows();
    if (windows.size() == 1 && windows[0] == this)
    {
        log_verbose("  This is the first window; not sharing OpenGL context");
        SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 0);
    }
    else
    {
        log_verbose("  This is an additional window; sharing with pre-existing OpenGL context");
        SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
    }

    log_verbose("  Creating OpenGL context");
    m_gl_context = SDL_GL_CreateContext(sdl_window());

    GameImpl::instance().ensure_graphics_device_initialized(*this);
}

OpenGLWindow::~OpenGLWindow() noexcept
{
    log_verbose("Destroying OpenGL window '{}'", title());

    if (m_gl_context)
    {
        log_verbose("  Destroying OpenGL context");
#ifdef __EMSCRIPTEN__
        SDL_GL_DeleteContext(m_gl_context);
#else
        SDL_GL_DestroyContext(m_gl_context);
#endif
        m_gl_context = nullptr;
    }
}

void OpenGLWindow::handle_resize_event()
{
    const auto [widthPx, heightPx] = size_px();

    if (m_resize_callback)
    {
        const auto [width, height] = size();
        m_resize_callback(uint32_t(width), uint32_t(height), uint32_t(widthPx), uint32_t(heightPx));
    }
}

void OpenGLWindow::make_context_current()
{
    SDL_GL_MakeCurrent(sdl_window(), m_gl_context);
}
} // namespace cer::details
