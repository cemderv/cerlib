// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "cerlib/Game.hpp"
#include "GameImpl.hpp"
#include <cassert>
#include <cerlib/Util2.hpp>

// NOLINTBEGIN
#define LOAD_GAME_IMPL auto& impl = details::GameImpl::instance()
// NOLINTEND

namespace cer
{
Game::Game()
    : Game(true)
{
}

Game::Game(bool enable_audio)
{
    details::GameImpl::init_instance(enable_audio);

    auto& game_impl = details::GameImpl::instance();

    game_impl.set_load_func([this] {
        load_content();
    });

    game_impl.set_update_func([this](const GameTime& time) {
        return update(time);
    });

    game_impl.set_draw_func([this](const Window& window) {
        draw(window);
    });

#ifdef CERLIB_ENABLE_IMGUI
    game_impl.set_imgui_draw_func([this](const Window& window) {
        draw_imgui(window);
    });
#endif

    game_impl.set_event_func([this](const details::Event& event) {
        std::visit(VariantSwitch{
                       [this](const WindowShownEvent& e) {
                           on_window_shown(e);
                       },
                       [this](const WindowHiddenEvent& e) {
                           on_window_hidden(e);
                       },
                       [this](const WindowMovedEvent& e) {
                           on_window_moved(e);
                       },
                       [this](const WindowResizedEvent& e) {
                           on_window_resized(e);
                       },
                       [this](const WindowMinimizedEvent& e) {
                           on_window_minimized(e);
                       },
                       [this](const WindowMaximizedEvent& e) {
                           on_window_maximized(e);
                       },
                       [this](const WindowGotMouseFocusEvent& e) {
                           on_window_got_mouse_focus(e);
                       },
                       [this](const WindowLostMouseFocusEvent& e) {
                           on_window_lost_mouse_focus(e);
                       },
                       [this](const WindowGotKeyboardFocusEvent& e) {
                           on_window_got_keyboard_focus(e);
                       },
                       [this](const WindowLostKeyboardFocusEvent& e) {
                           on_window_lost_keyboard_focus(e);
                       },
                       [this](const WindowCloseEvent& e) {
                           on_window_close(e);
                       },
                       [this](const KeyPressEvent& e) {
                           on_key_press(e);
                       },
                       [this](const KeyReleaseEvent& e) {
                           on_key_release(e);
                       },
                       [this](const MouseMoveEvent& e) {
                           on_mouse_move(e);
                       },
                       [this](const MouseButtonPressEvent& e) {
                           on_mouse_button_press(e);
                       },
                       [this](const MouseButtonReleaseEvent& e) {
                           on_mouse_button_release(e);
                       },
                       [this](const MouseDoubleClickEvent& e) {
                           on_mouse_double_click(e);
                       },
                       [this](const MouseWheelEvent& e) {
                           on_mouse_wheel(e);
                       },
                       [this](const TouchFingerEvent& e) {
                           on_touch_finger(e);
                       },
                       [this](const GamepadConnectedEvent& e) {
                           on_gamepad_connected(e);
                       },
                       [this](const GamepadDisconnectedEvent& e) {
                           on_gamepad_disconnected(e);
                       },
                       [this](const TextInputEvent& e) {
                           on_text_input(e);
                       },
                   },
                   event);
    });
}

auto Game::display_count() -> uint32_t
{
    LOAD_GAME_IMPL;
    return impl.display_count();
}

auto Game::current_display_mode(uint32_t display_index) -> std::optional<DisplayMode>
{
    LOAD_GAME_IMPL;
    return impl.current_display_mode(display_index);
}

auto Game::display_modes(uint32_t display_index) -> List<DisplayMode>
{
    LOAD_GAME_IMPL;
    return impl.display_modes(display_index);
}

auto Game::display_content_scale(uint32_t display_index) -> float
{
    LOAD_GAME_IMPL;
    return impl.display_content_scale(display_index);
}

auto Game::display_orientation(uint32_t display_index) -> DisplayOrientation
{
    LOAD_GAME_IMPL;
    return impl.display_orientation(display_index);
}

auto Game::gamepads() -> List<Gamepad>
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

auto Game::update([[maybe_unused]] const GameTime& time) -> bool
{
    return true;
}

void Game::draw([[maybe_unused]] const Window& window)
{
}

void Game::draw_imgui([[maybe_unused]] const Window& window)
{
}

void Game::on_window_shown([[maybe_unused]] const WindowShownEvent& event)
{
}

void Game::on_window_hidden([[maybe_unused]] const WindowHiddenEvent& event)
{
}

void Game::on_window_moved([[maybe_unused]] const WindowMovedEvent& event)
{
}

void Game::on_window_resized([[maybe_unused]] const WindowResizedEvent& event)
{
}

void Game::on_window_minimized([[maybe_unused]] const WindowMinimizedEvent& event)
{
}

void Game::on_window_maximized([[maybe_unused]] const WindowMaximizedEvent& event)
{
}

void Game::on_window_got_mouse_focus([[maybe_unused]] const WindowGotMouseFocusEvent& event)
{
}

void Game::on_window_lost_mouse_focus([[maybe_unused]] const WindowLostMouseFocusEvent& event)
{
}

void Game::on_window_got_keyboard_focus([[maybe_unused]] const WindowGotKeyboardFocusEvent& event)
{
}

void Game::on_window_lost_keyboard_focus([[maybe_unused]] const WindowLostKeyboardFocusEvent& event)
{
}

void Game::on_window_close([[maybe_unused]] const WindowCloseEvent& event)
{
}

void Game::on_key_press([[maybe_unused]] const KeyPressEvent& event)
{
}

void Game::on_key_release([[maybe_unused]] const KeyReleaseEvent& event)
{
}

void Game::on_mouse_move([[maybe_unused]] const MouseMoveEvent& event)
{
}

void Game::on_mouse_button_press([[maybe_unused]] const MouseButtonPressEvent& event)
{
}

void Game::on_mouse_button_release([[maybe_unused]] const MouseButtonReleaseEvent& event)
{
}

void Game::on_mouse_double_click([[maybe_unused]] const MouseDoubleClickEvent& event)
{
}

void Game::on_mouse_wheel([[maybe_unused]] const MouseWheelEvent& event)
{
}

void Game::on_touch_finger([[maybe_unused]] const TouchFingerEvent& event)
{
}

void Game::on_gamepad_connected([[maybe_unused]] const GamepadConnectedEvent& event)
{
}

void Game::on_gamepad_disconnected([[maybe_unused]] const GamepadDisconnectedEvent& event)
{
}

void Game::on_text_input([[maybe_unused]] const TextInputEvent& event)
{
}
} // namespace cer

void cer::details::run_game_internal()
{
    LOAD_GAME_IMPL;
    impl.run();
}
