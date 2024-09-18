// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <cerlib/Export.hpp>
#include <cerlib/SmallDataArray.hpp>
#include <cerlib/Vector2.hpp>
#include <chrono>

namespace cer
{
struct Color;

namespace details
{
class GamepadImpl;
}

/**
 * Defines the axis of a gamepad.
 *
 * @ingroup Input
 */
enum class GamepadAxis
{
    /** An unknown axis */
    Unknown = 0,

    /** The left X axis */
    LeftX = 1,

    /** The left Y axis */
    LeftY = 2,

    /** The right X axis */
    RightX = 3,

    /** The right Y axis */
    RightY = 4,

    /** The left trigger axis */
    LeftTrigger = 5,

    /** The right trigger axis */
    RightTrigger = 6,
};

/**
 * Defines the button of a gamepad.
 *
 * @ingroup Input
 */
enum class GamepadButton
{
    Unknown       = 0, /** */
    ActionSouth   = 1, /** */
    ActionEast    = 2, /** */
    Back          = 3, /** */
    DpadDown      = 4, /** */
    DpadLeft      = 5, /** */
    DpadRight     = 6, /** */
    DpadUp        = 7, /** */
    Guide         = 8, /** */
    LeftShoulder  = 9, /** */
    LeftStick     = 10, /** */
    Misc          = 11, /** */
    LeftPaddle1   = 12, /** */
    LeftPaddle2   = 13, /** */
    RightPaddle1  = 14, /** */
    RightPaddle2  = 15, /** */
    RightShoulder = 16, /** */
    RightStick    = 17, /** */
    Start         = 18, /** */
    Touchpad      = 19, /** */
    ActionWest    = 20, /** */
    ActionNorth   = 21, /** */
};

/**
 * Defines the type of a gamepad.
 *
 * @ingroup Input
 */
enum class GamepadType
{
    Standard                    = 1, /** */
    NintendoSwitchJoyconLeft    = 2, /** */
    NintendoSwitchJoyconRight   = 3, /** */
    NintendoSwitchJoyconPair    = 4, /** */
    NintendoSwitchProController = 5, /** */
    Playstation3                = 6, /** */
    Playstation4                = 7, /** */
    Playstation5                = 8, /** */
    Xbox360                     = 9, /** */
    XboxOne                     = 10, /** */
};

/**
 * Defines the type of a gamepad's sensor.
 *
 * @ingroup Input
 */
enum class GamepadSensorType
{
    Unknown           = 0, /** */
    Acceleration      = 1, /** */
    Gyroscope         = 2, /** */
    AccelerationLeft  = 3, /** */
    GyroscopeLeft     = 4, /** */
    AccelerationRight = 5, /** */
    GyroscopeRight    = 6, /** */
};

/**
 * Represents context-specific data of a gamepad's sensor.
 *
 * @ingroup Input
 */
using GamepadSensorData = SmallDataArray<float, 16>;

/**
 * Represents information about a single finger touch
 * on a gamepad's touchpad.
 *
 * @ingroup Input
 */
struct GamepadTouchpadFingerData final
{
    /** Default comparison */
    bool operator==(const GamepadTouchpadFingerData&) const = default;

    /** Default comparison */
    bool operator!=(const GamepadTouchpadFingerData&) const = default;

    /** The index of the touch. */
    int32_t index = 0;

    /** The position of the touch, normalized to the range [0.0 .. 1.0]. */
    Vector2 position{};

    /** The pressure of the touch. */
    float pressure = 0.0f;
};

/**
 * Represents the duration of a gamepad's rumble effect.
 *
 * @ingroup Input
 */
using GamepadRumbleTime = std::chrono::duration<double>;

/**
 * Represents a connected gamepad.
 *
 * A gamepad is obtained from `Game::gamepads()` and / or the `Game::on_gamepad_connected()`
 * function callback. If a gamepad is disconnected from the system, the
 * `Game::on_gamepad_disconnected()` function callback is called.
 *
 * @ingroup Input
 */
class CERLIB_API Gamepad final
{
    CERLIB_DECLARE_OBJECT(Gamepad);

  public:
    /**
     * Gets the name of the gamepad as provided by the vendor.
     */
    std::string_view name() const;

