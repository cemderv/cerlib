// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "GameImpl.hpp"
#include "WindowImpl.hpp"
#include "audio/AudioDevice.hpp"
#include "cerlib/Input.hpp"
#include "cerlib/Logging.hpp"
#include "cerlib/RunGame.hpp"
#include "contentmanagement/ContentManager.hpp"
#include "contentmanagement/FileSystem.hpp"
#include "graphics/FontImpl.hpp"
#include "graphics/GraphicsDevice.hpp"
#include "input/GamepadImpl.hpp"
#include "input/InputImpl.hpp"
#include "util/InternalError.hpp"
#include <gsl/util>

#ifndef __EMSCRIPTEN__
#define SDL_MAIN_NOIMPL
#include <SDL3/SDL_main.h>
#endif

#include <algorithm>
#include <cassert>
#include <vector>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#if defined(__ANDROID__)
#include <android/asset_manager_jni.h>
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

#ifdef CERLIB_HAVE_OPENGL
#include "graphics/opengl/OpenGLGraphicsDevice.hpp"
#endif

#ifdef LoadImage
#undef LoadImage
#endif

// clang-format off
#ifdef __EMSCRIPTEN__
#  define CER_EVENT_QUIT                SDL_QUIT
#  define CER_EVENT_WINDOW_SHOWN        SDL_WINDOWEVENT_SHOWN
#  define CER_EVENT_WINDOW_HIDDEN       SDL_WINDOWEVENT_HIDDEN
#  define CER_EVENT_WINDOW_MOVED        SDL_WINDOWEVENT_MOVED
#  define CER_EVENT_WINDOW_RESIZED      SDL_WINDOWEVENT_RESIZED
#  define CER_EVENT_WINDOW_MINIMIZED    SDL_WINDOWEVENT_MINIMIZED
#  define CER_EVENT_WINDOW_MAXIMIZED    SDL_WINDOWEVENT_MAXIMIZED
#  define CER_EVENT_WINDOW_ENTER        SDL_WINDOWEVENT_ENTER
#  define CER_EVENT_WINDOW_LEAVE        SDL_WINDOWEVENT_LEAVE
#  define CER_EVENT_WINDOW_FOCUS_GAINED SDL_WINDOWEVENT_FOCUS_GAINED
#  define CER_EVENT_WINDOW_FOCUS_LOST   SDL_WINDOWEVENT_FOCUS_LOST
#  define CER_EVENT_WINDOW_CLOSE        SDL_WINDOWEVENT_CLOSE
#  define CER_EVENT_KEYDOWN             SDL_KEYDOWN
#  define CER_EVENT_KEYUP               SDL_KEYUP
#  define CER_EVENT_MOUSEMOTION         SDL_MOUSEMOTION
#  define CER_EVENT_MOUSEBUTTONDOWN     SDL_MOUSEBUTTONDOWN
#  define CER_EVENT_MOUSEBUTTONUP       SDL_MOUSEBUTTONUP
#  define CER_EVENT_MOUSEWHEEL          SDL_MOUSEWHEEL
#  define CER_EVENT_TOUCH_FINGER_DOWN   SDL_FINGERDOWN
#  define CER_EVENT_TOUCH_FINGER_UP     SDL_FINGERUP
#  define CER_EVENT_TOUCH_FINGER_MOTION SDL_FINGERMOTION
#  define SDL_EVENT_GAMEPAD_ADDED       SDL_CONTROLLERDEVICEADDED
#  define SDL_EVENT_GAMEPAD_REMOVED     SDL_CONTROLLERDEVICEREMOVED
#else
#  define CER_EVENT_QUIT                SDL_EVENT_QUIT
#  define CER_EVENT_WINDOW_SHOWN        SDL_EVENT_WINDOW_SHOWN
#  define CER_EVENT_WINDOW_HIDDEN       SDL_EVENT_WINDOW_HIDDEN
#  define CER_EVENT_WINDOW_MOVED        SDL_EVENT_WINDOW_MOVED
#  define CER_EVENT_WINDOW_RESIZED      SDL_EVENT_WINDOW_RESIZED
#  define CER_EVENT_WINDOW_MINIMIZED    SDL_EVENT_WINDOW_MINIMIZED
#  define CER_EVENT_WINDOW_MAXIMIZED    SDL_EVENT_WINDOW_MAXIMIZED
#  define CER_EVENT_WINDOW_ENTER        SDL_EVENT_WINDOW_MOUSE_ENTER
#  define CER_EVENT_WINDOW_LEAVE        SDL_EVENT_WINDOW_MOUSE_LEAVE
#  define CER_EVENT_WINDOW_FOCUS_GAINED SDL_EVENT_WINDOW_FOCUS_GAINED
#  define CER_EVENT_WINDOW_FOCUS_LOST   SDL_EVENT_WINDOW_FOCUS_LOST
#  define CER_EVENT_WINDOW_CLOSE        SDL_EVENT_WINDOW_CLOSE_REQUESTED
#  define CER_EVENT_KEYDOWN             SDL_EVENT_KEY_DOWN
#  define CER_EVENT_KEYUP               SDL_EVENT_KEY_UP
#  define CER_EVENT_MOUSEMOTION         SDL_EVENT_MOUSE_MOTION
#  define CER_EVENT_MOUSEBUTTONDOWN     SDL_EVENT_MOUSE_BUTTON_DOWN
#  define CER_EVENT_MOUSEBUTTONUP       SDL_EVENT_MOUSE_BUTTON_UP
#  define CER_EVENT_MOUSEWHEEL          SDL_EVENT_MOUSE_WHEEL
#  define CER_EVENT_TOUCH_FINGER_DOWN   SDL_EVENT_FINGER_DOWN
#  define CER_EVENT_TOUCH_FINGER_UP     SDL_EVENT_FINGER_UP
#  define CER_EVENT_TOUCH_FINGER_MOTION SDL_EVENT_FINGER_MOTION
#endif
// clang-format on

