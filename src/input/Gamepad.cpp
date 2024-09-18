// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include <cerlib/Gamepad.hpp>

#include "GamepadImpl.hpp"
#include "InputImpl.hpp"
#include "util/Util.hpp"

#ifdef __EMSCRIPTEN__
#include <SDL2/SDL.h>
#else
#include <SDL3/SDL.h>
#endif

namespace cer
{
CERLIB_IMPLEMENT_OBJECT(Gamepad);

std::string_view Gamepad::name() const
{
    DECLARE_THIS_IMPL;
    return impl->name();
}

std::optional<std::string_view> Gamepad::serial_number() const
{
    DECLARE_THIS_IMPL;
    return impl->serial_number();
}

double Gamepad::axis_value(GamepadAxis axis) const
{
    DECLARE_THIS_IMPL;
    return impl->axis_value(axis);
}

bool Gamepad::is_button_down(GamepadButton button) const
{
    DECLARE_THIS_IMPL;
    return impl->is_button_down(button);
}

bool Gamepad::is_button_up(GamepadButton button) const
{
    return !is_button_down(button);
}

std::optional<SmallDataArray<float, 16>> Gamepad::sensor_data(GamepadSensorType sensor) const
{
    DECLARE_THIS_IMPL;
    return impl->sensor_data(sensor);
}

float Gamepad::sensor_data_rate(GamepadSensorType sensor) const
{
    DECLARE_THIS_IMPL;
    return impl->sensor_data_rate(sensor);
}

std::optional<uint64_t> Gamepad::steam_handle() const
{
    DECLARE_THIS_IMPL;
    return impl->steam_handle();
}

uint32_t Gamepad::touchpad_count() const
{
    DECLARE_THIS_IMPL;
    return impl->touchpad_count();
}

SmallDataArray<GamepadTouchpadFingerData, 8> Gamepad::touchpad_finger_data(
    uint32_t touchpad_index) const
{
    DECLARE_THIS_IMPL;
    return impl->touchpad_finger_data(touchpad_index);
}

std::optional<GamepadType> Gamepad::type() const
{
    DECLARE_THIS_IMPL;
    return impl->type();
}

bool Gamepad::set_led_color(const Color& color)
{
    DECLARE_THIS_IMPL;
    return impl->set_led_color(color);
}

bool Gamepad::start_rumble(float             left_motor_intensity,
                           float             right_motor_intensity,
                           GamepadRumbleTime duration)
{
    DECLARE_THIS_IMPL;
    return impl->start_rumble(left_motor_intensity, right_motor_intensity, duration);
}

bool Gamepad::has_sensor(GamepadSensorType sensor) const
{
    DECLARE_THIS_IMPL;
    return impl->has_sensor(sensor);
}

bool Gamepad::is_sensor_enabled(GamepadSensorType sensor) const
{
    DECLARE_THIS_IMPL;
    return impl->is_sensor_enabled(sensor);
}

void Gamepad::set_sensor_enabled(GamepadSensorType sensor, bool enabled)
{
    DECLARE_THIS_IMPL;
    impl->set_sensor_enabled(sensor, enabled);
}
} // namespace cer
