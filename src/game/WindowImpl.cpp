// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "WindowImpl.hpp"

#include "cerlib/Logging.hpp"
#include "game/GameImpl.hpp"
#include "util/InternalError.hpp"
#include <gsl/narrow>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

namespace cer::details
{
static int get_sdl_window_flags(bool allow_high_dpi)
{
    int flags = 0;

#ifndef __EMSCRIPTEN__
    flags |= SDL_WINDOW_RESIZABLE;
#endif

    flags |= SDL_WINDOW_INPUT_FOCUS;

#if defined(__APPLE__)
#if TARGET_OS_IOS
    flags |= SDL_WINDOW_FULLSCREEN;
    flags |= SDL_WINDOW_BORDERLESS;
#endif
#endif

    if (allow_high_dpi)
    {
#ifdef __EMSCRIPTEN__
        flags |= SDL_WINDOW_ALLOW_HIGHDPI;
#else
        flags |= SDL_WINDOW_HIGH_PIXEL_DENSITY;
#endif
    }

    return flags;
}

static
#ifdef __EMSCRIPTEN__
    int
#else
    SDL_bool
#endif
    sdl_window_event_watcher(void* userdata, SDL_Event* event)
{
    WindowImpl*       window     = static_cast<WindowImpl*>(userdata);
    const SDL_Window* sdl_window = window->sdl_window();

#ifdef __EMSCRIPTEN__
    if (event->type == SDL_WINDOWEVENT && event->window.event == SDL_WINDOWEVENT_RESIZED)
    {
#else
    if (event->type == SDL_EVENT_WINDOW_RESIZED)
    {
#endif
        const auto win = SDL_GetWindowFromID(event->window.windowID);
        if (win == sdl_window)
            window->handle_resize_event();
    }

#ifdef __EMSCRIPTEN__
    return 0;
#else
    return SDL_FALSE;
#endif
}

WindowImpl::WindowImpl(std::string_view        title,
                       uint32_t                id,
                       std::optional<int32_t>  position_x,
                       std::optional<int32_t>  position_y,
                       std::optional<uint32_t> width,
                       std::optional<uint32_t> height,
                       bool                    allow_high_dpi)
    : m_initial_title(title)
    , m_initial_position_x(position_x)
    , m_initial_position_y(position_y)
    , m_initial_width(width)
    , m_initial_height(height)
    , m_allow_high_dpi(allow_high_dpi)
    , m_sdl_window(nullptr)
    , m_id(id)
    , m_sync_interval(1)
    , m_clear_color(cornflowerblue)
{
    log_debug("Creating window with title '{}'", title);

    if (!GameImpl::is_instance_initialized())
    {
        CER_THROW_LOGIC_ERROR_STR("The game instance must be initialized prior to creating "
                                  "any windows. Please call InitGame() first.");
    }

    GameImpl& app_impl = GameImpl::instance();

#if defined(__EMSCRIPTEN__) // && ANDROID && IOS
    const auto windows = app_impl.windows();

    if (!windows.empty())
        CER_THROW_LOGIC_ERROR_STR("The current system does not support more than one window.");
#endif

    app_impl.notify_window_created(this);

    SDL_AddEventWatch(sdl_window_event_watcher, this);
}

void WindowImpl::show_message_box(MessageBoxType   type,
                                  std::string_view title,
                                  std::string_view message,
                                  const Window&    parent_window)
{
    const Uint32 flags = [type]() -> Uint32 {
        switch (type)
        {
            case MessageBoxType::Information: return SDL_MESSAGEBOX_INFORMATION;
            case MessageBoxType::Warning: return SDL_MESSAGEBOX_WARNING;
            case MessageBoxType::Error: return SDL_MESSAGEBOX_ERROR;
        }

        return 0;
    }();

    const std::string title_str{title};
    const std::string message_str{message};

    SDL_Window* parent_sdl_window = parent_window ? parent_window.impl()->sdl_window() : nullptr;

    SDL_ShowSimpleMessageBox(flags, title_str.c_str(), message_str.c_str(), parent_sdl_window);
}

auto WindowImpl::create_sdl_window(int additional_flags) -> void
{
    const int flags = get_sdl_window_flags(m_allow_high_dpi) | additional_flags;

    log_debug("  Creating SDL window");

    m_sdl_window = SDL_CreateWindow(m_initial_title.c_str(),
#ifdef __EMSCRIPTEN__
                                    m_initial_position_x.value_or(SDL_WINDOWPOS_CENTERED),
                                    m_initial_position_y.value_or(SDL_WINDOWPOS_CENTERED),
#endif
                                    m_initial_width.value_or(1280u),
                                    m_initial_height.value_or(720u),
                                    flags);

    if (m_sdl_window == nullptr)
        CER_THROW_RUNTIME_ERROR("Failed to create the internal window. Reason: {}", SDL_GetError());
}

WindowImpl::~WindowImpl() noexcept
{
    if (m_sdl_window != nullptr)
    {
        log_debug("  Destroying SDL window");
        SDL_DestroyWindow(m_sdl_window);
        m_sdl_window = nullptr;
    }

    GameImpl& app_impl = GameImpl::instance();
    app_impl.notify_window_destroyed(this);
}

uint32_t WindowImpl::id() const
{
    return m_id;
}

void WindowImpl::set_id(uint32_t value)
{
    m_id = value;
}

Vector2 WindowImpl::size() const
{
    int width{};
    int height{};
    SDL_GetWindowSize(m_sdl_window, &width, &height);
    return {static_cast<float>(width), static_cast<float>(height)};
}

Vector2 WindowImpl::size_px() const
{
    int widthPx{};
    int heightPx{};
    SDL_GetWindowSizeInPixels(m_sdl_window, &widthPx, &heightPx);
    return {static_cast<float>(widthPx), static_cast<float>(heightPx)};
}

float WindowImpl::pixel_ratio() const
{
#ifdef __EMSCRIPTEN__
    return emscripten_get_device_pixel_ratio();
#else

    int width{};
    int height{};
    SDL_GetWindowSize(m_sdl_window, &width, &height);

    int widthPx{};
    int heightPx{};
    SDL_GetWindowSizeInPixels(m_sdl_window, &widthPx, &heightPx);

    return static_cast<float>(static_cast<double>(widthPx) / static_cast<double>(width));
#endif
}

std::string_view WindowImpl::title() const
{
    return SDL_GetWindowTitle(m_sdl_window);
}

void WindowImpl::set_title(std::string_view value)
{
    const std::string str{value};
    SDL_SetWindowTitle(m_sdl_window, str.c_str());
}

void WindowImpl::set_visible(bool value)
{
    value ? SDL_ShowWindow(m_sdl_window) : SDL_HideWindow(m_sdl_window);
}

void WindowImpl::set_always_on_top(bool value)
{
    SDL_SetWindowAlwaysOnTop(m_sdl_window, value ? SDL_TRUE : SDL_FALSE);
}

void WindowImpl::set_bordered(bool value)
{
    SDL_SetWindowBordered(m_sdl_window, value ? SDL_TRUE : SDL_FALSE);
}

void WindowImpl::set_full_screen(bool value)
{
#ifdef __EMSCRIPTEN__
    SDL_SetWindowFullscreen(m_sdl_window, value ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
#else
    SDL_SetWindowFullscreen(m_sdl_window, value);
#endif
}

void WindowImpl::set_resizable(bool value)
{
    SDL_SetWindowResizable(m_sdl_window, value ? SDL_TRUE : SDL_FALSE);
}

void WindowImpl::minimize()
{
    SDL_MinimizeWindow(m_sdl_window);
}

void WindowImpl::maximize()
{
    SDL_MaximizeWindow(m_sdl_window);
}

void WindowImpl::show()
{
    SDL_ShowWindow(m_sdl_window);
}

void WindowImpl::hide()
{
    SDL_HideWindow(m_sdl_window);
}

void WindowImpl::set_minimum_size(uint32_t width, uint32_t height)
{
    SDL_SetWindowMinimumSize(m_sdl_window, gsl::narrow<int>(width), gsl::narrow<int>(height));
}

void WindowImpl::set_maximum_size(uint32_t width, uint32_t height)
{
    SDL_SetWindowMaximumSize(m_sdl_window, gsl::narrow<int>(width), gsl::narrow<int>(height));
}

void WindowImpl::set_mouse_grab(bool value)
{
    SDL_SetWindowMouseGrab(m_sdl_window, value ? SDL_TRUE : SDL_FALSE);
}

void WindowImpl::set_position(int32_t x, int32_t y)
{
    SDL_SetWindowPosition(m_sdl_window, gsl::narrow<int>(x), gsl::narrow<int>(y));
}

void WindowImpl::set_size(uint32_t width, uint32_t height)
{
    SDL_SetWindowSize(m_sdl_window, gsl::narrow<int>(width), gsl::narrow<int>(height));
}

void WindowImpl::set_resize_callback(const Window::ResizeCallback& value)
{
    m_resize_callback = value;
}

uint32_t WindowImpl::display_index() const
{
#ifdef __EMSCRIPTEN__
    return uint32_t(SDL_GetWindowDisplayIndex(m_sdl_window));
#else
    return static_cast<uint32_t>(SDL_GetDisplayForWindow(m_sdl_window));
#endif
}

SDL_Window* WindowImpl::sdl_window() const
{
    return m_sdl_window;
}

uint32_t WindowImpl::sync_interval() const
{
    return m_sync_interval;
}

void WindowImpl::set_sync_interval(uint32_t value)
{
    m_sync_interval = value;
}

void WindowImpl::set_clear_color(std::optional<Color> value)
{
    m_clear_color = value;
}

std::optional<Color> WindowImpl::clear_color() const
{
    return m_clear_color;
}
} // namespace cer::details