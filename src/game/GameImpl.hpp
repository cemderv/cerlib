// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "cerlib/Game.hpp"
#include "cerlib/Vector2.hpp"
#include "util/Object.hpp"
#include <cerlib/CopyMoveMacros.hpp>
#include <map>
#include <span>
#include <variant>
#include <vector>

#ifdef __EMSCRIPTEN__
#include <SDL2/SDL.h>
#else
#include <SDL3/SDL.h>
#endif

#ifdef CERLIB_ENABLE_IMGUI
#include <imgui.h>
#endif

namespace cer
{
class Window;
}

namespace cer
{
class AudioDevice;
}

namespace cer::details
{
class GraphicsDevice;
class ContentManager;
class InputImpl;

using Event = std::variant<WindowShownEvent,
                           WindowHiddenEvent,
                           WindowMovedEvent,
                           WindowResizedEvent,
                           WindowMinimizedEvent,
                           WindowMaximizedEvent,
                           WindowGotMouseFocusEvent,
                           WindowLostMouseFocusEvent,
                           WindowGotKeyboardFocusEvent,
                           WindowLostKeyboardFocusEvent,
                           WindowCloseEvent,
                           KeyPressEvent,
                           KeyReleaseEvent,
                           MouseMoveEvent,
                           MouseButtonPressEvent,
                           MouseButtonReleaseEvent,
                           MouseDoubleClickEvent,
                           MouseWheelEvent,
                           TouchFingerEvent,
                           GamepadConnectedEvent,
                           GamepadDisconnectedEvent,
                           TextInputEvent>;

/**
 * Game implementation object.
 */
class GameImpl final : public Object
{
    friend WindowImpl;

  public:
    using LoadFunc = std::function<void()>;

    using UpdateFunc = std::function<bool(const GameTime& time)>;

    using DrawFunc = std::function<void(const Window& window)>;

#ifdef CERLIB_ENABLE_IMGUI
    using ImGuiDrawFunc = std::function<void(const Window& window)>;
#endif

    using EventFunc = std::function<void(const Event& event)>;

    explicit GameImpl(bool enable_audio);

    forbid_copy_and_move(GameImpl);

    ~GameImpl() noexcept override;

    static void init_instance(bool enable_audio);

    static auto instance() -> GameImpl&;

    static auto is_instance_initialized() -> bool;

    static void destroy_instance();

    void run();

    void set_load_func(const LoadFunc& func);

    void set_update_func(const UpdateFunc& func);

    void set_draw_func(const DrawFunc& func);

#ifdef CERLIB_ENABLE_IMGUI
    void set_imgui_draw_func(const ImGuiDrawFunc& func);
#endif

    void set_event_func(const EventFunc& func);

    auto content_manager() -> ContentManager&;

    auto display_count() const -> uint32_t;

    auto current_display_mode(uint32_t display_index) const -> std::optional<DisplayMode>;

    auto display_modes(uint32_t display_index) const -> std::vector<DisplayMode>;

    auto display_content_scale(uint32_t display_index) const -> float;

    auto display_orientation(uint32_t display_index) const -> DisplayOrientation;

    auto keyboard_focused_window() const -> Window;

    auto mouse_focused_window() const -> Window;

    auto is_audio_device_initialized() const -> bool;

    auto graphics_device() -> GraphicsDevice&;

    auto audio_device() -> AudioDevice&;

    void ensure_graphics_device_initialized(WindowImpl& first_window);

    auto windows() const -> std::span<WindowImpl* const>;

    auto gamepads() const -> std::vector<Gamepad>;

  private:
    void open_initial_gamepads();

    void initialize_imgui();

    void create_graphics_device(WindowImpl& first_window);

    auto tick() -> bool;

    void process_events();

    void process_single_event(const SDL_Event& event, InputImpl& input_impl);

    void do_time_measurement();

    void do_draw();

    void do_imgui_draw(const Window& window);

    void notify_window_created(WindowImpl* window);

    void notify_window_destroyed(WindowImpl* window);

    auto find_window_by_sdl_window_id(Uint32 sdl_window_id) const -> Window;

    auto find_window_by_sdl_window(SDL_Window* sdl_window) const -> WindowImpl*;

    void raise_event(const Event& event);

    auto find_gamepad_by_sdl_joystick_id(SDL_JoystickID sdl_joystick_id) const
        -> std::ranges::borrowed_iterator_t<const std::vector<Gamepad>&>;

    bool       m_is_running{};
    bool       m_is_first_tick{true};
    bool       m_has_loaded_content{};
    Uint64     m_previous_time{};
    GameTime   m_game_time;
    LoadFunc   m_load_func;
    UpdateFunc m_update_func;
    DrawFunc   m_draw_func;

#ifdef CERLIB_ENABLE_IMGUI
    struct ImGuiDeleter
    {
        void operator()(ImGuiContext* context) const
        {
            ImGui::DestroyContext(context);
        }
    };

    ImGuiDrawFunc m_imgui_draw_func;

    std::unique_ptr<ImGuiContext, ImGuiDeleter> m_imgui_context;
#endif

    EventFunc                       m_event_func;
    std::unique_ptr<GraphicsDevice> m_graphics_device;
    std::unique_ptr<AudioDevice>    m_audio_device;
    std::unique_ptr<ContentManager> m_content_manager;
    std::vector<WindowImpl*>        m_windows;
    Vector2                         m_previous_mouse_position;
    std::vector<Gamepad>            m_connected_gamepads;
};
} // namespace cer::details
