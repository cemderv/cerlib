// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "GamepadImpl.hpp"

#include "cerlib/Color.hpp"
#include "cerlib/KeyModifier.hpp"
#include "cerlib/MouseButton.hpp"

#include <cassert>

#ifdef __EMSCRIPTEN__
#define SDL_SCANCODE_MEDIA_NEXT_TRACK     SDL_SCANCODE_AUDIONEXT
#define SDL_SCANCODE_MEDIA_PREVIOUS_TRACK SDL_SCANCODE_AUDIOPREV
#define SDL_SCANCODE_MEDIA_STOP           SDL_SCANCODE_AUDIOSTOP
#define SDL_SCANCODE_MEDIA_PLAY           SDL_SCANCODE_AUDIOPLAY
#define SDL_SCANCODE_MEDIA_EJECT          SDL_SCANCODE_EJECT
#define SDL_SCANCODE_MEDIA_REWIND         SDL_SCANCODE_AUDIOREWIND
#define SDL_SCANCODE_MEDIA_FAST_FORWARD   SDL_SCANCODE_AUDIOFASTFORWARD
#endif

namespace cer::details
{
#ifdef __EMSCRIPTEN__
using SDL_GamepadAxis = SDL_GameControllerAxis;
#define SDL_GAMEPAD_AXIS_MAX    SDL_CONTROLLER_AXIS_MAX
#define SDL_GAMEPAD_AXIS_LEFTX  SDL_CONTROLLER_AXIS_LEFTX
#define SDL_GAMEPAD_AXIS_LEFTY  SDL_CONTROLLER_AXIS_LEFTY
#define SDL_GAMEPAD_AXIS_RIGHTX SDL_CONTROLLER_AXIS_RIGHTX
#define SDL_GAMEPAD_AXIS_RIGHTY SDL_CONTROLLER_AXIS_RIGHTY
#endif

static SDL_GamepadAxis to_sdl_gamepad_axis(GamepadAxis axis)
{
    switch (axis)
    {
        case GamepadAxis::Unknown: return SDL_GAMEPAD_AXIS_MAX;
        case GamepadAxis::LeftX: return SDL_GAMEPAD_AXIS_LEFTX;
        case GamepadAxis::LeftY: return SDL_GAMEPAD_AXIS_LEFTY;
        case GamepadAxis::RightX: return SDL_GAMEPAD_AXIS_RIGHTX;
        case GamepadAxis::RightY: return SDL_GAMEPAD_AXIS_RIGHTY;
#ifdef __EMSCRIPTEN__
        case GamepadAxis::LeftTrigger:
        case GamepadAxis::RightTrigger: return SDL_GAMEPAD_AXIS_MAX;
#else
        case GamepadAxis::LeftTrigger: return SDL_GAMEPAD_AXIS_LEFT_TRIGGER;
        case GamepadAxis::RightTrigger: return SDL_GAMEPAD_AXIS_RIGHT_TRIGGER;
#endif
    }

    return SDL_GAMEPAD_AXIS_MAX;
}

#ifdef __EMSCRIPTEN__
using SDL_GamepadButton = SDL_GameControllerButton;
#define SDL_GAMEPAD_BUTTON_MAX            SDL_CONTROLLER_BUTTON_MAX
#define SDL_GAMEPAD_BUTTON_SOUTH          SDL_CONTROLLER_BUTTON_A
#define SDL_GAMEPAD_BUTTON_EAST           SDL_CONTROLLER_BUTTON_B
#define SDL_GAMEPAD_BUTTON_BACK           SDL_CONTROLLER_BUTTON_BACK
#define SDL_GAMEPAD_BUTTON_DPAD_DOWN      SDL_CONTROLLER_BUTTON_DPAD_DOWN
#define SDL_GAMEPAD_BUTTON_DPAD_UP        SDL_CONTROLLER_BUTTON_DPAD_UP
#define SDL_GAMEPAD_BUTTON_DPAD_LEFT      SDL_CONTROLLER_BUTTON_DPAD_LEFT
#define SDL_GAMEPAD_BUTTON_DPAD_RIGHT     SDL_CONTROLLER_BUTTON_DPAD_RIGHT
#define SDL_GAMEPAD_BUTTON_GUIDE          SDL_CONTROLLER_BUTTON_GUIDE
#define SDL_GAMEPAD_BUTTON_LEFT_SHOULDER  SDL_CONTROLLER_BUTTON_LEFTSHOULDER
#define SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER SDL_CONTROLLER_BUTTON_RIGHTSHOULDER
#define SDL_GAMEPAD_BUTTON_LEFT_STICK     SDL_CONTROLLER_BUTTON_LEFTSTICK
#define SDL_GAMEPAD_BUTTON_RIGHT_STICK    SDL_CONTROLLER_BUTTON_RIGHTSTICK
#define SDL_GAMEPAD_BUTTON_MISC1          SDL_CONTROLLER_BUTTON_MISC1
#define SDL_GAMEPAD_BUTTON_LEFT_PADDLE1   SDL_CONTROLLER_BUTTON_PADDLE1
#define SDL_GAMEPAD_BUTTON_LEFT_PADDLE2   SDL_CONTROLLER_BUTTON_PADDLE2
#define SDL_GAMEPAD_BUTTON_RIGHT_PADDLE1  SDL_CONTROLLER_BUTTON_PADDLE3
#define SDL_GAMEPAD_BUTTON_RIGHT_PADDLE2  SDL_CONTROLLER_BUTTON_PADDLE4
#define SDL_GAMEPAD_BUTTON_START          SDL_CONTROLLER_BUTTON_START
#define SDL_GAMEPAD_BUTTON_TOUCHPAD       SDL_CONTROLLER_BUTTON_TOUCHPAD
#define SDL_GAMEPAD_BUTTON_WEST           SDL_CONTROLLER_BUTTON_X
#define SDL_GAMEPAD_BUTTON_NORTH          SDL_CONTROLLER_BUTTON_Y
#endif

static SDL_GamepadButton to_sdl_gamepad_button(GamepadButton button)
{
    switch (button)
    {
        case GamepadButton::Unknown: return SDL_GAMEPAD_BUTTON_MAX;
        case GamepadButton::ActionSouth: return SDL_GAMEPAD_BUTTON_SOUTH;
        case GamepadButton::ActionEast: return SDL_GAMEPAD_BUTTON_EAST;
        case GamepadButton::Back: return SDL_GAMEPAD_BUTTON_BACK;
        case GamepadButton::DpadDown: return SDL_GAMEPAD_BUTTON_DPAD_DOWN;
        case GamepadButton::DpadLeft: return SDL_GAMEPAD_BUTTON_DPAD_LEFT;
        case GamepadButton::DpadRight: return SDL_GAMEPAD_BUTTON_DPAD_RIGHT;
        case GamepadButton::DpadUp: return SDL_GAMEPAD_BUTTON_DPAD_UP;
        case GamepadButton::Guide: return SDL_GAMEPAD_BUTTON_GUIDE;
        case GamepadButton::LeftShoulder: return SDL_GAMEPAD_BUTTON_LEFT_SHOULDER;
        case GamepadButton::LeftStick: return SDL_GAMEPAD_BUTTON_LEFT_STICK;
        case GamepadButton::Misc: return SDL_GAMEPAD_BUTTON_MISC1;
        case GamepadButton::LeftPaddle1: return SDL_GAMEPAD_BUTTON_LEFT_PADDLE1;
        case GamepadButton::LeftPaddle2: return SDL_GAMEPAD_BUTTON_LEFT_PADDLE2;
        case GamepadButton::RightPaddle1: return SDL_GAMEPAD_BUTTON_RIGHT_PADDLE1;
        case GamepadButton::RightPaddle2: return SDL_GAMEPAD_BUTTON_RIGHT_PADDLE2;
        case GamepadButton::RightShoulder: return SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER;
        case GamepadButton::RightStick: return SDL_GAMEPAD_BUTTON_RIGHT_STICK;
        case GamepadButton::Start: return SDL_GAMEPAD_BUTTON_START;
        case GamepadButton::Touchpad: return SDL_GAMEPAD_BUTTON_TOUCHPAD;
        case GamepadButton::ActionWest: return SDL_GAMEPAD_BUTTON_WEST;
        case GamepadButton::ActionNorth: return SDL_GAMEPAD_BUTTON_NORTH;
    }

    return SDL_GAMEPAD_BUTTON_MAX;
}

#ifdef __EMSCRIPTEN__
using SDL_GamepadType = SDL_GameControllerType;
#define SDL_GAMEPAD_TYPE_UNKNOWN                     SDL_CONTROLLER_TYPE_UNKNOWN
#define SDL_GAMEPAD_TYPE_MAX                         SDL_CONTROLLER_TYPE_UNKNOWN
#define SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_LEFT SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_LEFT
#define SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT                                              \
    SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT
#define SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_PAIR SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_PAIR
#define SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_PRO         SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_PRO
#define SDL_GAMEPAD_TYPE_PS3                         SDL_CONTROLLER_TYPE_PS3
#define SDL_GAMEPAD_TYPE_PS4                         SDL_CONTROLLER_TYPE_PS4
#define SDL_GAMEPAD_TYPE_PS5                         SDL_CONTROLLER_TYPE_PS5
#define SDL_GAMEPAD_TYPE_XBOX360                     SDL_CONTROLLER_TYPE_XBOX360
#define SDL_GAMEPAD_TYPE_XBOXONE                     SDL_CONTROLLER_TYPE_XBOXONE
#endif

[[maybe_unused]] static SDL_GamepadType to_sdl_gamepad_type(GamepadType type)
{
    switch (type)
    {
        case GamepadType::NintendoSwitchJoyconLeft:
            return SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_LEFT;
        case GamepadType::NintendoSwitchJoyconRight:
            return SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT;
        case GamepadType::NintendoSwitchJoyconPair:
            return SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_PAIR;
        case GamepadType::NintendoSwitchProController: return SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_PRO;
        case GamepadType::Playstation3: return SDL_GAMEPAD_TYPE_PS3;
        case GamepadType::Playstation4: return SDL_GAMEPAD_TYPE_PS4;
        case GamepadType::Playstation5: return SDL_GAMEPAD_TYPE_PS5;
        case GamepadType::Xbox360: return SDL_GAMEPAD_TYPE_XBOX360;
        case GamepadType::XboxOne: return SDL_GAMEPAD_TYPE_XBOXONE;
        default: break;
    }

    return SDL_GAMEPAD_TYPE_UNKNOWN;
}

static SDL_SensorType to_sdl_gamepad_sensor_type(GamepadSensorType type)
{
    switch (type)
    {
        case GamepadSensorType::Unknown: return SDL_SENSOR_UNKNOWN;
        case GamepadSensorType::Acceleration: return SDL_SENSOR_ACCEL;
        case GamepadSensorType::Gyroscope: return SDL_SENSOR_GYRO;
        case GamepadSensorType::AccelerationLeft: return SDL_SENSOR_ACCEL_L;
        case GamepadSensorType::GyroscopeLeft: return SDL_SENSOR_GYRO_L;
        case GamepadSensorType::AccelerationRight: return SDL_SENSOR_ACCEL_R;
        case GamepadSensorType::GyroscopeRight: return SDL_SENSOR_GYRO_R;
    }

    return SDL_SENSOR_UNKNOWN;
}

GamepadImpl::GamepadImpl(SDL_JoystickID joystick_id, SDL_Gamepad_t* sdl_gamepad)
    : m_joystick_id(joystick_id)
    , m_sdl_gamepad(sdl_gamepad)
{
}

std::string_view GamepadImpl::name() const
{
#ifdef __EMSCRIPTEN__
    return SDL_GameControllerName(m_sdl_gamepad);
#else
    return SDL_GetGamepadName(m_sdl_gamepad);
#endif
}

std::optional<std::string_view> GamepadImpl::serial_number() const
{
#ifdef __EMSCRIPTEN__
    const auto serial = SDL_GameControllerGetSerial(m_sdl_gamepad);
#else
    const auto serial = SDL_GetGamepadSerial(m_sdl_gamepad);
#endif

    return serial ? std::string_view(serial) : std::optional<std::string_view>(); // NOLINT
}

double GamepadImpl::axis_value(GamepadAxis axis) const
{
#ifdef __EMSCRIPTEN__
    const auto value = SDL_GameControllerGetAxis(m_sdl_gamepad, to_sdl_gamepad_axis(axis));
#else
    const auto value = SDL_GetGamepadAxis(m_sdl_gamepad, to_sdl_gamepad_axis(axis));
#endif

    return value < 0 ? static_cast<double>(value) / 32768 : static_cast<double>(value) / 32767;
}

bool GamepadImpl::is_button_down(GamepadButton button) const
{
#ifdef __EMSCRIPTEN__
    const auto state = SDL_GameControllerGetButton(m_sdl_gamepad, to_sdl_gamepad_button(button));
#else
    const auto state = SDL_GetGamepadButton(m_sdl_gamepad, to_sdl_gamepad_button(button));
#endif

    return state != 0;
}

std::optional<SmallDataArray<float, 16>> GamepadImpl::sensor_data(GamepadSensorType sensor) const
{
    auto result = SmallDataArray<float, 16>();

#ifdef __EMSCRIPTEN__
    const auto success = SDL_GameControllerGetSensorData(m_sdl_gamepad,
                                                         to_sdl_gamepad_sensor_type(sensor),
                                                         result.begin(),
                                                         static_cast<int>(result.size()));
#else
    const auto success = SDL_GetGamepadSensorData(m_sdl_gamepad,
                                                  to_sdl_gamepad_sensor_type(sensor),
                                                  result.begin(),
                                                  static_cast<int>(result.size()));
#endif

    return success == 0 ? result : std::optional<SmallDataArray<float, 16>>(); // NOLINT
}

float GamepadImpl::sensor_data_rate(GamepadSensorType sensor) const
{
#ifdef __EMSCRIPTEN__
    return SDL_GameControllerGetSensorDataRate(m_sdl_gamepad, to_sdl_gamepad_sensor_type(sensor));
#else
    return SDL_GetGamepadSensorDataRate(m_sdl_gamepad, to_sdl_gamepad_sensor_type(sensor));
#endif
}

std::optional<uint64_t> GamepadImpl::steam_handle() const
{
#ifdef __EMSCRIPTEN__
    return std::nullopt;
#else
    return SDL_GetGamepadSteamHandle(m_sdl_gamepad);
#endif
}

uint32_t GamepadImpl::touchpad_count() const
{
#ifdef __EMSCRIPTEN__
    return static_cast<uint32_t>(SDL_GameControllerGetNumTouchpads(m_sdl_gamepad));
#else
    return static_cast<uint32_t>(SDL_GetNumGamepadTouchpads(m_sdl_gamepad));
#endif
}

SmallDataArray<GamepadTouchpadFingerData, 8> GamepadImpl::touchpad_finger_data(
    uint32_t touchpad_index) const
{
    const auto sdl_touchpad_index = static_cast<int>(touchpad_index);

#ifdef __EMSCRIPTEN__
    const auto count = SDL_GameControllerGetNumTouchpadFingers(m_sdl_gamepad, sdl_touchpad_index);
#else
    const auto count = SDL_GetNumGamepadTouchpadFingers(m_sdl_gamepad, sdl_touchpad_index);
#endif

    auto result = SmallDataArray<GamepadTouchpadFingerData, 8>(static_cast<uint32_t>(count));

    for (int i = 0; i < count; ++i)
    {
        Uint8 state    = 0;
        auto  x        = 0.0f;
        auto  y        = 0.0f;
        auto  pressure = 0.0f;

#ifdef __EMSCRIPTEN__
        if (SDL_GameControllerGetTouchpadFinger(m_sdl_gamepad,
                                                sdl_touchpad_index,
                                                i,
                                                &state,
                                                &x,
                                                &y,
                                                &pressure) == SDL_TRUE)
#else
        if (SDL_GetGamepadTouchpadFinger(m_sdl_gamepad,
                                         sdl_touchpad_index,
                                         i,
                                         &state,
                                         &x,
                                         &y,
                                         &pressure))
#endif
        {
            result[i] = GamepadTouchpadFingerData{
                .index    = i,
                .position = {x, y},
                .pressure = pressure,
            };
        }
    }

    return result;
}

static std::optional<GamepadType> from_sdl_gamepad_type(SDL_GamepadType type)
{
    switch (type)
    {
#ifndef __EMSCRIPTEN__
        case SDL_GAMEPAD_TYPE_STANDARD: return GamepadType::Standard;
#endif
        case SDL_GAMEPAD_TYPE_XBOX360: return GamepadType::Xbox360;
        case SDL_GAMEPAD_TYPE_XBOXONE: return GamepadType::XboxOne;
        case SDL_GAMEPAD_TYPE_PS3: return GamepadType::Playstation3;
        case SDL_GAMEPAD_TYPE_PS4: return GamepadType::Playstation4;
        case SDL_GAMEPAD_TYPE_PS5: return GamepadType::Playstation5;
        case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_PRO: return GamepadType::NintendoSwitchProController;
        case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_LEFT:
            return GamepadType::NintendoSwitchJoyconLeft;
        case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT:
            return GamepadType::NintendoSwitchJoyconRight;
        case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_PAIR:
            return GamepadType::NintendoSwitchJoyconPair;
        default: break;
    }

    return std::nullopt;
}

std::optional<GamepadType> GamepadImpl::type() const
{
#ifdef __EMSCRIPTEN__
    const auto sdl_gamepad_type = SDL_GameControllerGetType(m_sdl_gamepad);
#else
    const auto sdl_gamepad_type = SDL_GetGamepadType(m_sdl_gamepad);
#endif
    return from_sdl_gamepad_type(sdl_gamepad_type);
}

bool GamepadImpl::set_led_color(const Color& color)
{
    const Uint8 r =
        clamp(static_cast<Uint8>(color.r / 255.0f), static_cast<Uint8>(0), static_cast<Uint8>(255));

    const Uint8 g =
        clamp(static_cast<Uint8>(color.g / 255.0f), static_cast<Uint8>(0), static_cast<Uint8>(255));

    const Uint8 b =
        clamp(static_cast<Uint8>(color.b / 255.0f), static_cast<Uint8>(0), static_cast<Uint8>(255));

#ifdef __EMSCRIPTEN__
    return SDL_GameControllerSetLED(m_sdl_gamepad, r, g, b) == SDL_TRUE;
#else
    return SDL_SetGamepadLED(m_sdl_gamepad, r, g, b);
#endif
}

bool GamepadImpl::start_rumble(float             left_motor_intensity,
                               float             right_motor_intensity,
                               GamepadRumbleTime duration)
{
    const auto normalized_left_motor_intensity =
        is_zero(left_motor_intensity)
            ? static_cast<Uint16>(0)
            : static_cast<Uint16>(clamp(left_motor_intensity, 0.0f, 1.0f) *
                                  static_cast<float>(0xFFFF));

    const auto normalized_right_motor_intensity =
        is_zero(right_motor_intensity)
            ? static_cast<Uint16>(0)
            : static_cast<Uint16>(clamp(right_motor_intensity, 0.0f, 1.0f) *
                                  static_cast<float>(0xFFFF));

    if (m_sdl_gamepad != nullptr)
    {
#ifdef __EMSCRIPTEN__
        const auto success = SDL_GameControllerRumble(
            m_sdl_gamepad,
            normalized_left_motor_intensity,
            normalized_right_motor_intensity,
            static_cast<Uint32>(
                std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()));
#else
        const auto success = SDL_RumbleGamepad(
            m_sdl_gamepad,
            normalized_left_motor_intensity,
            normalized_right_motor_intensity,
            static_cast<Uint32>(
                std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()));
#endif

        return success == 0; // NOLINT
    }

    return false;
}

bool GamepadImpl::has_sensor(GamepadSensorType sensor) const
{
#ifdef __EMSCRIPTEN__
    return SDL_GameControllerHasSensor(m_sdl_gamepad, to_sdl_gamepad_sensor_type(sensor)) ==
           SDL_TRUE;
#else
    return SDL_GamepadHasSensor(m_sdl_gamepad, to_sdl_gamepad_sensor_type(sensor));
#endif
}

bool GamepadImpl::is_sensor_enabled(GamepadSensorType sensor) const
{
#ifdef __EMSCRIPTEN__
    return SDL_GameControllerIsSensorEnabled(m_sdl_gamepad, to_sdl_gamepad_sensor_type(sensor)) ==
           SDL_TRUE;
#else
    return SDL_GamepadSensorEnabled(m_sdl_gamepad, to_sdl_gamepad_sensor_type(sensor));
#endif
}

void GamepadImpl::set_sensor_enabled(GamepadSensorType sensor, bool enabled)
{
#ifdef __EMSCRIPTEN__
    SDL_GameControllerSetSensorEnabled(m_sdl_gamepad,
                                       to_sdl_gamepad_sensor_type(sensor),
                                       enabled ? SDL_TRUE : SDL_FALSE);
#else
    SDL_SetGamepadSensorEnabled(m_sdl_gamepad, to_sdl_gamepad_sensor_type(sensor), enabled);
#endif
}
} // namespace cer::details
