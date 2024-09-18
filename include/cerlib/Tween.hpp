// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <cerlib/Export.hpp>

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
class CERLIB_API Tweener
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
    float value() const;

    /**
     * Gets a percentage value indicating how far the tweener has progressed, in
     * the range `[0.0 .. 1.0]`.
     */
    float percentage() const;

    /**
     * Gets a value indicating whether the tweener is running.
     *
     * @remark This does not mean that the tweener is updating itself automatically.
     * It only means that when the tweener is not running (i.e. no `start()` was called),
     * that calls to `update()` will be ignored. It is still the user's responsibility to
     * update the tweener manually using `update()`.
     */
    bool is_running() const;

    /** Gets a value indicating whether the tweener has reached its end. */
    bool has_ended() const;

    /** Built-in tweening function */
    static float back_ease_in(float t, float b, float c, float d);

    /** Built-in tweening function */
    static float back_ease_out(float t, float b, float c, float d);

    /** Built-in tweening function */
    static float back_ease_in_out(float t, float b, float c, float d);

    /** Built-in tweening function */
    static float bounce_ease_out(float t, float b, float c, float d);

    /** Built-in tweening function */
    static float bounce_ease_in(float t, float b, float c, float d);

    /** Built-in tweening function */
    static float bounce_ease_in_out(float t, float b, float c, float d);

    /** Built-in tweening function */
    static float circular_ease_in(float t, float b, float c, float d);

    /** Built-in tweening function */
    static float circular_ease_out(float t, float b, float c, float d);

    /** Built-in tweening function */
    static float circular_ease_in_out(float t, float b, float c, float d);

    /** Built-in tweening function */
    static float cubic_ease_in(float t, float b, float c, float d);

    /** Built-in tweening function */
    static float cubic_ease_out(float t, float b, float c, float d);

    /** Built-in tweening function */
    static float cubic_ease_in_out(float t, float b, float c, float d);

    /** Built-in tweening function */
    static float elastic_ease_in(float t, float b, float c, float d);

    /** Built-in tweening function */
    static float elastic_ease_out(float t, float b, float c, float d);

    /** Built-in tweening function */
    static float elastic_ease_in_out(float t, float b, float c, float d);

    /** Built-in tweening function */
    static float exponential_ease_in(float t, float b, float c, float d);

    /** Built-in tweening function */
    static float exponential_ease_out(float t, float b, float c, float d);

    /** Built-in tweening function */
    static float exponential_ease_in_out(float t, float b, float c, float d);

    /** Built-in tweening function */
    static float linear(float t, float b, float c, float d);

    /** Built-in tweening function */
    static float quadratic_ease_in(float t, float b, float c, float d);

    /** Built-in tweening function */
    static float quadratic_ease_out(float t, float b, float c, float d);

    /** Built-in tweening function */
    static float quadratic_ease_in_out(float t, float b, float c, float d);

    /** Built-in tweening function */
    static float quartic_ease_in(float t, float b, float c, float d);

    /** Built-in tweening function */
    static float quartic_ease_out(float t, float b, float c, float d);

    /** Built-in tweening function */
    static float quartic_ease_in_out(float t, float b, float c, float d);

    /** Built-in tweening function */
    static float quintic_ease_in(float t, float b, float c, float d);

    /** Built-in tweening function */
    static float quintic_ease_out(float t, float b, float c, float d);

    /** Built-in tweening function */
    static float quintic_ease_in_out(float t, float b, float c, float d);

    /** Built-in tweening function */
    static float sinusoidal_ease_in(float t, float b, float c, float d);

    /** Built-in tweening function */
    static float sinusoidal_ease_out(float t, float b, float c, float d);

    /** Built-in tweening function */
    static float sinusoidal_ease_in_out(float t, float b, float c, float d);

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
