// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "cerlib/Game.hpp"
#include "GameImpl.hpp"
#include "util/Util.hpp"

#include <cassert>

// NOLINTBEGIN
#define LOAD_GAME_IMPL auto& impl = details::GameImpl::instance()
// NOLINTEND

namespace cer
{
template <typename... T>
struct EventSwitch : T...
{
    using T::operator()...;
};

template <typename... T>
EventSwitch(T...) -> EventSwitch<T...>;

Game::Game()
    : Game(true)
{
}

Game::Game(bool enable_audio)
{
    details::GameImpl::init_instance(enable_audio);

    details::GameImpl& game_impl = details::GameImpl::instance();

    game_impl.set_load_func([this] { load_content(); });

    game_impl.set_update_func([this](const GameTime& time) { return update(time); });

    game_impl.set_draw_func([this](const Window& window) { draw(window); });

#ifdef CERLIB_ENABLE_IMGUI
    game_impl.set_imgui_draw_func([this](const Window& window) { draw_imgui(window); });
#endif

    game_impl.set_event_func([this](const details::Event& event) {
        std::visit(
            EventSwitch{
                [this](const WindowShownEvent& e) { on_window_shown(e); },
                [this](const WindowHiddenEvent& e) { on_window_hidden(e); },
                [this](const WindowMovedEvent& e) { on_window_moved(e); },
                [this](const WindowResizedEvent& e) { on_window_resized(e); },
                [this](const WindowMinimizedEvent& e) { on_window_minimized(e); },
                [this](const WindowMaximizedEvent& e) { on_window_maximized(e); },
                [this](const WindowGotMouseFocusEvent& e) { on_window_got_mouse_focus(e); },
                [this](const WindowLostMouseFocusEvent& e) { on_window_lost_mouse_focus(e); },
                [this](const WindowGotKeyboardFocusEvent& e) { on_window_got_keyboard_focus(e); },
                [this](const WindowLostKeyboardFocusEvent& e) { on_window_lost_keyboard_focus(e); },
                [this](const WindowCloseEvent& e) { on_window_close(e); },
                [this](const KeyPressEvent& e) { on_key_press(e); },
                [this](const KeyReleaseEvent& e) { on_key_release(e); },
                [this](const MouseMoveEvent& e) { on_mouse_move(e); },
                [this](const MouseButtonPressEvent& e) { on_mouse_button_press(e); },
                [this](const MouseButtonReleaseEvent& e) { on_mouse_button_release(e); },
                [this](const MouseDoubleClickEvent& e) { on_mouse_double_click(e); },
                [this](const MouseWheelEvent& e) { on_mouse_wheel(e); },
                [this](const TouchFingerEvent& e) { on_touch_finger(e); },
                [this](const GamepadConnectedEvent& e) { on_gamepad_connected(e); },
                [this](const GamepadDisconnectedEvent& e) { on_gamepad_disconnected(e); },
                [this](const TextInputEvent& e) { on_text_input(e); },
            },
            event);
    });
}

uint32_t Game::display_count()
{
    LOAD_GAME_IMPL;
    return impl.display_count();
}

std::optional<DisplayMode> Game::current_display_mode(uint32_t display_index)
{
    LOAD_GAME_IMPL;
    return impl.current_display_mode(display_index);
}

std::vector<DisplayMode> Game::display_modes(uint32_t display_index)
{
    LOAD_GAME_IMPL;
    return impl.display_modes(display_index);
}

float Game::display_content_scale(uint32_t display_index)
{
    LOAD_GAME_IMPL;
    return impl.display_content_scale(display_index);
}

DisplayOrientation Game::display_orientation(uint32_t display_index)
{
    LOAD_GAME_IMPL;
    return impl.display_orientation(display_index);
}

std::vector<Gamepad> Game::gamepads()
{
    LOAD_GAME_IMPL;
    return impl.gamepads();
}

Game::~Game() noexcept
{
    details::GameImpl::destroy_instance();
}

void Game::load_content()
{
}

bool Game::update(const GameTime& time)
{
    CERLIB_UNUSED(time);
    return true;
}

void Game::draw(const Window& window)
{
    CERLIB_UNUSED(window);
}

void Game::draw_imgui(const Window& window)
{
    CERLIB_UNUSED(window);
}

void Game::on_window_shown(const WindowShownEvent& event)
{
    CERLIB_UNUSED(event);
}

void Game::on_window_hidden(const WindowHiddenEvent& event)
{
    CERLIB_UNUSED(event);
}

void Game::on_window_moved(const WindowMovedEvent& event)
{
    CERLIB_UNUSED(event);
}

void Game::on_window_resized(const WindowResizedEvent& event)
{
    CERLIB_UNUSED(event);
}

void Game::on_window_minimized(const WindowMinimizedEvent& event)
{
    CERLIB_UNUSED(event);
}

void Game::on_window_maximized(const WindowMaximizedEvent& event)
{
    CERLIB_UNUSED(event);
}

void Game::on_window_got_mouse_focus(const WindowGotMouseFocusEvent& event)
{
    CERLIB_UNUSED(event);
}

void Game::on_window_lost_mouse_focus(const WindowLostMouseFocusEvent& event)
{
    CERLIB_UNUSED(event);
}

void Game::on_window_got_keyboard_focus(const WindowGotKeyboardFocusEvent& event)
{
    CERLIB_UNUSED(event);
}

void Game::on_window_lost_keyboard_focus(const WindowLostKeyboardFocusEvent& event)
{
    CERLIB_UNUSED(event);
}

void Game::on_window_close(const WindowCloseEvent& event)
{
    CERLIB_UNUSED(event);
}

void Game::on_key_press(const KeyPressEvent& event)
{
    CERLIB_UNUSED(event);
}

void Game::on_key_release(const KeyReleaseEvent& event)
{
    CERLIB_UNUSED(event);
}

void Game::on_mouse_move(const MouseMoveEvent& event)
{
    CERLIB_UNUSED(event);
}

void Game::on_mouse_button_press(const MouseButtonPressEvent& event)
{
    CERLIB_UNUSED(event);
}

void Game::on_mouse_button_release(const MouseButtonReleaseEvent& event)
{
    CERLIB_UNUSED(event);
}

void Game::on_mouse_double_click(const MouseDoubleClickEvent& event)
{
    CERLIB_UNUSED(event);
}

void Game::on_mouse_wheel(const MouseWheelEvent& event)
{
    CERLIB_UNUSED(event);
}

void Game::on_touch_finger(const TouchFingerEvent& event)
{
    CERLIB_UNUSED(event);
}

void Game::on_gamepad_connected(const GamepadConnectedEvent& event)
{
    CERLIB_UNUSED(event);
}

void Game::on_gamepad_disconnected(const GamepadDisconnectedEvent& event)
{
    CERLIB_UNUSED(event);
}

void Game::on_text_input(const TextInputEvent& event)
{
    CERLIB_UNUSED(event);
}
} // namespace cer

void cer::details::run_game_internal()
{
    LOAD_GAME_IMPL;
    impl.run();
}
