// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "WindowImpl.hpp"

#include "cerlib/Logging.hpp"
#include "cerlib/Version.hpp"
#include "game/GameImpl.hpp"
#include "util/Platform.hpp"
#include "util/narrow_cast.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

namespace cer::details
{
static auto get_sdl_window_flags(bool allow_high_dpi) -> int
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

static auto sdl_window_event_watcher(void* userdata, SDL_Event* event)
{
    auto*       window     = static_cast<WindowImpl*>(userdata);
    const auto* sdl_window = window->sdl_window();

#ifdef __EMSCRIPTEN__
    if (event->type == SDL_WINDOWEVENT && event->window.event == SDL_WINDOWEVENT_RESIZED)
    {
#else
    if (event->type == SDL_EVENT_WINDOW_RESIZED)
    {
#endif
        const auto win = SDL_GetWindowFromID(event->window.windowID);

        if (win == sdl_window)
        {
            window->handle_resize_event();
        }
    }

#ifdef __EMSCRIPTEN__
    return 0;
#else
    return false;
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
    , m_clear_color(cornflowerblue * 0.25f)
{
    log_verbose("Creating window with title '{}'", title);

    if (!GameImpl::is_instance_initialized())
    {
        throw std::logic_error{"The game instance must be initialized prior to creating "
                               "any windows. Please call run_game() first."};
    }

    auto& app_impl = GameImpl::instance();

    if (is_mobile_platform() || target_platform() == TargetPlatform::Web)
    {
        if (const auto windows = app_impl.windows(); !windows.empty())
        {
            throw std::logic_error{"The current system does not support more than one window."};
        }
    }

    app_impl.notify_window_created(this);

    SDL_AddEventWatch(sdl_window_event_watcher, this);
}

void WindowImpl::show_message_box(MessageBoxType   type,
                                  std::string_view title,
                                  std::string_view message,
                                  const Window&    parent_window)
{
    const auto flags = [type]() -> Uint32 {
        switch (type)
        {
            case MessageBoxType::Information: return SDL_MESSAGEBOX_INFORMATION;
            case MessageBoxType::Warning: return SDL_MESSAGEBOX_WARNING;
            case MessageBoxType::Error: return SDL_MESSAGEBOX_ERROR;
        }

        return 0;
    }();

    const auto title_str   = std::string{title};
    const auto message_str = std::string{message};

    auto* parent_sdl_window = parent_window ? parent_window.impl()->sdl_window() : nullptr;

    SDL_ShowSimpleMessageBox(flags, title_str.c_str(), message_str.c_str(), parent_sdl_window);
}

void WindowImpl::activate_onscreen_keyboard()
{
    if (is_mobile_platform())
    {
#ifdef __EMSCRIPTEN__
        SDL_StartTextInput();
#else
        SDL_StartTextInput(m_sdl_window);
#endif
    }
}

void WindowImpl::deactivate_onscreen_keyboard()
{
    if (is_mobile_platform())
    {
#ifdef __EMSCRIPTEN__
        SDL_StopTextInput();
#else
        SDL_StopTextInput(m_sdl_window);
#endif
    }
}

auto WindowImpl::create_sdl_window(int additional_flags) -> void
{
    const int flags = get_sdl_window_flags(m_allow_high_dpi) | additional_flags;

    log_verbose("  Creating SDL window");

    m_sdl_window = SDL_CreateWindow(m_initial_title.c_str(),
#ifdef __EMSCRIPTEN__
                                    m_initial_position_x.value_or(SDL_WINDOWPOS_CENTERED),
                                    m_initial_position_y.value_or(SDL_WINDOWPOS_CENTERED),
#endif
                                    m_initial_width.value_or(1280u),
                                    m_initial_height.value_or(720u),
                                    flags);

    if (m_sdl_window == nullptr)
    {
        throw std::runtime_error{
            fmt::format("Failed to create the internal window. Reason: {}", SDL_GetError())};
    }

// Ensure that the window receives text input on non-mobile platforms.
#ifdef __EMSCRIPTEN__
    SDL_StartTextInput();
#else
    if (!is_mobile_platform())
    {
        SDL_StartTextInput(m_sdl_window);
    }
#endif
}

WindowImpl::~WindowImpl() noexcept
{
    if (m_sdl_window != nullptr)
    {
        log_verbose("  Destroying SDL window");
        SDL_DestroyWindow(m_sdl_window);
        m_sdl_window = nullptr;
    }

    auto& app_impl = GameImpl::instance();
    app_impl.notify_window_destroyed(this);
}

auto WindowImpl::id() const -> uint32_t
{
    return m_id;
}

void WindowImpl::set_id(uint32_t value)
{
    m_id = value;
}

auto WindowImpl::size() const -> Vector2
{
    int width  = 0;
    int height = 0;
    SDL_GetWindowSize(m_sdl_window, &width, &height);
    return {float(width), float(height)};
}

auto WindowImpl::size_px() const -> Vector2
{
#ifdef __EMSCRIPTEN__
    return size() * pixel_ratio();
#else
    int width_px  = 0;
    int height_px = 0;
    SDL_GetWindowSizeInPixels(m_sdl_window, &width_px, &height_px);
    return {float(width_px), float(height_px)};
#endif
}

auto WindowImpl::pixel_ratio() const -> float
{
#ifdef __EMSCRIPTEN__
    return emscripten_get_device_pixel_ratio();
#else

    int width  = 0;
    int height = 0;
    SDL_GetWindowSize(m_sdl_window, &width, &height);

    int width_px  = 0;
    int height_px = 0;
    SDL_GetWindowSizeInPixels(m_sdl_window, &width_px, &height_px);

    return float(double(width_px) / double(width));
#endif
}

auto WindowImpl::title() const -> std::string_view
{
    return SDL_GetWindowTitle(m_sdl_window);
}

void WindowImpl::set_title(std::string_view value)
{
    const auto str = std::string{value};
    SDL_SetWindowTitle(m_sdl_window, str.c_str());
}

void WindowImpl::set_visible(bool value)
{
    value ? SDL_ShowWindow(m_sdl_window) : SDL_HideWindow(m_sdl_window);
}

void WindowImpl::set_always_on_top(bool value)
{
#ifdef __EMSCRIPTEN__
    SDL_SetWindowAlwaysOnTop(m_sdl_window, value ? SDL_TRUE : SDL_FALSE); // NOLINT
#else
    SDL_SetWindowAlwaysOnTop(m_sdl_window, value); // NOLINT
#endif
}

void WindowImpl::set_bordered(bool value)
{
#ifdef __EMSCRIPTEN__
    SDL_SetWindowBordered(m_sdl_window, value ? SDL_TRUE : SDL_FALSE); // NOLINT
#else
    SDL_SetWindowBordered(m_sdl_window, value); // NOLINT
#endif
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
#ifdef __EMSCRIPTEN__
    SDL_SetWindowResizable(m_sdl_window, value ? SDL_TRUE : SDL_FALSE); // NOLINT
#else
    SDL_SetWindowResizable(m_sdl_window, value); // NOLINT
#endif
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
    SDL_SetWindowMinimumSize(m_sdl_window, narrow<int>(width), narrow<int>(height));
}

void WindowImpl::set_maximum_size(uint32_t width, uint32_t height)
{
    SDL_SetWindowMaximumSize(m_sdl_window, narrow<int>(width), narrow<int>(height));
}

void WindowImpl::set_mouse_grab(bool value)
{
#ifdef __EMSCRIPTEN__
    SDL_SetWindowMouseGrab(m_sdl_window, value ? SDL_TRUE : SDL_FALSE); // NOLINT
#else
    SDL_SetWindowMouseGrab(m_sdl_window, value); // NOLINT
#endif
}

void WindowImpl::set_position(int32_t x, int32_t y)
{
    SDL_SetWindowPosition(m_sdl_window, narrow<int>(x), narrow<int>(y));
}

void WindowImpl::set_size(uint32_t width, uint32_t height)
{
    SDL_SetWindowSize(m_sdl_window, narrow<int>(width), narrow<int>(height));
}

void WindowImpl::set_resize_callback(const Window::ResizeCallback& value)
{
    m_resize_callback = value;
}

auto WindowImpl::display_index() const -> uint32_t
{
#ifdef __EMSCRIPTEN__
    return uint32_t(SDL_GetWindowDisplayIndex(m_sdl_window));
#else
    return uint32_t(SDL_GetDisplayForWindow(m_sdl_window)); // NOLINT
#endif
}

auto WindowImpl::sdl_window() const -> SDL_Window*
{
    return m_sdl_window;
}

auto WindowImpl::sync_interval() const -> uint32_t
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

auto WindowImpl::clear_color() const -> std::optional<Color>
{
    return m_clear_color;
}
} // namespace cer::details