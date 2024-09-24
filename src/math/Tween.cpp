// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "cerlib/Tween.hpp"
#include "cerlib/Math.hpp"

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wsequence-point"
#endif

namespace cer
{
Tweener::Tweener()
    : m_duration(1)
{
}

Tweener::Tweener(float from, float to, float duration, TweeningFunction* tweening_function)
    : m_position(from)
    , m_from(from)
    , m_change(to - from)
    , m_duration(duration)
    , m_to(to)
    , m_func(tweening_function)
{
}

void Tweener::start()
{
    m_is_running = true;
}

void Tweener::update(double elapsed_time)
{
    if (!m_is_running || m_elapsed == m_duration)
    {
        return;
    }

    m_elapsed += elapsed_time;

    if (m_elapsed >= m_duration)
    {
        m_elapsed  = m_duration;
        m_position = m_from + m_change;

        switch (m_loop_mode)
        {
            case TweenLoopMode::None: break;
            case TweenLoopMode::FrontToBack: reset(); break;
            case TweenLoopMode::BackAndForth: reverse(); break;
        }
    }
    else
    {
        m_position = m_func(static_cast<float>(m_elapsed), m_from, m_change, m_duration);
    }
}

void Tweener::stop()
{
    m_is_running = false;
}

void Tweener::reset()
{
    m_elapsed  = 0;
    m_position = m_from;
}

void Tweener::restart()
{
    reset();
    start();
}

void Tweener::reverse()
{
    m_elapsed = 0;
    m_change  = m_from - m_position;
    m_to      = m_from;
    m_from    = m_position;
}

float Tweener::percentage() const
{
    return m_to == 0.0f ? 0.0f : (static_cast<float>(m_position) / m_to);
}

bool Tweener::is_running() const
{
    return m_is_running;
}

bool Tweener::has_ended() const
{
    return m_elapsed == m_duration;
}

// NOLINTBEGIN

#ifdef __clang__
#pragma clang diagnostic ignored "-Wunsequenced"
#endif

float Tweener::back_ease_in(float t, float b, float c, float d)
{
    return c * (t /= d) * t * ((1.70158f + 1) * t - 1.70158f) + b;
}

float Tweener::back_ease_out(float t, float b, float c, float d)
{
    return c * ((t = t / d - 1) * t * ((1.70158f + 1) * t + 1.70158f) + 1) + b;
}

float Tweener::back_ease_in_out(float t, float b, float c, float d)
{
    float s = 1.70158f;

    if ((t /= d / 2) < 1)
    {
        return c / 2 * (t * t * (((s *= (1.525f)) + 1) * t - s)) + b;
    }

    return c / 2 * ((t -= 2) * t * (((s *= (1.525f)) + 1) * t + s) + 2) + b;
}

float Tweener::bounce_ease_out(float t, float b, float c, float d)
{
    if ((t /= d) < 1 / 2.75)
    {
        return c * (7.5625f * t * t) + b;
    }

    if (t < 2 / 2.75)
    {
        return c * (7.5625f * (t -= (1.5f / 2.75f)) * t + .75f) + b;
    }

    if (t < 2.5 / 2.75)
    {
        return c * (7.5625f * (t -= 2.25f / 2.75f) * t + .9375f) + b;
    }

    return c * (7.5625f * (t -= (2.625f / 2.75f)) * t + .984375f) + b;
}

float Tweener::bounce_ease_in(float t, float b, float c, float d)
{
    return c - bounce_ease_out(d - t, 0, c, d) + b;
}

float Tweener::bounce_ease_in_out(float t, float b, float c, float d)
{
    if (t < d / 2)
    {
        return bounce_ease_in(t * 2, 0, c, d) * 0.5f + b;
    }

    return bounce_ease_out(t * 2 - d, 0, c, d) * .5f + c * 0.5f + b;
}

float Tweener::circular_ease_in(float t, float b, float c, float d)
{
    return -c * (std::sqrt(1 - (t /= d) * t) - 1) + b;
}

float Tweener::circular_ease_out(float t, float b, float c, float d)
{
    return c * std::sqrt(1 - (t = t / d - 1) * t) + b;
}

float Tweener::circular_ease_in_out(float t, float b, float c, float d)
{
    if ((t /= d / 2) < 1)
    {
        return -c / 2 * (std::sqrt(1 - t * t) - 1) + b;
    }

    return c / 2 * (std::sqrt(1 - (t -= 2) * t) + 1) + b;
}

float Tweener::cubic_ease_in(float t, float b, float c, float d)
{
    return c * (t /= d) * t * t + b;
}

float Tweener::cubic_ease_out(float t, float b, float c, float d)
{
    return c * ((t = t / d - 1) * t * t + 1) + b;
}

float Tweener::cubic_ease_in_out(float t, float b, float c, float d)
{
    if ((t /= d / 2) < 1)
    {
        return c / 2 * t * t * t + b;
    }

    return c / 2 * ((t -= 2) * t * t + 2) + b;
}

float Tweener::elastic_ease_in(float t, float b, float c, float d)
{
    if (t == 0)
    {
        return b;
    }

    if ((t /= d) == 1)
    {
        return b + c;
    }

    float p = d * 0.3f;
    float s = p / 4;
    return -(c * std::pow(2.0f, 10.0f * (t -= 1)) * std::sin((t * d - s) * (2 * pi) / p)) + b;
}

float Tweener::elastic_ease_out(float t, float b, float c, float d)
{
    if (t == 0)
    {
        return b;
    }

    if ((t /= d) == 1)
    {
        return b + c;
    }

    const auto p = d * .3f;
    const auto s = p / 4;

    return c * std::pow(2.0f, -10.0f * t) * std::sin((t * d - s) * (2 * pi) / p) + c + b;
}

float Tweener::elastic_ease_in_out(float t, float b, float c, float d)
{
    if (t == 0)
    {
        return b;
    }

    if ((t /= d / 2) == 2)
    {
        return b + c;
    }

    const auto p = d * (.3f * 1.5f);
    const auto a = c;
    const auto s = p / 4;

    if (t < 1)
    {
        return -0.5f *
                   (a * std::pow(2.0f, 10.0f * (t -= 1)) * std::sin((t * d - s) * (2 * pi) / p)) +
               b;
    }

    return a * std::pow(2.0f, -10.0f * (t -= 1)) * std::sin((t * d - s) * (2 * pi) / p) * 0.5f + c +
           b;
}

float Tweener::exponential_ease_in(float t, float b, float c, float d)
{
    return t == 0 ? b : c * std::pow(2.0f, 10.0f * (t / d - 1)) + b;
}

float Tweener::exponential_ease_out(float t, float b, float c, float d)
{
    return t == d ? b + c : c * (-std::pow(2.0f, -10.0f * t / d) + 1) + b;
}

float Tweener::exponential_ease_in_out(float t, float b, float c, float d)
{
    if (t == 0)
    {
        return b;
    }

    if (t == d)
    {
        return b + c;
    }

    if ((t /= d / 2) < 1)
    {
        return c / 2 * std::pow(2.0f, 10.0f * (t - 1)) + b;
    }

    return c / 2 * (-std::pow(2.0f, -10.0f * --t) + 2) + b;
}

float Tweener::linear(float t, float b, float c, float d)
{
    return c * t / d + b;
}

float Tweener::quadratic_ease_in(float t, float b, float c, float d)
{
    return c * (t /= d) * t + b;
}

float Tweener::quadratic_ease_out(float t, float b, float c, float d)
{
    return -c * (t /= d) * (t - 2) + b;
}

float Tweener::quadratic_ease_in_out(float t, float b, float c, float d)
{
    if ((t /= d / 2) < 1)
    {
        return c / 2 * t * t + b;
    }

    return -c / 2 * ((--t) * (t - 2) - 1) + b;
}

float Tweener::quartic_ease_in(float t, float b, float c, float d)
{
    return c * (t /= d) * t * t * t + b;
}

float Tweener::quartic_ease_out(float t, float b, float c, float d)
{
    return -c * ((t = t / d - 1) * t * t * t - 1) + b;
}

float Tweener::quartic_ease_in_out(float t, float b, float c, float d)
{
    if ((t /= d / 2) < 1)
    {
        return c / 2 * t * t * t * t + b;
    }

    return -c / 2 * ((t -= 2) * t * t * t - 2) + b;
}

float Tweener::quintic_ease_in(float t, float b, float c, float d)
{
    return c * (t /= d) * t * t * t * t + b;
}

float Tweener::quintic_ease_out(float t, float b, float c, float d)
{
    return c * ((t = t / d - 1) * t * t * t * t + 1) + b;
}

float Tweener::quintic_ease_in_out(float t, float b, float c, float d)
{
    if ((t /= d / 2) < 1)
    {
        return c / 2 * t * t * t * t * t + b;
    }

    return c / 2 * ((t -= 2) * t * t * t * t + 2) + b;
}

float Tweener::sinusoidal_ease_in(float t, float b, float c, float d)
{
    return -c * std::cos(t / d * (pi / 2)) + c + b;
}

float Tweener::sinusoidal_ease_out(float t, float b, float c, float d)
{
    return c * std::sin(t / d * (pi / 2)) + b;
}

float Tweener::sinusoidal_ease_in_out(float t, float b, float c, float d)
{
    return -c / 2 * (std::cos(pi * t / d) - 1) + b;
}

// NOLINTEND
} // namespace cer
