// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <cerlib/details/ObjectMacros.hpp>

namespace cer
{
enum class TweenLoopMode
{
    None         = 0,
    FrontToBack  = 1,
    BackAndForth = 2,
};

/**
 * Represents a tween object.
 * Tweening is used to animate arbitrary values using specific functions.
 * A tweener is typically created as part of an object, such as a Player class.
 * It is then updated manually, every frame, together with the player.
 * The player is then able to obtain the updated (animated) property value.
 *
 * Example:
 * @code{.cpp}
 * // Create a tweener that linearly goes from 0 to 100 during 2 seconds.
 * Tweener tweener = {0.0f, 100.0f, 2.0f, cer::tween::linear);
 *
 * const float value_before = tweener.value();
 *
 * tweener.update(dt);
 *
 * const float value_after = tweener.value();
 * @endcode
 *
 * [More information](https://en.wikipedia.org/wiki/Inbetweening)
 *
 * @ingroup Math
 */
class Tweener
{
  public:
    /**
     * Represents a function used to animate properties in a tweener.
     * Built-in functions are part of the Tween class, such as `cer::Tween::linear`.
     */
    using TweeningFunction = float(float elapsed, float from, float change, float duration);

    Tweener();

    Tweener(float from, float to, float duration, TweeningFunction* tweening_function);

    void start();

    void update(double elapsed_time);

    void stop();

    void reset();

    void restart();

    void reverse();

    /** Gets the current value of the property. */
    auto value() const -> float;

    /**
     * Gets a percentage value indicating how far the tweener has progressed, in
     * the range `[0.0 .. 1.0]`.
     */
    auto percentage() const -> float;

    /**
     * Gets a value indicating whether the tweener is running.
     *
     * @remark This does not mean that the tweener is updating itself automatically.
     * It only means that when the tweener is not running (i.e. no `start()` was called),
     * that calls to `update()` will be ignored. It is still the user's responsibility to
     * update the tweener manually using `update()`.
     */
    auto is_running() const -> bool;

    /** Gets a value indicating whether the tweener has reached its end. */
    auto has_ended() const -> bool;

    /** Built-in tweening function */
    static auto back_ease_in(float t, float b, float c, float d) -> float;

    /** Built-in tweening function */
    static auto back_ease_out(float t, float b, float c, float d) -> float;

    /** Built-in tweening function */
    static auto back_ease_in_out(float t, float b, float c, float d) -> float;

    /** Built-in tweening function */
    static auto bounce_ease_out(float t, float b, float c, float d) -> float;

    /** Built-in tweening function */
    static auto bounce_ease_in(float t, float b, float c, float d) -> float;

    /** Built-in tweening function */
    static auto bounce_ease_in_out(float t, float b, float c, float d) -> float;

    /** Built-in tweening function */
    static auto circular_ease_in(float t, float b, float c, float d) -> float;

    /** Built-in tweening function */
    static auto circular_ease_out(float t, float b, float c, float d) -> float;

    /** Built-in tweening function */
    static auto circular_ease_in_out(float t, float b, float c, float d) -> float;

    /** Built-in tweening function */
    static auto cubic_ease_in(float t, float b, float c, float d) -> float;

    /** Built-in tweening function */
    static auto cubic_ease_out(float t, float b, float c, float d) -> float;

    /** Built-in tweening function */
    static auto cubic_ease_in_out(float t, float b, float c, float d) -> float;

    /** Built-in tweening function */
    static auto elastic_ease_in(float t, float b, float c, float d) -> float;

    /** Built-in tweening function */
    static auto elastic_ease_out(float t, float b, float c, float d) -> float;

    /** Built-in tweening function */
    static auto elastic_ease_in_out(float t, float b, float c, float d) -> float;

    /** Built-in tweening function */
    static auto exponential_ease_in(float t, float b, float c, float d) -> float;

    /** Built-in tweening function */
    static auto exponential_ease_out(float t, float b, float c, float d) -> float;

    /** Built-in tweening function */
    static auto exponential_ease_in_out(float t, float b, float c, float d) -> float;

    /** Built-in tweening function */
    static auto linear(float t, float b, float c, float d) -> float;

    /** Built-in tweening function */
    static auto quadratic_ease_in(float t, float b, float c, float d) -> float;

    /** Built-in tweening function */
    static auto quadratic_ease_out(float t, float b, float c, float d) -> float;

    /** Built-in tweening function */
    static auto quadratic_ease_in_out(float t, float b, float c, float d) -> float;

    /** Built-in tweening function */
    static auto quartic_ease_in(float t, float b, float c, float d) -> float;

    /** Built-in tweening function */
    static auto quartic_ease_out(float t, float b, float c, float d) -> float;

    /** Built-in tweening function */
    static auto quartic_ease_in_out(float t, float b, float c, float d) -> float;

    /** Built-in tweening function */
    static auto quintic_ease_in(float t, float b, float c, float d) -> float;

    /** Built-in tweening function */
    static auto quintic_ease_out(float t, float b, float c, float d) -> float;

    /** Built-in tweening function */
    static auto quintic_ease_in_out(float t, float b, float c, float d) -> float;

    /** Built-in tweening function */
    static auto sinusoidal_ease_in(float t, float b, float c, float d) -> float;

    /** Built-in tweening function */
    static auto sinusoidal_ease_out(float t, float b, float c, float d) -> float;

    /** Built-in tweening function */
    static auto sinusoidal_ease_in_out(float t, float b, float c, float d) -> float;

  private:
    float             m_position{};
    float             m_from{};
    float             m_change{};
    float             m_duration{};
    TweenLoopMode     m_loop_mode{TweenLoopMode::None};
    double            m_elapsed{};
    bool              m_is_running{};
    float             m_to{};
    TweeningFunction* m_func{};
};
} // namespace cer
