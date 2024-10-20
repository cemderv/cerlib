// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include <cerlib/Gamepad.hpp>

#include "GamepadImpl.hpp"
#include "InputImpl.hpp"
#include <cerlib/Util2.hpp>

#ifdef __EMSCRIPTEN__
#include <SDL2/SDL.h>
#else
#include <SDL3/SDL.h>
#endif

namespace cer
{
CERLIB_IMPLEMENT_OBJECT(Gamepad);

auto Gamepad::name() const -> std::string_view
{
    DECLARE_THIS_IMPL;
    return impl->name();
}

auto Gamepad::serial_number() const -> std::optional<std::string_view>
{
    DECLARE_THIS_IMPL;
    return impl->serial_number();
}

auto Gamepad::axis_value(GamepadAxis axis) const -> double
{
    DECLARE_THIS_IMPL;
    return impl->axis_value(axis);
}

auto Gamepad::is_button_down(GamepadButton button) const -> bool
{
    DECLARE_THIS_IMPL;
    return impl->is_button_down(button);
}

auto Gamepad::is_button_up(GamepadButton button) const -> bool
{
    return !is_button_down(button);
}

auto Gamepad::sensor_data(GamepadSensorType sensor) const
    -> std::optional<SmallDataArray<float, 16>>
{
    DECLARE_THIS_IMPL;
    return impl->sensor_data(sensor);
}

auto Gamepad::sensor_data_rate(GamepadSensorType sensor) const -> float
{
    DECLARE_THIS_IMPL;
    return impl->sensor_data_rate(sensor);
}

auto Gamepad::steam_handle() const -> std::optional<uint64_t>
{
    DECLARE_THIS_IMPL;
    return impl->steam_handle();
}

auto Gamepad::touchpad_count() const -> uint32_t
{
    DECLARE_THIS_IMPL;
    return impl->touchpad_count();
}

auto Gamepad::touchpad_finger_data(uint32_t touchpad_index) const
    -> SmallDataArray<GamepadTouchpadFingerData, 8>
{
    DECLARE_THIS_IMPL;
    return impl->touchpad_finger_data(touchpad_index);
}

auto Gamepad::type() const -> std::optional<GamepadType>
{
    DECLARE_THIS_IMPL;
    return impl->type();
}

auto Gamepad::set_led_color(const Color& color) -> bool
{
    DECLARE_THIS_IMPL;
    return impl->set_led_color(color);
}

auto Gamepad::start_rumble(float             left_motor_intensity,
                           float             right_motor_intensity,
                           GamepadRumbleTime duration) -> bool
{
    DECLARE_THIS_IMPL;
    return impl->start_rumble(left_motor_intensity, right_motor_intensity, duration);
}

auto Gamepad::has_sensor(GamepadSensorType sensor) const -> bool
{
    DECLARE_THIS_IMPL;
    return impl->has_sensor(sensor);
}

auto Gamepad::is_sensor_enabled(GamepadSensorType sensor) const -> bool
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
