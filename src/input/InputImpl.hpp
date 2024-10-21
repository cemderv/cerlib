// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "cerlib/Input.hpp"
#include "cerlib/Key.hpp"
#include "cerlib/KeyModifier.hpp"
#include "cerlib/Vector2.hpp"

#ifdef __EMSCRIPTEN__
#include <SDL2/SDL.h>
#else
#include <SDL3/SDL.h>
#endif

#include <array>

namespace cer::details
{
class InputImpl final
{
    InputImpl();

  public:
    static auto instance() -> InputImpl&;

    static auto to_sdl_key(Key key) -> int;

    static auto from_sdl_key(SDL_Keycode sdl_key) -> Key;

    static auto to_sdl_mouse_button(MouseButton button) -> int;

    static auto from_sdl_mouse_button(int sdl_button) -> MouseButton;

#ifdef __EMSCRIPTEN__
    static auto from_sdl_keysym(const SDL_Keysym& sdl_keysym) -> std::pair<Key, KeyModifier>;
#else
    static auto from_sdl_keysym(SDL_Keycode sdl_key, SDL_Keymod sdl_mod)
        -> std::pair<Key, KeyModifier>;
#endif

    auto is_key_down(Key key) const -> bool;

    auto was_key_just_pressed(Key key) const -> bool;

    auto was_key_just_released(Key key) const -> bool;

    auto is_mouse_button_down(MouseButton button) const -> bool;

    void update_key_states();

    auto mouse_position_delta() const -> Vector2;

    void set_mouse_position_delta(Vector2 value);

    auto mouse_wheel_delta() const -> Vector2;

    void set_mouse_wheel_delta(Vector2 value);

  private:
    using KeyStateArray = std::array<uint8_t, size_t(Key::EndCall)>;

    KeyStateArray m_previous_key_states;
    KeyStateArray m_key_states;
    Vector2       m_mouse_position_delta;
    Vector2       m_mouse_wheel_delta;
};
} // namespace cer::details
