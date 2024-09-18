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

    SDL_JoystickID joystick_id() const
    {
        return m_joystick_id;
    }

    SDL_Gamepad_t* sdl_gamepad() const
    {
        return m_sdl_gamepad;
    }

    std::string_view name() const;

    std::optional<std::string_view> serial_number() const;

    double axis_value(GamepadAxis axis) const;

    bool is_button_down(GamepadButton button) const;

    std::optional<SmallDataArray<float, 16>> sensor_data(GamepadSensorType sensor) const;

    float sensor_data_rate(GamepadSensorType sensor) const;

    std::optional<uint64_t> steam_handle() const;

    uint32_t touchpad_count() const;

    SmallDataArray<GamepadTouchpadFingerData, 8> touchpad_finger_data(
        uint32_t touchpad_index) const;

    std::optional<GamepadType> type() const;

    bool set_led_color(const Color& color);

    bool start_rumble(float             left_motor_intensity,
                      float             right_motor_intensity,
                      GamepadRumbleTime duration);

    bool has_sensor(GamepadSensorType sensor) const;

    bool is_sensor_enabled(GamepadSensorType sensor) const;

    void set_sensor_enabled(GamepadSensorType sensor, bool enabled);

  private:
    SDL_JoystickID m_joystick_id{};
    SDL_Gamepad_t* m_sdl_gamepad{};
};
} // namespace cer::details
