// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "GameImpl.hpp"
#include "WindowImpl.hpp"
#include "audio/AudioDevice.hpp"
#include "cerlib/Input.hpp"
#include "cerlib/Logging.hpp"
#include "cerlib/RunGame.hpp"
#include "cerlib/Version.hpp"
#include "contentmanagement/ContentManager.hpp"
#include "contentmanagement/FileSystem.hpp"
#include "graphics/FontImpl.hpp"
#include "graphics/GraphicsDevice.hpp"
#include "input/GamepadImpl.hpp"
#include "input/InputImpl.hpp"

#ifndef __EMSCRIPTEN__
#define SDL_MAIN_NOIMPL
#include <SDL3/SDL_main.h>
#endif

#include <algorithm>
#include <cassert>
#include <cerlib/List.hpp>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

#ifdef __ANDROID__
#include <android/asset_manager_jni.h>
#endif

#if __APPLE__
#include <TargetConditionals.h>
#endif

#ifdef CERLIB_HAVE_OPENGL
#include "graphics/opengl/OpenGLGraphicsDevice.hpp"
#endif

#ifdef CERLIB_ENABLE_IMGUI
#include <imgui.h>
#ifdef __EMSCRIPTEN__
#include <impl/imgui_impl_sdl2.hpp>
#else
#include <impl/imgui_impl_sdl3.hpp>
#endif
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
#  define CER_EVENT_TEXT_INPUT          SDL_TEXTINPUT
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
#  define CER_EVENT_TEXT_INPUT          SDL_EVENT_TEXT_INPUT
#endif
// clang-format on

namespace cer::details
{
using namespace std::chrono_literals;

static UniquePtr<GameImpl> s_game_instance;

GameImpl::GameImpl(bool enable_audio)
{
    log_verbose("Creating game");

    if (is_desktop_platform() && enable_audio)
    {
        if (const auto* env = SDL_getenv("CERLIB_DISABLE_AUDIO");
            env != nullptr && std::strncmp(env, "1", 1) == 0)
        {
            log_verbose("Implicitly disabling audio due to environment variable");
            enable_audio = false;
        }
    }

    auto init_flags = SDL_INIT_VIDEO | SDL_INIT_JOYSTICK;

#ifdef __EMSCRIPTEN__
    init_flags |= SDL_INIT_GAMECONTROLLER;
#else
    init_flags |= SDL_INIT_GAMEPAD;
#endif

    if (enable_audio)
    {
        init_flags |= SDL_INIT_AUDIO;
    }

#ifdef __EMSCRIPTEN__
    if (SDL_Init(init_flags) != 0)
#else
    if (!SDL_Init(init_flags))
#endif
    {
        const auto error = SDL_GetError();
        throw std::runtime_error{
            fmt::format("Failed to initialize the windowing system. Reason: {}", error)};
    }

    log_verbose("SDL is initialized");

    if (enable_audio)
    {
        log_verbose("Audio is enabled, attempting to initialize it");

        try
        {
            m_audio_device = std::make_unique<AudioDevice>(EngineFlags{}, 44100, 4096, 2);
            log_debug("Audio initialized successfully");
        }
        catch (const std::exception& ex)
        {
            log_debug("Tried to initialize audio engine but failed; disabling audio");
            log_debug("Reason: {}", ex.what());
            m_audio_device.reset();
        }
    }

    log_verbose("Creating ContentManager");

    m_content_manager = std::make_unique<ContentManager>();

    open_initial_gamepads();

    initialize_imgui();
}

GameImpl::~GameImpl() noexcept = default;

void GameImpl::init_instance(bool enable_audio)
{
    if (s_game_instance != nullptr)
    {
        throw std::logic_error{"The game is already initialized exists."};
    }

    s_game_instance = std::make_unique<GameImpl>(enable_audio);
}

auto GameImpl::instance() -> GameImpl&
{
    if (s_game_instance == nullptr)
    {
        throw std::logic_error{"The game is not initialized yet. Please call run_game() first."};
    }

    return *s_game_instance;
}

auto GameImpl::is_instance_initialized() -> bool
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
    {
        throw std::logic_error{"The game is already running."};
    }

