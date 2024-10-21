// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "cerlib/Gamepad.hpp"
#include "cerlib/Vector2.hpp"
#include "util/Object.hpp"

#ifdef __EMSCRIPTEN__
#include <SDL2/SDL.h>
#else
#include <SDL3/SDL.h>
#endif

#include <array>

namespace cer::details
{
#ifdef __EMSCRIPTEN__
using SDL_Gamepad_t = SDL_GameController;
#else
using SDL_Gamepad_t = SDL_Gamepad;
#endif

class GamepadImpl final : public Object
{
  public:
    explicit GamepadImpl(SDL_JoystickID joystick_id, SDL_Gamepad_t* sdl_gamepad);

    auto joystick_id() const -> SDL_JoystickID
    {
        return m_joystick_id;
    }

    auto sdl_gamepad() const -> SDL_Gamepad_t*
    {
        return m_sdl_gamepad;
    }

    auto name() const -> std::string_view;

    auto serial_number() const -> std::optional<std::string_view>;

    auto axis_value(GamepadAxis axis) const -> double;

    auto is_button_down(GamepadButton button) const -> bool;

    auto sensor_data(GamepadSensorType sensor) const -> std::optional<SmallDataArray<float, 16>>;

    auto sensor_data_rate(GamepadSensorType sensor) const -> float;

    auto steam_handle() const -> std::optional<uint64_t>;

    auto touchpad_count() const -> uint32_t;

    auto touchpad_finger_data(uint32_t touchpad_index) const
        -> SmallDataArray<GamepadTouchpadFingerData, 8>;

    auto type() const -> std::optional<GamepadType>;

    auto set_led_color(const Color& color) -> bool;

    auto start_rumble(float             left_motor_intensity,
                      float             right_motor_intensity,
                      GamepadRumbleTime duration) -> bool;

    auto has_sensor(GamepadSensorType sensor) const -> bool;

    auto is_sensor_enabled(GamepadSensorType sensor) const -> bool;

    void set_sensor_enabled(GamepadSensorType sensor, bool enabled);

  private:
    SDL_JoystickID m_joystick_id{};
    SDL_Gamepad_t* m_sdl_gamepad{};
};
} // namespace cer::details
