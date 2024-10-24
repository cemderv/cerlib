// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <cerlib/Event.hpp>
#include <cerlib/Image.hpp>
#include <cerlib/Logging.hpp>
#include <cerlib/details/ObjectMacros.hpp>

namespace cer
{
class Window;
enum class Key;
enum class KeyModifier;
enum class MouseButton;
enum class ImageFormat;

namespace details
{
class GameImpl;
class WindowImpl;

auto run_game_internal() -> void;
} // namespace details

/**
 * Represents a mode of a display ("which resolutions does the display
 * support, which formats, which refresh rates?").
 *
 * @ingroup Game
 */
struct DisplayMode
{
    /** Default comparison */
    auto operator==(const DisplayMode&) const -> bool = default;

    /** Default comparison */
    auto operator!=(const DisplayMode&) const -> bool = default;

    /** If known, the format of the mode */
    Option<ImageFormat> format;

    /** The width of the mode, in pixels */
    uint32_t width{};

    /** The height of the mode, in pixels */
    uint32_t height{};

    /** The refresh rate of the mode (Hz) */
    uint32_t refresh_rate{};

    /** The DPI scale factor of the mode */
    float content_scale{};
};

/**
 * Defines the fixed orientation of a display.
 *
 * @ingroup Game
 */
enum class DisplayOrientation
{
    Unknown          = 0, /** */
    Landscape        = 1, /** */
    LandscapeFlipped = 2, /** */
    Portrait         = 3, /** */
    PortraitFlipped  = 4, /** */
};

/**
 * Represents timing information about a running game.
 *
 * @ingroup Game
 */
struct GameTime
{
    /** Default comparison */
    auto operator==(const GameTime&) const -> bool = default;

    /** Default comparison */
    auto operator!=(const GameTime&) const -> bool = default;

    /** The time that has elapsed since the last frame, in fractional seconds */
    double elapsed_time{};

    /** The time that has elapsed since the game started running, in fractional seconds */
    double total_time{};
};

/**
 * Represents the central game class.
 *
 * It is responsible for initializing, running and deinitializing the game instance.
 * Only one game instance may be alive in a process at a time.
 *
 * The `Game` class cannot be used directly; it must be inherited from.
 * To run your custom Game-derived class, use the `cer::run_game()` function.
 *
 * @ingroup Game
 */
class Game
{
  protected:
    /** Default constructor, intended to be called by deriving classes */
    Game();

    /**
     * The game's constructor, intended to be called by deriving classes.
     *
     * @param enable_audio If true, enables the audio device. If the game does not need
     * any audio capabilities, false may be specified to avoid overhead.
     */
    explicit Game(bool enable_audio);

  public:
    Game(const Game&) = delete;

    void operator=(const Game&) = delete;

    Game(Game&&) noexcept = delete;

    void operator=(Game&&) noexcept = delete;

    virtual ~Game() noexcept;

    /**
     * Gets the number of displays connected to the game's system.
     */
    auto display_count() -> uint32_t;

    /**
     * Gets the display mode of a specific display.
     * If no display mode was determined, an empty value is returned.
     *
     * @param display_index The index of the display for which to obtain the current
     * display mode.
     */
    auto current_display_mode(uint32_t display_index) -> Option<DisplayMode>;

    /**
     * Gets a list of all supported display modes of a specific display.
     * If no display modes were determined, an empty list is returned.
     *
     * @param display_index The index of the display for which to obtain the display
     * modes.
     */
    auto display_modes(uint32_t display_index) -> List<DisplayMode>;

    auto display_content_scale(uint32_t display_index) -> float;

    auto display_orientation(uint32_t display_index) -> DisplayOrientation;

    auto gamepads() -> List<Gamepad>;

  protected:
    virtual void load_content();

    virtual auto update(const GameTime& time) -> bool;

    virtual void draw(const Window& window);

    virtual void draw_imgui(const Window& window);

    virtual void on_window_shown(const WindowShownEvent& event);

    virtual void on_window_hidden(const WindowHiddenEvent& event);

    virtual void on_window_moved(const WindowMovedEvent& event);

    virtual void on_window_resized(const WindowResizedEvent& event);

    virtual void on_window_minimized(const WindowMinimizedEvent& event);

    virtual void on_window_maximized(const WindowMaximizedEvent& event);

    virtual void on_window_got_mouse_focus(const WindowGotMouseFocusEvent& event);

    virtual void on_window_lost_mouse_focus(const WindowLostMouseFocusEvent& event);

    virtual void on_window_got_keyboard_focus(const WindowGotKeyboardFocusEvent& event);

    virtual void on_window_lost_keyboard_focus(const WindowLostKeyboardFocusEvent& event);

    virtual void on_window_close(const WindowCloseEvent& event);

    virtual void on_key_press(const KeyPressEvent& event);

    virtual void on_key_release(const KeyReleaseEvent& event);

    virtual void on_mouse_move(const MouseMoveEvent& event);

    virtual void on_mouse_button_press(const MouseButtonPressEvent& event);

    virtual void on_mouse_button_release(const MouseButtonReleaseEvent& event);

    virtual void on_mouse_double_click(const MouseDoubleClickEvent& event);

    virtual void on_mouse_wheel(const MouseWheelEvent& event);

    virtual void on_touch_finger(const TouchFingerEvent& event);

    virtual void on_gamepad_connected(const GamepadConnectedEvent& event);

    virtual void on_gamepad_disconnected(const GamepadDisconnectedEvent& event);

    virtual void on_text_input(const TextInputEvent& event);
};

/**
 * Creates, runs and shuts down a game.
 *
 * **This is the central function to call from the main function.**
 *
 * @tparam T The type of the game to run. Must be a type derived from the Game class.
 * @param args The arguments to pass to the game's constructor.
 *
 * Example:
 * @code{.cpp}
 * int main()
 * {
 *   return cer::run_game<MyGame>();
 *
 *   // Or if MyGame's constructor is e.g. MyGame(int, bool, string):
 *
 *   return cer::run_game<MyGame>(5, true, "Some String");
 * }
 * @endcode
 *
 * @attention This is a blocking call. It blocks until the game is done running.
 *
 * @ingroup Game
 */
template <typename T, typename... Args>
    requires(std::is_base_of_v<Game, T> && !std::is_same_v<T, Game>)
[[nodiscard]] auto run_game(Args&&... args) -> int
{
    try
    {
        const T game{std::forward<Args>(args)...};
        details::run_game_internal();
        return 0;
    }
    catch (const std::exception& e)
    {
        log_error("An unhandled error occurred: {}", e.what());
        return 1;
    }
}
} // namespace cer
