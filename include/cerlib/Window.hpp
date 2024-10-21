// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <cerlib/Color.hpp>
#include <cerlib/details/ObjectMacros.hpp>
#include <functional>
#include <optional>
#include <string_view>

namespace cer
{
namespace details
{
class WindowImpl;
}

/**
 * Defines the type of a message box.
 * A message box is created by using methods defined in the App class.
 *
 * @ingroup Game
 */
enum class MessageBoxType
{
    Information = 1,
    Warning     = 2,
    Error       = 3,
};

/**
 * Represents a single window.
 *
 * @ingroup Game
 */
class Window final
{
    CERLIB_DECLARE_OBJECT(Window);

  public:
    /**
     * Represents a function that is called by the Window whenever
     * it is resized.
     */
    using ResizeCallback =
        std::function<void(uint32_t width, uint32_t height, uint32_t width_px, uint32_t height_px)>;

    /**
     * Creates a simple window.
     *
     * @param title The title / caption of the window.
     * @param id An optional user-defined ID for the window. Has no meaning within cerlib
     * and is mutable after construction. See Window::id() and Window::set_id().
     * @param position_x The X-position of the window, in logical units. If empty, the
     * position is centered within the display.
     * @param position_y The Y-position of the window, in logical units. If empty, the
     * position is centered within the display.
     * @param width The width of the window, in logical units.
     * @param height The height of the window, in logical units.
     * buffer alongside its view.
     * @param allow_high_dpi If true, high-density displays are considered and the window
     * is able to report a pixel ratio.
     */
    explicit Window(std::string_view        title,
                    uint32_t                id             = 0,
                    std::optional<int32_t>  position_x     = std::nullopt,
                    std::optional<int32_t>  position_y     = std::nullopt,
                    std::optional<uint32_t> width          = std::nullopt,
                    std::optional<uint32_t> height         = std::nullopt,
                    bool                    allow_high_dpi = true);

    /**
     * Gets the optional user-defined ID of the window.
     * The default value is 0.
     */
    auto id() const -> uint32_t;

    /**
     * Sets the optional user-defined ID of the window.
     * The ID has no meaning within cerlib, it exists solely to allow the user
     * to identify multiple windows by their IDs.
     *
     * @param value The ID to set.
     */
    void set_id(uint32_t value);

    /**
     * Gets the width of the window, in logical units.
     * To obtain the window's client width in pixels, use width_px().
     */
    auto width() const -> float;

    /**
     * Gets the height of the window, in logical units.
     * To obtain the window's client height in pixels, use height_px().
     */
    auto height() const -> float;

    /**
     * Gets the size of the window, in logical units.
     */
    auto size() const -> Vector2;

    /**
     * Gets the width of the window's client area, in pixels.
     */
    auto width_px() const -> float;

    /**
     * Gets the height of the window's client area, in pixels.
     */
    auto height_px() const -> float;

    /**
     * Gets the size of the window's client area, in pixels.
     */
    auto size_px() const -> Vector2;

    /**
     * Gets the ratio between logical units and pixels.
     */
    auto pixel_ratio() const -> float;

    /**
     * Gets the current title / caption of the window.
     */
    auto title() const -> std::string_view;

    /**
     * Sets the current title / caption of the window.
     *
     * @param value The title to set.
     */
    void set_title(std::string_view value);

    /**
     * Sets a value indicating whether the window is visible or not.
     *
     * @param value If true, shows the window; otherwise hides it.
     */
    void set_visible(bool value);

    /**
     * Sets a value indicating whether the window is always on top of other
     * windows.
     *
     * @param value If true, the window will stay on top of other windows.
     */
    void set_always_on_top(bool value);

    /**
     * Sets a value indicating whether the window has a visible border.
     *
     * @param value If true, the window gets a border.
     */
    void set_bordered(bool value);

    /**
     * Sets a value indicating whether the window is in a full-screen state.
     * This is not an exclusive full-screen mode, but a borderless window that has the
     * same size as the display it is in.
     *
     * @param value If true, the window switches to a fullscreen mode.
     */
    void set_full_screen(bool value);

    /**
     * Sets a value indicating whether the window is resizable by the user.
     *
     * @param value If true, the window becomes user-resizable.
     */
    void set_resizable(bool value);

    /**
     * Minimizes the window.
     */
    void minimize();

    /**
     * Maximizes the window.
     */
    void maximize();

    /**
     * Makes the window visible to the user.
     */
    void show();

    /**
     * Hides the window from the user.
     */
    void hide();

    /**
     * Sets the minimum allowed size of the window, in logical units.
     */
    void set_minimum_size(uint32_t width, uint32_t height);

    /**
     * Sets the maximum allowed size of the window, in logical units.
     */
    void set_maximum_size(uint32_t width, uint32_t height);

    /**
     * Sets a value indicating whether the window grabs the mouse's focus
     * indefinitely.
     */
    void set_mouse_grab(bool value);

    /**
     * Sets the position of the window, in logical units.
     */
    void set_position(int32_t x, int32_t y);

    /**
     * Sets the size of the window, in logical units.
     */
    void set_size(uint32_t width, uint32_t height);

    /**
     * Sets the function that should be called by the window whenever the window is
     * resized.
     */
    void set_resize_callback(const ResizeCallback& value);

    /**
     * Gets the index of the display the window is currently in.
     * This index can be used in methods defined by the App class to obtain
     * further information about the display.
     */
    auto display_index() const -> uint32_t;

    auto sync_interval() const -> uint32_t;

    void set_sync_interval(uint32_t value);

    void set_clear_color(std::optional<Color> value);

    auto clear_color() const -> std::optional<Color>;

    /**
     * Shows a native message box.
     *
     * @param type The type of message box to show.
     * @param title The title of the message box.
     * @param message The contents of the message box.
     * @param parent_window The parent window of the message box. May be empty.
     */
    static void show_message_box(MessageBoxType   type,
                                 std::string_view title,
                                 std::string_view message,
                                 const Window&    parent_window = Window{});

    /**
     * Activates the on-screen keyboard of the current system.
     * This only has an effect on platforms that support on-screen keyboards.
     */
    void activate_onscreen_keyboard();

    /**
     * Deactivates the on-screen keyboard of the current system.
     * This only has an effect on platforms that support on-screen keyboards.
     */
    void deactivate_onscreen_keyboard();
};
} // namespace cer