    log_verbose("Starting to run game");

    m_is_running = true;

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg(
        [](void* user_param) {
            static_cast<GameImpl*>(user_param)->tick();
        },
        this,
        0,
        1);
#else
    while (tick())
    {
        // Nothing to do
    }
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

#ifdef CERLIB_ENABLE_IMGUI

void GameImpl::set_imgui_draw_func(const ImGuiDrawFunc& func)
{
    m_imgui_draw_func = func;
}

#endif

void GameImpl::set_event_func(const EventFunc& func)
{
    m_event_func = func;
}

auto GameImpl::content_manager() -> ContentManager&
{
    return *m_content_manager;
}

auto GameImpl::display_count() const -> uint32_t
{
#ifdef __EMSCRIPTEN__
    return uint32_t(SDL_GetNumVideoDisplays());
#else
    int count{};
    std::ignore = SDL_GetDisplays(&count);
    return count;
#endif
}

static auto from_sdl_display_mode_format(Uint32 format) -> Option<ImageFormat>
{
    switch (format)
    {
        case SDL_PIXELFORMAT_ARGB8888:
        case SDL_PIXELFORMAT_RGBA8888: return ImageFormat::R8G8B8A8_UNorm;
    }

    return {};
}

static auto from_sdl_display_mode(const SDL_DisplayMode& sdl_mode) -> Option<DisplayMode>
{
    if (const auto format = from_sdl_display_mode_format(sdl_mode.format))
    {
        return DisplayMode{
            .format       = format,
            .width        = uint32_t(sdl_mode.w),
            .height       = uint32_t(sdl_mode.h),
            .refresh_rate = uint32_t(sdl_mode.refresh_rate),
            // .ContentScale = sdlMode.pixel_density,
            .content_scale = 1.0,
        };
    }

    return {};
}

auto GameImpl::current_display_mode(uint32_t display_index) const -> Option<DisplayMode>
{
#ifdef __EMSCRIPTEN__
    SDL_DisplayMode mode{};
    SDL_GetCurrentDisplayMode(int(display_index), &mode);
    return from_sdl_display_mode(mode);
#else
    const auto mode = SDL_GetCurrentDisplayMode(SDL_DisplayID(display_index));
    return mode != nullptr ? from_sdl_display_mode(*mode) : std::nullopt;
#endif
}

auto GameImpl::display_modes(uint32_t display_index) const -> List<DisplayMode>
{
    auto list = List<DisplayMode>{};

#ifdef __EMSCRIPTEN__

    const auto mode_count = SDL_GetNumDisplayModes(int(display_index));
    list.reserve(size_t(mode_count));

    for (int i = 0; i < mode_count; ++i)
    {
        SDL_DisplayMode sdl_mode{};
        if (SDL_GetDisplayMode(int(display_index), i, &sdl_mode) == 0)
        {
            if (const auto mode = from_sdl_display_mode(sdl_mode))
            {
                list.push_back(*mode);
            }
        }
    }

#else

    int mode_count{};

    if (const auto modes = SDL_GetFullscreenDisplayModes(SDL_DisplayID(display_index), &mode_count);
        mode_count > 0 && modes != nullptr)
    {
        const auto modes_span = std::span{modes, size_t(mode_count)};

        list.reserve(size_t(mode_count));

        for (int i = 0; i < mode_count; ++i)
        {
            if (const auto mode = from_sdl_display_mode(*modes_span[i]))
            {
                list.push_back(*mode);
            }
        }
    }

#endif

    return list;
}

auto GameImpl::display_content_scale(uint32_t display_index) const -> float
{
#ifdef __EMSCRIPTEN__
    return 1.0f;
#else
    return SDL_GetDisplayContentScale(SDL_DisplayID(display_index));
#endif
}

auto GameImpl::display_orientation(uint32_t display_index) const -> DisplayOrientation
{
#ifdef __EMSCRIPTEN__
    const SDL_DisplayOrientation orientation = SDL_GetDisplayOrientation(int(display_index));
#else
    const SDL_DisplayOrientation orientation =
        SDL_GetCurrentDisplayOrientation(SDL_DisplayID(display_index));
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

auto GameImpl::keyboard_focused_window() const -> Window
{
    const auto sdl_window = SDL_GetKeyboardFocus();

    return Window(find_window_by_sdl_window(sdl_window));
}

auto GameImpl::mouse_focused_window() const -> Window
{
    const auto sdl_window = SDL_GetMouseFocus();

    return Window(find_window_by_sdl_window(sdl_window));
}

auto GameImpl::is_audio_device_initialized() const -> bool
{
    return m_audio_device != nullptr;
}

auto GameImpl::graphics_device() -> GraphicsDevice&
{
    if (!m_graphics_device)
    {
        throw std::logic_error{"Attempting to load graphics resources or draw. However, "
                               "no window was created. Please create a window first."};
    }

    return *m_graphics_device;
}

auto GameImpl::audio_device() -> AudioDevice&
{
    if (!m_audio_device)
    {
        throw std::logic_error{
            "No audio engine available. Either no suitable audio device was found, or the "
            "game was not initialized with audio enabled. Please see the enable_audio parameter of "
            "the Game class."};
    }

    return *m_audio_device;
}

void GameImpl::ensure_graphics_device_initialized(WindowImpl& first_window)
{
    if (!m_graphics_device)
    {
        create_graphics_device(first_window);
    }

    assert(m_graphics_device != nullptr && "Graphics device was somehow not created");
}

auto GameImpl::windows() const -> std::span<WindowImpl* const>
{
    return m_windows;
}

auto GameImpl::gamepads() const -> List<Gamepad>
{
    return m_connected_gamepads;
}

void GameImpl::open_initial_gamepads()
{
#ifndef __EMSCRIPTEN__
    assert(m_connected_gamepads.empty());

    int        count                 = 0;
    auto       sdl_joystick_ids      = SDL_GetGamepads(&count);
    const auto sdl_joystick_ids_span = std::span{sdl_joystick_ids, size_t(count)};

    for (const auto joystick_id : sdl_joystick_ids_span)
    {
        if (auto sdl_gamepad = SDL_OpenGamepad(joystick_id))
        {
            auto gamepad_impl = std::make_unique<GamepadImpl>(joystick_id, sdl_gamepad).release();

            m_connected_gamepads.emplace_back(gamepad_impl);
        }
    }
#endif
}

void GameImpl::initialize_imgui()
{
#ifdef CERLIB_ENABLE_IMGUI
    m_imgui_context.reset(ImGui::CreateContext());

    auto& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui::StyleColorsDark();
#endif
}

void GameImpl::create_graphics_device(WindowImpl& first_window)
{
    assert(m_graphics_device == nullptr && "Graphics device is already initialized");

    log_verbose("Initializing device");

    try
    {
#ifdef CERLIB_HAVE_OPENGL
        m_graphics_device = std::make_unique<OpenGLGraphicsDevice>(first_window);
#else
        CER_THROW_RUNTIME_ERROR_STR("OpenGL is not available on this system.");
#endif
    }
    catch ([[maybe_unused]] std::exception& ex)
    {
        log_debug("Device creation failed: {}", ex.what());
        m_graphics_device.reset();
        throw;
    }
}

auto GameImpl::tick() -> bool
{
    if (!m_is_running)
    {
        return false;
    }

    if (!m_has_loaded_content)
    {
        if (m_load_func)
        {
            m_load_func();
        }

        m_has_loaded_content = true;
    }

    process_events();

    if (m_audio_device != nullptr)
    {
        m_audio_device->purge_sounds();
    }

    do_time_measurement();

    bool should_exit = false;

    // Do update()
    if (m_update_func && !m_update_func(m_game_time))
    {
        should_exit = true;
    }

    do_draw();

    m_is_first_tick = false;

    return !should_exit;
}

void GameImpl::process_events()
{
    auto& input_impl = InputImpl::instance();

    const auto mouse_position = current_mouse_position();
    input_impl.set_mouse_position_delta(mouse_position - m_previous_mouse_position);
    m_previous_mouse_position = mouse_position;

    input_impl.set_mouse_wheel_delta({});

    SDL_Event event{};
    while (SDL_PollEvent(&event))
    {
        process_single_event(event, input_impl);
    }

    input_impl.update_key_states();
}

void GameImpl::process_single_event(const SDL_Event& event, InputImpl& input_impl)
{
#ifdef CERLIB_ENABLE_IMGUI
#ifdef __EMSCRIPTEN__
    ImGui_ImplSDL2_ProcessEvent(&event);
#else
    ImGui_ImplSDL3_ProcessEvent(&event);
#endif

    const auto& io = ImGui::GetIO();
#endif

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
                .new_width  = uint32_t(event.window.data1),
                .new_height = uint32_t(event.window.data2),
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
#ifdef CERLIB_ENABLE_IMGUI
            if (io.WantCaptureKeyboard)
            {
                break;
            }
#endif

#ifdef __EMSCRIPTEN__
            const auto [key, modifiers] = InputImpl::from_sdl_keysym(event.key.keysym);
#else
            const auto [key, modifiers] = InputImpl::from_sdl_keysym(event.key.key, event.key.mod);
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
#ifdef CERLIB_ENABLE_IMGUI
            if (io.WantCaptureKeyboard)
            {
                break;
            }
#endif

#ifdef __EMSCRIPTEN__
            const auto [key, modifiers] = InputImpl::from_sdl_keysym(event.key.keysym);
#else
            const auto [key, modifiers] = InputImpl::from_sdl_keysym(event.key.key, event.key.mod);
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
#ifdef CERLIB_ENABLE_IMGUI
            if (io.WantCaptureMouse)
            {
                break;
            }
#endif

            const auto position = Vector2{float(event.motion.x), float(event.motion.y)};
            const auto delta    = Vector2{float(event.motion.xrel), float(event.motion.yrel)};

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
#ifdef CERLIB_ENABLE_IMGUI
            if (io.WantCaptureMouse)
            {
                break;
            }
#endif

            const auto timestamp = event.button.timestamp;
            auto       window    = find_window_by_sdl_window_id(event.button.windowID);
            const auto position  = Vector2{float(event.button.x), float(event.button.y)};
            const auto id        = event.button.which;
            const auto button    = InputImpl::from_sdl_mouse_button(event.button.button);

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
#ifdef CERLIB_ENABLE_IMGUI
            if (io.WantCaptureMouse)
            {
                break;
            }
#endif

            const auto position = Vector2{float(event.wheel.x), float(event.wheel.y)};

#ifdef __EMSCRIPTEN__
            auto delta = Vector2{float(event.wheel.preciseX), float(event.wheel.preciseY)};
#else
            auto delta = Vector2{event.wheel.x, event.wheel.y};
#endif

            if (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED)
            {
                delta = -delta;
            }

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
            const auto sdl_joystick_id = event.jdevice.which;
#else
            const auto sdl_joystick_id = event.gdevice.which;
#endif

            const auto it_existing_gamepad = find_gamepad_by_sdl_joystick_id(sdl_joystick_id);

            if (it_existing_gamepad == m_connected_gamepads.cend())
            {
#ifdef __EMSCRIPTEN__
                const auto sdl_gamepad = SDL_GameControllerOpen(sdl_joystick_id);
#else
                const auto sdl_gamepad = SDL_OpenGamepad(sdl_joystick_id);
#endif

                if (sdl_gamepad != nullptr)
                {
                    auto gamepad_impl =
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
            const auto id = event.cdevice.which;
#else
            const auto id = event.gdevice.which;
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
                    default: break;
                }
                return TouchFingerEventType(-1);
            }();

            if (type != TouchFingerEventType(-1))
            {
                auto       window      = find_window_by_sdl_window_id(event.tfinger.windowID);
                const auto window_size = window.size_px();
                const auto position    = Vector2{event.tfinger.x, event.tfinger.y} * window_size;
                const auto delta       = Vector2{event.tfinger.dx, event.tfinger.dy} * window_size;

                const auto evt = TouchFingerEvent{
                    .type      = type,
                    .timestamp = event.tfinger.timestamp,
                    .window    = std::move(window),
#ifdef __EMSCRIPTEN__
                    .touch_id  = uint64_t(event.tfinger.touchId),
                    .finger_id = uint64_t(event.tfinger.fingerId),
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
        case CER_EVENT_TEXT_INPUT: {
#ifdef CERLIB_ENABLE_IMGUI
            if (io.WantTextInput)
            {
                break;
            }
#endif

            auto window = find_window_by_sdl_window_id(event.text.windowID);

            raise_event(TextInputEvent{
                .timestamp = event.text.timestamp,
                .window    = std::move(window),
                .text      = event.text.text,
            });
        }
        default: break;
    }
}

void GameImpl::do_time_measurement()
{
    const auto current_time   = SDL_GetPerformanceCounter();
    const auto time_frequency = SDL_GetPerformanceFrequency();

    m_game_time.elapsed_time = m_is_first_tick ? 0
                                               : (double(current_time) - double(m_previous_time)) *
                                                     1000 / double(time_frequency) * 0.001;

    m_game_time.total_time += m_game_time.elapsed_time;

    m_previous_time = current_time;
}

void GameImpl::do_draw()
{
    if (m_draw_func)
    {
        for (auto window_impl : m_windows)
        {
            auto window = Window{window_impl};
            m_graphics_device->start_frame(window);

            // Ensure that the frame ends even if an exception is thrown during this frame.
            defer_named(end_frame_guard)
            {
#ifdef CERLIB_ENABLE_IMGUI
                const auto post_draw_callback = [this, &window] {
                    do_imgui_draw(window);
                };
#else
                const auto post_draw_callback = [] {
                };
#endif

                m_graphics_device->end_frame(window, post_draw_callback);
            };

            m_draw_func(window);
        }
    }
}

void GameImpl::do_imgui_draw(const Window& window)
{
#ifdef CERLIB_ENABLE_IMGUI
    if (m_imgui_draw_func)
    {
        m_graphics_device->start_imgui_frame(window);

        // Ensure that the ImGui frame ends even if an exception is thrown during this
        // frame.
        defer_named(end_imgui_frame_guard)
        {
            m_graphics_device->end_imgui_frame(window);
        };

#ifdef __EMSCRIPTEN__
        ImGui_ImplSDL2_NewFrame();
#else
        ImGui_ImplSDL3_NewFrame();
#endif

        ImGui::NewFrame();

        m_imgui_draw_func(window);

        ImGui::Render();
    }
#endif
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

auto GameImpl::find_window_by_sdl_window_id(Uint32 sdl_window_id) const -> Window
{
    auto sdl_window  = SDL_GetWindowFromID(sdl_window_id);
    auto window_impl = find_window_by_sdl_window(sdl_window);

    return Window{window_impl};
}

auto GameImpl::find_window_by_sdl_window(SDL_Window* sdl_window) const -> WindowImpl*
{
    const auto it = std::ranges::find_if(m_windows, [sdl_window](WindowImpl* e) {
        return e->sdl_window() == sdl_window;
    });

    return it != m_windows.cend() ? *it : nullptr;
}

void GameImpl::raise_event(const Event& event)
{
    if (m_event_func)
    {
        m_event_func(event);
    }
}

auto GameImpl::find_gamepad_by_sdl_joystick_id(SDL_JoystickID sdl_joystick_id) const
    -> std::ranges::borrowed_iterator_t<const List<Gamepad>&>
{
    return std::ranges::find_if(m_connected_gamepads, [sdl_joystick_id](const auto& e) {
        return e.impl()->joystick_id() == sdl_joystick_id;
    });
}

auto run_game(int a, char* b[], MainFunc c, void* d) -> int
{
#ifndef __EMSCRIPTEN__
    return SDL_RunApp(a, b, c, d);
#else
    return c(a, b);
#endif
}
} // namespace cer::details
