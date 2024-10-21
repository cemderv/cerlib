// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <cerlib/SoundTypes.hpp>
#include <cerlib/details/ObjectMacros.hpp>

namespace cer
{
namespace details
{
class SoundChannelImpl;
}

/**
 * Represents a virtual channel of a played sound.
 * When playing a sound, a handle to its channel is returned from the playback function.
 * The channel can be used to further manage the playback, such as changing volume or
 * pausing the sound.
 *
 * @ingroup Audio
 */
class SoundChannel
{
    CERLIB_DECLARE_OBJECT(SoundChannel);

  public:
    /**
     * Gets the unique ID of the channel.
     */
    auto id() const -> uint32_t;

    /**
     * Gets a value indicating whether the channel is paused.
     */
    auto is_paused() const -> bool;

    /**
     * Sets a value indicating whether the channel is paused.
     *
     * @param value If true, the channel resumes playback. If false, the channel is
     * paused.
     */
    void set_paused(bool value);

    /**
     * Gets the playback speed of the channel.
     */
    auto relative_playback_speed() const -> float;

    /**
     * Sets the playback speed of the channel.
     *
     * @param value The new playback speed.
     */
    void set_relative_playback_speed(float value);

    /**
     * Changes the playback position of the channel.
     *
     * @param seconds The new position.
     */
    void seek(SoundTime seconds);

    /**
     * Stops playing the channel.
     */
    void stop();

    /**
     * Gets the channel's current volume.
     */
    auto volume() const -> float;

    /**
     * Sets the channel's current volume.
     */
    void set_volume(float value);

    /**
     * Gets the channel's current pan.
     */
    auto pan() const -> float;

    /**
     * Sets the channel's current pan.
     */
    void set_pan(float value);

    /**
     * Sets whether the channel should be protected.
     */
    void set_protected(bool value);

    /**
     * Gets a value indicating whether the channel is currently looping.
     */
    auto is_looping() const -> bool;

    /**
     * Sets whether the channel should loop.
     */
    void set_looping(bool value);

    /**
     * Gets the time point at which the channel is looping.
     */
    auto loop_point() const -> SoundTime;

    /**
     * Sets the time point at which the channel should loop.
     */
    void set_loop_point(SoundTime value);

    /**
     * Sets the behavior of the channel when it is inaudible.
     */
    void set_inaudible_behavior(SoundInaudibleBehavior value);

    /**
     * Starts fading the channel's volume.
     *
     * @param to_volume The target volume
     * @param fade_duration The fade duration
     */
    void fade_volume(float to_volume, SoundTime fade_duration);

    /**
     * Starts fading the channel's pan.
     *
     * @param to_pan The target pan
     * @param fade_duration The fade duration
     */
    void fade_pan(float to_pan, SoundTime fade_duration);

    /**
     * Starts fading the channel's playback speed.
     *
     * @param to_speed The target speed
     * @param fade_duration The fade duration
     */
    void fade_relative_playback_speed(float to_speed, SoundTime fade_duration);

    /**
     * Stops the channel's playback after a certain amount of time has passed.
     *
     * @param after The duration until the channel is stopped
     */
    void stop_after(SoundTime after);

    /**
     * Pauses the channel's playback after a certain amount of time has passed.
     *
     * @param after The duration until the channel is paused
     */
    void pause_after(SoundTime after);

    /**
     * Gets the channel's current playback position.
     */
    auto stream_position() const -> SoundTime;
};
} // namespace cer