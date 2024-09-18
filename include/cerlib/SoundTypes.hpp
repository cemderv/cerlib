// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <chrono>

namespace cer
{
/**
 * Defines audio time, in fractional seconds.
 * This is a type alias for `std::chrono::duration` and therefore supports chrono
 * literals.
 *
 * Example:
 *
 * @code{.cpp}
 * using namespace std::chrono_literals;
 * // ...
 * cer::play_sound(..., delay: 1.5s);
 * cer::play_sound(..., delay: 2300ms);
 * @endcode
 *
 * @ingroup Audio
 */
using SoundTime = std::chrono::duration<double>;

/**
 * Defines the behavior of a sound's playback when it is inaudible.
 *
 * @ingroup Audio
 */
enum class SoundInaudibleBehavior
{
    /**
     * If the sound is inaudible, its playback is paused.
     */
    PauseIfInaudible = 1,

    /**
     * If the sound is inaudible, it is killed.
     */
    KillIfInaudible = 2,

    /**
     * If the sound is inaudible, its playback continues.
     */
    KeepTickingIfInaudible = 3,
};
} // namespace cer
