// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <cerlib/Export.hpp>
#include <cerlib/SoundTypes.hpp>

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
    uint32_t id() const;

    /**
     * Gets a value indicating whether the channel is paused.
     */
    bool is_paused() const;

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
    float relative_playback_speed() const;

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
    float volume() const;

    /**
     * Sets the channel's current volume.
     */
    void set_volume(float value);

    /**
     * Gets the channel's current pan.
     */
    float pan() const;

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
    bool is_looping() const;

    /**
     * Sets whether the channel should loop.
     */
    void set_looping(bool value);

    /**
     * Gets the time point at which the channel is looping.
     */
    SoundTime loop_point() const;

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
    SoundTime stream_position() const;
};
} // namespace cer