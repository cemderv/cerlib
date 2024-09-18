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
    static InputImpl& instance();

    static int to_sdl_key(Key key);

    static Key from_sdl_key(SDL_Keycode sdl_key);

    static int to_sdl_mouse_button(MouseButton button);

    static MouseButton from_sdl_mouse_button(int sdl_button);

#ifdef __EMSCRIPTEN__
    static std::pair<Key, KeyModifier> from_sdl_keysym(const SDL_Keysym& sdl_keysym);
#else
    static std::pair<Key, KeyModifier> from_sdl_keysym(SDL_Keycode sdl_key, SDL_Keymod sdl_mod);
#endif

    bool is_key_down(Key key) const;

    bool was_key_just_pressed(Key key) const;

    bool was_key_just_released(Key key) const;

    bool is_mouse_button_down(MouseButton button) const;

    void update_key_states();

    Vector2 mouse_position_delta() const;

    void set_mouse_position_delta(Vector2 value);

    Vector2 mouse_wheel_delta() const;

    void set_mouse_wheel_delta(Vector2 value);

  private:
    using KeyStateArray = std::array<uint8_t, static_cast<size_t>(Key::EndCall)>;

    KeyStateArray m_previous_key_states;
    KeyStateArray m_key_states;
    Vector2       m_mouse_position_delta;
    Vector2       m_mouse_wheel_delta;
};
} // namespace cer::details
