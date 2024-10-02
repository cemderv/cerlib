// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <cerlib/Export.hpp>
#include <cerlib/Vector2.hpp>

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
bool is_key_down(Key key);

/**
 * Gets a value indicating whether a specific key is currently released.
 *
 * @param key The key to query.
 *
 * @ingroup Input
 */
bool is_key_up(Key key);

/**
 * Gets a value indicating whether a specific key was released during the current
 * game tick and pressed during the previous game tick.
 *
 * @param key button The key to query.
 *
 * @ingroup Input
 */
bool was_key_just_pressed(Key key);

/**
 * Gets a value indicating whether a specific key was pressed during the current
 * game tick and released during the previous game tick.
 *
 * @param key button The key to query.
 *
 * @ingroup Input
 */
bool was_key_just_released(Key key);

/**
 * Gets a value indicating whether a specific mouse button is currently pressed.
 *
 * @param button The mouse button to query.
 *
 * @ingroup Input
 */
bool is_mouse_button_down(MouseButton button);

/**
 * Gets a value indicating whether a specific mouse button is currently released.
 *
 * @param button The mouse button to query.
 *
 * @ingroup Input
 */
bool is_mouse_button_up(MouseButton button);

/**
 * Gets the current mouse position within the currently focused window.
 * The coordinates are relative to the window's top-left corner.
 *
 * @ingroup Input
 */
Vector2 current_mouse_position();

/**
 * Gets the amount of mouse movement within the currently focused window since
 * the last game tick, in pixels.
 *
 * @ingroup Input
 */
Vector2 current_mouse_position_delta();

/**
 * Gets the amount of mouse wheel movement since the last game tick.
 *
 * @ingroup Input
 */
Vector2 current_mouse_wheel_delta();
} // namespace cer
