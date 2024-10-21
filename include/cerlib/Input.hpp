// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <cerlib/Vector2.hpp>
#include <cerlib/details/ObjectMacros.hpp>

namespace cer
{
struct Vector2;
enum class Key;
enum class MouseButton;

/**
 * Gets a value indicating whether a specific key is currently pressed.
 *
 * @param key The key to query.
 *
 * @ingroup Input
 */
auto is_key_down(Key key) -> bool;

/**
 * Gets a value indicating whether a specific key is currently released.
 *
 * @param key The key to query.
 *
 * @ingroup Input
 */
auto is_key_up(Key key) -> bool;

/**
 * Gets a value indicating whether a specific key was released during the current
 * game tick and pressed during the previous game tick.
 *
 * @param key button The key to query.
 *
 * @ingroup Input
 */
auto was_key_just_pressed(Key key) -> bool;

/**
 * Gets a value indicating whether a specific key was pressed during the current
 * game tick and released during the previous game tick.
 *
 * @param key button The key to query.
 *
 * @ingroup Input
 */
auto was_key_just_released(Key key) -> bool;

/**
 * Gets a value indicating whether a specific mouse button is currently pressed.
 *
 * @param button The mouse button to query.
 *
 * @ingroup Input
 */
auto is_mouse_button_down(MouseButton button) -> bool;

/**
 * Gets a value indicating whether a specific mouse button is currently released.
 *
 * @param button The mouse button to query.
 *
 * @ingroup Input
 */
auto is_mouse_button_up(MouseButton button) -> bool;

/**
 * Gets the current mouse position within the currently focused window.
 * The coordinates are relative to the window's top-left corner.
 *
 * @ingroup Input
 */
auto current_mouse_position() -> Vector2;

/**
 * Gets the amount of mouse movement within the currently focused window since
 * the last game tick, in pixels.
 *
 * @ingroup Input
 */
auto current_mouse_position_delta() -> Vector2;

/**
 * Gets the amount of mouse wheel movement since the last game tick.
 *
 * @ingroup Input
 */
auto current_mouse_wheel_delta() -> Vector2;
} // namespace cer