namespace cer::details
{
using namespace std::chrono_literals;

static std::unique_ptr<GameImpl> s_game_instance;

static constexpr bool force_audio_disabled = false;

GameImpl::GameImpl(bool enable_audio)
{
    log_debug("Creating game");

    auto init_flags = SDL_INIT_VIDEO | SDL_INIT_JOYSTICK;

#ifdef __EMSCRIPTEN__
    init_flags |= SDL_INIT_GAMECONTROLLER;
#else
    init_flags |= SDL_INIT_GAMEPAD;
#endif

#ifdef __EMSCRIPTEN__
    if (SDL_Init(init_flags) != 0)
    {
#else
    if (SDL_Init(init_flags) != SDL_TRUE)
    {
#endif
        const auto error = SDL_GetError();
        CER_THROW_RUNTIME_ERROR("Failed to initialize the windowing system. Reason: {}", error);
    }

    log_debug("SDL is initialized");

    if (enable_audio && !force_audio_disabled)
    {
        log_debug("Audio is enabled, attempting to initialize it");

        auto success   = false;
        m_audio_device = std::make_unique<AudioDevice>(success);
        if (!success)
        {
            log_debug("Tried to initialize audio engine but failed; disabling audio");
            m_audio_device.reset();
        }
        else
        {
            log_debug("Audio initialized successfully");
        }
    }

    log_debug("Creating ContentManager");

    m_content_manager = std::make_unique<ContentManager>();

    open_initial_gamepads();
}

GameImpl::~GameImpl() noexcept = default;

void GameImpl::init_instance(bool enable_audio)
{
    if (s_game_instance)
    {
        CER_THROW_LOGIC_ERROR_STR("The game is already initialized exists.");
    }

    s_game_instance = std::make_unique<GameImpl>(enable_audio);
}

GameImpl& GameImpl::instance()
{
    if (!s_game_instance)
    {
        CER_THROW_LOGIC_ERROR_STR("The game is not initialized yet. Please call InitGame() first.");
    }

    assert(s_game_instance);
    return *s_game_instance;
}

bool GameImpl::is_instance_initialized()
{
    return s_game_instance != nullptr;
}

void GameImpl::destroy_instance()
{
    s_game_instance.reset();
}

void GameImpl::run()
{
    if (m_is_running)
        CER_THROW_LOGIC_ERROR_STR("The game is already running.");

    log_debug("Starting to run game");

    m_is_running = true;

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg(
        [](void* user_param) { static_cast<GameImpl*>(user_param)->tick(); },
        this,
        0,
        1);
#else
    while (tick())
        ;
#endif
}

void GameImpl::set_load_func(const LoadFunc& func)
{
    m_load_func = func;
}

void GameImpl::set_update_func(const UpdateFunc& func)
{
    m_update_func = func;
}

void GameImpl::set_draw_func(const DrawFunc& func)
{
    m_draw_func = func;
}

void GameImpl::set_event_func(const EventFunc& func)
{
    m_event_func = func;
}

ContentManager& GameImpl::content_manager()
{
    return *m_content_manager;
}

uint32_t GameImpl::display_count() const
{
#ifdef __EMSCRIPTEN__
    return uint32_t(SDL_GetNumVideoDisplays());
#else
    int count{};
    std::ignore = SDL_GetDisplays(&count);
    return count;
#endif
}

static std::optional<ImageFormat> from_sdl_display_mode_format(Uint32 format)
{
    switch (format)
    {
        case SDL_PIXELFORMAT_ARGB8888:
        case SDL_PIXELFORMAT_RGBA8888: return ImageFormat::R8G8B8A8_UNorm;
    }

    return {};
}

static std::optional<DisplayMode> from_sdl_display_mode(const SDL_DisplayMode& sdlMode)
{
    if (const auto format = from_sdl_display_mode_format(sdlMode.format))
    {
        return DisplayMode{
            .format       = *format,
            .width        = static_cast<uint32_t>(sdlMode.w),
            .height       = static_cast<uint32_t>(sdlMode.h),
            .refresh_rate = static_cast<uint32_t>(sdlMode.refresh_rate),
            // .ContentScale = sdlMode.pixel_density,
            .content_scale = 1.0,
        };
    }

    return {};
}

std::optional<DisplayMode> GameImpl::current_display_mode(uint32_t display_index) const
{
#ifdef __EMSCRIPTEN__
    SDL_DisplayMode mode{};
    SDL_GetCurrentDisplayMode(int(display_index), &mode);
    return from_sdl_display_mode(mode);
#else
    const auto mode = SDL_GetCurrentDisplayMode(static_cast<SDL_DisplayID>(display_index));
    return mode != nullptr ? from_sdl_display_mode(*mode) : std::nullopt;
#endif
}

std::vector<DisplayMode> GameImpl::display_modes(uint32_t display_index) const
{
    auto list = std::vector<DisplayMode>();

#ifdef __EMSCRIPTEN__

    const auto mode_count = SDL_GetNumDisplayModes(static_cast<int>(display_index));
    list.reserve(static_cast<size_t>(mode_count));

    for (int i = 0; i < mode_count; ++i)
    {
        SDL_DisplayMode sdl_mode{};
        if (SDL_GetDisplayMode(static_cast<int>(display_index), i, &sdl_mode) == 0)
        {
            if (const auto mode = from_sdl_display_mode(sdl_mode))
            {
                list.push_back(*mode);
            }
        }
    }

#else

    int mode_count{};

    if (const auto modes =
            SDL_GetFullscreenDisplayModes(static_cast<SDL_DisplayID>(display_index), &mode_count);
        mode_count > 0 && modes != nullptr)
    {
        list.reserve(static_cast<size_t>(mode_count));

        for (int i = 0; i < mode_count; ++i)
        {
            if (const auto mode = from_sdl_display_mode(*modes[i]))
                list.push_back(*mode);
        }
    }

#endif

    return list;
}

float GameImpl::display_content_scale(uint32_t display_index) const
{
#ifdef __EMSCRIPTEN__
    return 1.0f;
#else
    return SDL_GetDisplayContentScale(static_cast<SDL_DisplayID>(display_index));
#endif
}

DisplayOrientation GameImpl::display_orientation(uint32_t display_index) const
{
#ifdef __EMSCRIPTEN__
    const SDL_DisplayOrientation orientation =
        SDL_GetDisplayOrientation(static_cast<int>(display_index));
#else
    const SDL_DisplayOrientation orientation =
        SDL_GetCurrentDisplayOrientation(static_cast<SDL_DisplayID>(display_index));
#endif

    switch (orientation)
    {
        case SDL_ORIENTATION_LANDSCAPE: return DisplayOrientation::Landscape;
        case SDL_ORIENTATION_LANDSCAPE_FLIPPED: return DisplayOrientation::LandscapeFlipped;
        case SDL_ORIENTATION_PORTRAIT: return DisplayOrientation::Portrait;
        case SDL_ORIENTATION_PORTRAIT_FLIPPED: return DisplayOrientation::PortraitFlipped;
        case SDL_ORIENTATION_UNKNOWN: return DisplayOrientation::Unknown;
    }

    return DisplayOrientation::Unknown;
}

Window GameImpl::keyboard_focused_window() const
{
    const auto sdl_window = SDL_GetKeyboardFocus();

    return Window(find_window_by_sdl_window(sdl_window));
}

Window GameImpl::mouse_focused_window() const
{
    SDL_Window* sdl_window = SDL_GetMouseFocus();

    return Window(find_window_by_sdl_window(sdl_window));
}

bool GameImpl::is_audio_device_initialized() const
{
    return m_audio_device != nullptr;
}

GraphicsDevice& GameImpl::graphics_device()
{
    if (!m_graphics_device)
    {
        CER_THROW_LOGIC_ERROR_STR("Attempting to load graphics resources or draw. However, "
                                  "no window was created. Please "
                                  "create a window first.");
    }

    return *m_graphics_device;
}

AudioDevice& GameImpl::audio_device()
{
    if (!m_audio_device)
    {
        CER_THROW_LOGIC_ERROR_STR(
            "No audio engine available. Either no suitable audio device was found, or the "
            "game was not initialized with "
            "audio enabled. Please see the enableAudio parameter of InitGame().");
    }

    return *m_audio_device;
}

void GameImpl::ensure_graphics_device_initialized(WindowImpl& first_window)
{
    if (!m_graphics_device)
    {
        log_debug("Initializing device");

        try
        {
#ifdef CERLIB_HAVE_OPENGL
            m_graphics_device = std::make_unique<OpenGLGraphicsDevice>(first_window);
#else
            CER_THROW_RUNTIME_ERROR_STR("OpenGL is not available on this system.");
#endif
        }
        catch (std::exception& ex)
        {
            std::ignore = ex;
            log_debug("Device creation failed: {}", ex.what());
            m_graphics_device.reset();
            throw;
        }
    }

    assert(m_graphics_device);
}

std::span<WindowImpl* const> GameImpl::windows() const
{
    return m_windows;
}

std::vector<Gamepad> GameImpl::gamepads() const
{
    return m_connected_gamepads;
}

void GameImpl::open_initial_gamepads()
{
#ifndef __EMSCRIPTEN__
    assert(m_connected_gamepads.empty());

    int             count            = 0;
    SDL_JoystickID* sdl_joystick_ids = SDL_GetGamepads(&count);

    for (int i = 0; i < count; ++i)
    {
        if (auto sdl_gamepad = SDL_OpenGamepad(sdl_joystick_ids[i]))
        {
            GamepadImpl* gamepad_impl =
                std::make_unique<GamepadImpl>(sdl_joystick_ids[i], sdl_gamepad).release();

            m_connected_gamepads.emplace_back(gamepad_impl);
        }
    }
#endif
}

bool GameImpl::tick()
{
    if (!m_is_running)
        return false;

    if (!m_has_loaded_content)
    {
        if (m_load_func)
        {
            m_load_func();
        }

        m_has_loaded_content = true;
    }

    process_events();

    bool should_exit = false;

    if (m_audio_device)
        m_audio_device->purge_sounds();

    const auto do_update = [this, &should_exit](GameTime game_time) {
        if (m_update_func && !m_update_func(game_time))
            should_exit = true;
    };

    const Uint64 current_time   = SDL_GetPerformanceCounter();
    const Uint64 time_frequency = SDL_GetPerformanceFrequency();

    m_game_time.elapsed_time =
        m_is_first_tick
            ? 0
            : (static_cast<double>(current_time) - static_cast<double>(m_previous_time)) * 1000 /
                  static_cast<double>(time_frequency) * 0.001;

    m_game_time.total_time += m_game_time.elapsed_time;
    do_update(m_game_time);

    m_previous_time = current_time;

    if (m_draw_func)
    {
        for (WindowImpl* window_impl : m_windows)
        {
            Window window{window_impl};
            m_graphics_device->start_frame(window);

            const auto _ = gsl::finally([this, &window] { m_graphics_device->end_frame(window); });

            m_draw_func(window);
        }
    }

    m_is_first_tick = false;

    return !should_exit;
}

void GameImpl::process_events()
{
    InputImpl& input_impl = InputImpl::instance();

    const Vector2 mouse_position = current_mouse_position();
    input_impl.set_mouse_position_delta(mouse_position - m_previous_mouse_position);
    m_previous_mouse_position = mouse_position;

    input_impl.set_mouse_wheel_delta({});

    SDL_Event event{};
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case CER_EVENT_QUIT: {
                m_is_running = false;
                break;
            }
            case CER_EVENT_WINDOW_SHOWN: {
                raise_event(WindowShownEvent{
                    .timestamp = event.window.timestamp,
                    .window    = find_window_by_sdl_window_id(event.window.windowID),
                });

                break;
            }
            case CER_EVENT_WINDOW_HIDDEN: {
                raise_event(WindowHiddenEvent{
                    .timestamp = event.window.timestamp,
                    .window    = find_window_by_sdl_window_id(event.window.windowID),
                });

                break;
            }
            case CER_EVENT_WINDOW_MOVED: {
                raise_event(WindowMovedEvent{
                    .timestamp = event.window.timestamp,
                    .window    = find_window_by_sdl_window_id(event.window.windowID),
                });

                break;
            }
            case CER_EVENT_WINDOW_RESIZED: {
                raise_event(WindowResizedEvent{
                    .timestamp  = event.window.timestamp,
                    .window     = find_window_by_sdl_window_id(event.window.windowID),
                    .new_width  = static_cast<uint32_t>(event.window.data1),
                    .new_height = static_cast<uint32_t>(event.window.data2),
                });

                break;
            }
            case CER_EVENT_WINDOW_MINIMIZED: {
                raise_event(WindowMinimizedEvent{
                    .timestamp = event.window.timestamp,
                    .window    = find_window_by_sdl_window_id(event.window.windowID),
                });

                break;
            }
            case CER_EVENT_WINDOW_MAXIMIZED: {
                raise_event(WindowMaximizedEvent{
                    .timestamp = event.window.timestamp,
                    .window    = find_window_by_sdl_window_id(event.window.windowID),
                });

                break;
            }
            case CER_EVENT_WINDOW_ENTER: {
                raise_event(WindowGotMouseFocusEvent{
                    .timestamp = event.window.timestamp,
                    .window    = find_window_by_sdl_window_id(event.window.windowID),
                });

                break;
            }
            case CER_EVENT_WINDOW_LEAVE: {
                raise_event(WindowLostMouseFocusEvent{
                    .timestamp = event.window.timestamp,
                    .window    = find_window_by_sdl_window_id(event.window.windowID),
                });

                break;
            }
            case CER_EVENT_WINDOW_FOCUS_GAINED: {
                raise_event(WindowGotKeyboardFocusEvent{
                    .timestamp = event.window.timestamp,
                    .window    = find_window_by_sdl_window_id(event.window.windowID),
                });

                break;
            }
            case CER_EVENT_WINDOW_FOCUS_LOST: {
                raise_event(WindowLostKeyboardFocusEvent{
                    .timestamp = event.window.timestamp,
                    .window    = find_window_by_sdl_window_id(event.window.windowID),
                });

                break;
            }
            case CER_EVENT_WINDOW_CLOSE: {
                raise_event(WindowCloseEvent{
                    .timestamp = event.window.timestamp,
                    .window    = find_window_by_sdl_window_id(event.window.windowID),
                });

                break;
            }
            case CER_EVENT_KEYDOWN: {
#ifdef __EMSCRIPTEN__
                const auto [key, modifiers] = InputImpl::from_sdl_keysym(event.key.keysym);
#else
                const auto [key, modifiers] =
                    InputImpl::from_sdl_keysym(event.key.key, event.key.mod);
#endif

                raise_event(KeyPressEvent{
                    .timestamp = event.key.timestamp,
                    .window    = find_window_by_sdl_window_id(event.key.windowID),
                    .key       = key,
                    .modifiers = modifiers,
                    .is_repeat = event.key.repeat != 0,
                });

                break;
            }
            case CER_EVENT_KEYUP: {
#ifdef __EMSCRIPTEN__
                const auto [key, modifiers] = InputImpl::from_sdl_keysym(event.key.keysym);
#else
                const auto [key, modifiers] =
                    InputImpl::from_sdl_keysym(event.key.key, event.key.mod);
#endif

                raise_event(KeyReleaseEvent{
                    .timestamp = event.key.timestamp,
                    .window    = find_window_by_sdl_window_id(event.key.windowID),
                    .key       = key,
                    .modifiers = modifiers,
                    .is_repeat = event.key.repeat != 0,
                });

                break;
            }
            case CER_EVENT_MOUSEMOTION: {
                const Vector2 position{static_cast<float>(event.motion.x),
                                       static_cast<float>(event.motion.y)};

                const Vector2 delta{static_cast<float>(event.motion.xrel),
                                    static_cast<float>(event.motion.yrel)};

                raise_event(MouseMoveEvent{
                    .timestamp = event.motion.timestamp,
                    .window    = find_window_by_sdl_window_id(event.motion.windowID),
                    .id        = event.motion.which,
                    .position  = position,
                    .delta     = delta,
                });

                break;
            }
            case CER_EVENT_MOUSEBUTTONDOWN:
            case CER_EVENT_MOUSEBUTTONUP: {
                const Uint64      timestamp = event.button.timestamp;
                Window            window    = find_window_by_sdl_window_id(event.button.windowID);
                const Vector2     position  = {static_cast<float>(event.button.x),
                                               static_cast<float>(event.button.y)};
                const auto        id        = event.button.which;
                const MouseButton button    = InputImpl::from_sdl_mouse_button(event.button.button);

                if (event.button.type == CER_EVENT_MOUSEBUTTONDOWN)
                {
                    if (event.button.clicks == 1)
                    {
                        raise_event(MouseButtonPressEvent{
                            .timestamp = timestamp,
                            .window    = std::move(window),
                            .id        = id,
                            .button    = button,
                            .position  = position,
                        });
                    }
                    else if (event.button.clicks == 2)
                    {
                        raise_event(MouseDoubleClickEvent{
                            .timestamp = timestamp,
                            .window    = std::move(window),
                            .id        = id,
                            .button    = button,
                            .position  = position,
                        });
                    }
                }
                else
                {
                    raise_event(MouseButtonReleaseEvent{
                        .timestamp = timestamp,
                        .window    = std::move(window),
                        .id        = id,
                        .button    = button,
                        .position  = position,
                    });
                }

                break;
            }
            case CER_EVENT_MOUSEWHEEL: {
                const Vector2 position = {static_cast<float>(event.wheel.x),
                                          static_cast<float>(event.wheel.y)};

#ifdef __EMSCRIPTEN__
                Vector2 delta{float(event.wheel.preciseX), float(event.wheel.preciseY)};
#else
                Vector2 delta{event.wheel.x, event.wheel.y};
#endif

                if (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED)
                    delta = -delta;

                raise_event(MouseWheelEvent{
                    .timestamp = event.motion.timestamp,
                    .window    = find_window_by_sdl_window_id(event.motion.windowID),
                    .id        = event.motion.which,
                    .position  = position,
                    .delta     = delta,
                });

                input_impl.set_mouse_wheel_delta(delta);

                break;
            }
            case SDL_EVENT_GAMEPAD_ADDED: {
#ifdef __EMSCRIPTEN__
                const SDL_JoystickID sdl_joystick_id = event.jdevice.which;
#else
                const SDL_JoystickID sdl_joystick_id = event.gdevice.which;
#endif

                const auto it_existing_gamepad = find_gamepad_by_sdl_joystick_id(sdl_joystick_id);

                if (it_existing_gamepad == m_connected_gamepads.cend())
                {
#ifdef __EMSCRIPTEN__
                    const auto sdl_gamepad = SDL_GameControllerOpen(sdl_joystick_id);
#else
                    SDL_Gamepad* sdl_gamepad = SDL_OpenGamepad(sdl_joystick_id);
#endif

                    if (sdl_gamepad != nullptr)
                    {
                        GamepadImpl* gamepad_impl =
                            std::make_unique<GamepadImpl>(sdl_joystick_id, sdl_gamepad).release();

                        m_connected_gamepads.emplace_back(gamepad_impl);

                        raise_event(GamepadConnectedEvent{
                            .gamepad = m_connected_gamepads.back(),
                        });
                    }
                }
                break;
            }
            case SDL_EVENT_GAMEPAD_REMOVED: {
#ifdef __EMSCRIPTEN__
                const SDL_JoystickID id = event.cdevice.which;
#else
                const SDL_JoystickID id = event.gdevice.which;
#endif

                const auto it = std::ranges::find_if(m_connected_gamepads, [id](const auto& e) {
                    return e.impl()->joystick_id() == id;
                });

                if (it != m_connected_gamepads.cend())
                {
                    raise_event(GamepadDisconnectedEvent{
                        .gamepad = *it,
                    });

#ifdef __EMSCRIPTEN__
                    SDL_GameControllerClose(it->impl()->sdl_gamepad());
#else
                    SDL_CloseGamepad(it->impl()->sdl_gamepad());
#endif

                    m_connected_gamepads.erase(it);
                }
                break;
            }
            case CER_EVENT_TOUCH_FINGER_UP:
            case CER_EVENT_TOUCH_FINGER_DOWN:
            case CER_EVENT_TOUCH_FINGER_MOTION: {
                const auto type = [t = event.type] {
                    switch (t)
                    {
                        case CER_EVENT_TOUCH_FINGER_UP: return TouchFingerEventType::Release;
                        case CER_EVENT_TOUCH_FINGER_DOWN: return TouchFingerEventType::Press;
                        case CER_EVENT_TOUCH_FINGER_MOTION: return TouchFingerEventType::Motion;
                    }
                    return static_cast<TouchFingerEventType>(-1);
                }();

                if (type != static_cast<TouchFingerEventType>(-1))
                {
                    const Window  window = find_window_by_sdl_window_id(event.tfinger.windowID);
                    const Vector2 window_size = window.size_px();
                    const Vector2 position =
                        Vector2{event.tfinger.x, event.tfinger.y} * window_size;
                    const Vector2 delta = Vector2{event.tfinger.dx, event.tfinger.dy} * window_size;

                    const auto evt = TouchFingerEvent{
                        .type      = type,
                        .timestamp = event.tfinger.timestamp,
                        .window    = window,
#ifdef __EMSCRIPTEN__
                        .touch_id  = static_cast<uint64_t>(event.tfinger.touchId),
                        .finger_id = static_cast<uint64_t>(event.tfinger.fingerId),
#else
                        .touch_id  = event.tfinger.touchID,
                        .finger_id = event.tfinger.fingerID,
#endif
                        .position = position,
                        .delta    = delta,
                        .pressure = event.tfinger.pressure,
                    };

                    raise_event(evt);
                }

                break;
            }
        }
    }

