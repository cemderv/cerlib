// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <cerlib/Gamepad.hpp>
#include <cerlib/Input.hpp>
#include <cerlib/Vector2.hpp>
#include <cerlib/Window.hpp>

namespace cer
{
enum class Key;
enum class KeyModifier;
enum class MouseButton;

/**
 * An event that is raised when a window's visibility changed from hidden to shown.
 *
 * @ingroup Events
 */
struct WindowShownEvent
{
    uint64_t timestamp{};
    Window   window{};
};

/**
 * An event that is raised when a window's visibility changed from shown to hidden.
 *
 * @ingroup Events
 */
struct WindowHiddenEvent
{
    uint64_t timestamp{};
    Window   window{};
};

/**
 * An event that is raised when a window has moved.
 *
 * @ingroup Events
 */
struct WindowMovedEvent
{
    uint64_t timestamp{};
    Window   window{};
};

/**
 * An event that is raised when a window was resized.
 *
 * @ingroup Events
 */
struct WindowResizedEvent
{
    uint64_t timestamp{};
    Window   window{};
    uint32_t new_width{};
    uint32_t new_height{};
};

/**
 * An event that is raised when a window changed to a minimized state.
 *
 * @ingroup Events
 */
struct WindowMinimizedEvent
{
    uint64_t timestamp{};
    Window   window{};
};

/**
 * An event that is raised when a window changed to a maximized state.
 *
 * @ingroup Events
 */
struct WindowMaximizedEvent
{
    uint64_t timestamp{};
    Window   window{};
};

/**
 * An event that is raised when a window received focus via mouse interaction.
 *
 * @ingroup Events
 */
struct WindowGotMouseFocusEvent
{
    uint64_t timestamp{};
    Window   window{};
};

/**
 * An event that is raised when a window has lost its mouse focus.
 *
 * @ingroup Events
 */
struct WindowLostMouseFocusEvent
{
    uint64_t timestamp{};
    Window   window{};
};

/**
 * An event that is raised when a window received focus via keyboard interaction.
 *
 * @ingroup Events
 */
struct WindowGotKeyboardFocusEvent
{
    uint64_t timestamp{};
    Window   window{};
};

/**
 * An event that is raised when a window lost its keyboard focus.
 *
 * @ingroup Events
 */
struct WindowLostKeyboardFocusEvent
{
    uint64_t timestamp{};
    Window   window{};
};

/**
 * An event that is raised when a window was requested to be closed.
 *
 * @ingroup Events
 */
struct WindowCloseEvent
{
    uint64_t timestamp{};
    Window   window{};
};

/**
 * An event that is raised when a key on the keyboard was just pressed down.
 *
 * @ingroup Events
 */
struct KeyPressEvent
{
    uint64_t    timestamp{};
    Window      window{};
    Key         key{};
    KeyModifier modifiers{};
    bool        is_repeat{};
};

/**
 * An event that is raised when a key on the keyboard was just released.
 *
 * @ingroup Events
 */
struct KeyReleaseEvent
{
    uint64_t    timestamp{};
    Window      window{};
    Key         key{};
    KeyModifier modifiers{};
    bool        is_repeat{};
};

/**
 * An event that is raised when the mouse pointer has just moved.
 *
 * @ingroup Events
 */
struct MouseMoveEvent
{
    uint64_t timestamp{};
    Window   window{};
    uint32_t id{};
    Vector2  position{};
    Vector2  delta{};
};

/**
 * An event that is raised when a mouse button was just pressed down.
 *
 * @ingroup Events
 */
struct MouseButtonPressEvent
{
    uint64_t    timestamp{};
    Window      window{};
    uint32_t    id{};
    MouseButton button{};
    Vector2     position{};
};

/**
 * An event that is raised when a mouse button was just released.
 *
 * @ingroup Events
 */
struct MouseButtonReleaseEvent
{
    uint64_t    timestamp{};
    Window      window{};
    uint32_t    id{};
    MouseButton button{};
    Vector2     position{};
};

/**
 * An event that is raised when a double click was performed in a window.
 *
 * @ingroup Events
 */
struct MouseDoubleClickEvent
{
    uint64_t    timestamp{};
    Window      window{};
    uint32_t    id{};
    MouseButton button{};
    Vector2     position{};
};

/**
 * An event that is raised when the mouse wheel was scrolled in a window.
 *
 * @ingroup Events
 */
struct MouseWheelEvent
{
    uint64_t timestamp{};
    Window   window{};
    uint32_t id{};
    Vector2  position{};
    Vector2  delta{};
};

/**
 * Defines the type of a cer::TouchFingerEvent.
 *
 * @ingroup Events
 */
enum class TouchFingerEventType
{
    Motion  = 1,
    Press   = 2,
    Release = 3,
};

/**
 * An event that is raised when the screen was touched or an existing touch has
 * moved.
 *
 * @ingroup Events
 */
struct TouchFingerEvent
{
    TouchFingerEventType type{};
    uint64_t             timestamp{};
    Window               window{};
    uint64_t             touch_id{};
    uint64_t             finger_id{};
    Vector2              position{};
    Vector2              delta{};
    float                pressure{};
};

/**
 * An event that is raised when a gamepad was connected to the system.
 * This event is not raised when a gamepad was already connected before the game started
 * running. To query gamepads that were connected prior to the game's run, use the
 * Game::gamepads() method.
 *
 * @ingroup Events
 */
struct GamepadConnectedEvent
{
    Gamepad gamepad;
};

/**
 * An event that is raised when a gamepad was disconnected from the system.
 *
 * @ingroup Events
 */
struct GamepadDisconnectedEvent
{
    Gamepad gamepad;
};

/**
 * An event that is raised when a window received text input, for example from
 * a physical or on-screen keyboard.
 *
 * @ingroup Events
 */
struct TextInputEvent
{
    uint64_t    timestamp{};
    Window      window;
    std::string text;
};
} // namespace cer