    /**
     * Gets the serial number of the gamepad, if available.
     */
    std::optional<std::string_view> serial_number() const;

    /**
     * Gets the normalized value of a specific axis on the gamepad.
     *
     * @param axis The axis to query.
     * @return The normalized value of the axis, in the range `[0.0 .. 1.0]`.
     */
    double axis_value(GamepadAxis axis) const;

    /**
     * Gets a value indicating whether a specific button is currently pressed on the gamepad.
     *
     * @param button The button to query.
     * @return True if the button is currently pressed; false otherwise.
     */
    bool is_button_down(GamepadButton button) const;

    /**
     * Gets a value indicating whether a specific button is currently *not* pressed on the
     * gamepad.
     *
     * @param button The button to query.
     * @return True if the button is currently not pressed; false otherwise.
     */
    bool is_button_up(GamepadButton button) const;

    /**
     * Gets the current data of a specific sensor on the gamepad.
     *
     * @param sensor The sensor to query.
     */
    std::optional<SmallDataArray<float, 16>> sensor_data(GamepadSensorType sensor) const;

    /**
     * Gets the data rate (number of events per second) of a specific sensor on the gamepad.
     *
     * @param sensor The sensor to query.
     */
    float sensor_data_rate(GamepadSensorType sensor) const;

    /**
     * Gets the Steam Input handle of the gamepad, if available.
     *
     * @return A InputHandle_t that can be used with the Steam Input API:
     * https://partner.steamgames.com/doc/api/ISteamInput
     */
    std::optional<uint64_t> steam_handle() const;

    /**
     * Gets the number of touchpads available on the gamepad.
     */
    uint32_t touchpad_count() const;

    /**
     * Gets the state of currently pressed touches on the gamepad's touchpad.
     * Each entry in the array represents a touch.
     *
     * @param touchpad_index The index of the touchpad to query. To obtain the number of touchpads
     * on the gamepad, use the Gamepad::touchpad_count() method.
     */
    SmallDataArray<GamepadTouchpadFingerData, 8> touchpad_finger_data(
        uint32_t touchpad_index) const;

    /**
     * Gets the type of the gamepad.
     */
    std::optional<GamepadType> type() const;

    /**
     * If supported by the gamepad, changes its LED color.
     *
     * @param color The color to change the LED to.
     *
     * @return True if the LED was successfully changed; false otherwise.
     */
    bool set_led_color(const Color& color);

    /**
     * Starts the rumble motors on the gamepad, if supported by the gamepad, for
     * a specific duration.
     *
     * @param left_motor_intensity The intensity of the left motor, in the range
     * `[0.0 .. 1.0]`
     * @param right_motor_intensity The intensity of the right motor, in the range
     * `[0.0 .. 1.0]`
     * @param duration The duration of the rumble
     *
     * @return True if the rumble was started successfully; false otherwise (for example,
     * if the gamepad does not support rumble effects).
     *
     * @remark Passing 0 as the motor intensity will stop the current rumble. Calling this
     * function will stop any rumble that was previously in effect.
     *
     * Example:
     * @code{.cpp}
     * using namespace std::chrono_literals;
     * // ...
     * my_gamepad.start_rumble(0.5f, 1.0f, 500ms);
     * @endcode
     */
    bool start_rumble(float             left_motor_intensity,
                      float             right_motor_intensity,
                      GamepadRumbleTime duration);

    /**
     * Gets a value indicating whether the gamepad supports a specific sensor.
     *
     * @param sensor The sensor to check for availability.
     * @return True if the gamepad supports the sensor; false otherwise.
     */
    bool has_sensor(GamepadSensorType sensor) const;

    /**
     * Gets a value indicating whether a specific sensor on the gamepad is currently enabled.
     *
     * @param sensor The sensor to check.
     * @return True if the sensor is currently enabled; false otherwise.
     */
    bool is_sensor_enabled(GamepadSensorType sensor) const;

    /**
     * Enables or disables a specific sensor on the gamepad.
     *
     * @param sensor The sensor to enable or disable.
     * @param enabled If true, enables the sensor; false disables it.
     */
    void set_sensor_enabled(GamepadSensorType sensor, bool enabled);
};
} // namespace cer