    input_impl.update_key_states();
}

void GameImpl::notify_window_created(WindowImpl* window)
{
#ifndef NDEBUG
    assert(window);
    const auto it = std::ranges::find(std::as_const(m_windows), window);
    assert(it == m_windows.cend());
#endif

    m_windows.push_back(window);
}

void GameImpl::notify_window_destroyed(WindowImpl* window)
{
    assert(window);
    const auto it = std::ranges::find(std::as_const(m_windows), window);
    assert(it != m_windows.cend());

    m_windows.erase(it);
}

Window GameImpl::find_window_by_sdl_window_id(Uint32 sdl_window_id) const
{
    SDL_Window* sdl_window  = SDL_GetWindowFromID(sdl_window_id);
    WindowImpl* window_impl = find_window_by_sdl_window(sdl_window);

    return Window{window_impl};
}

WindowImpl* GameImpl::find_window_by_sdl_window(SDL_Window* sdl_window) const
{
    const auto it = std::ranges::find_if(m_windows, [sdl_window](WindowImpl* e) {
        return e->sdl_window() == sdl_window;
    });

    return it != m_windows.cend() ? *it : nullptr;
}

void GameImpl::raise_event(const Event& event)
{
    if (m_event_func)
        m_event_func(event);
}

std::ranges::borrowed_iterator_t<const std::vector<Gamepad>&> GameImpl::
    find_gamepad_by_sdl_joystick_id(SDL_JoystickID sdl_joystick_id) const
{
    return std::ranges::find_if(m_connected_gamepads, [sdl_joystick_id](const auto& e) {
        return e.impl()->joystick_id() == sdl_joystick_id;
    });
}

int run_game(int a, char* b[], MainFunc c, void* d)
{
#ifndef __EMSCRIPTEN__
    return SDL_RunApp(a, b, c, d);
#else
    return c(a, b);
#endif
}
} // namespace cer::details
