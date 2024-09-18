// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <cerlib/Export.hpp>
#include <cerlib/SoundTypes.hpp>
#include <optional>

namespace cer
{
class Sound;
class SoundChannel;

/**
 * Gets a value indicating whether the audio device has been initialized.
 * This is the case if the game was initialized with audio enabled and a suitable audio
 * device was found on the system.
 *
 * If the audio device is not initialized, audio API calls will have no effect on audio
 * output.
 *
 * @ingroup Audio
 */
CERLIB_API bool is_audio_device_initialized();

/**
 * Plays a sound.
 *
 * @param sound The sound to play.
 * @param volume The initial volume of the sound.
 * @param pan The left/right panning of the sound. -1 is fully left, +1 is fully right.
 * @param start_paused If true, the sound will start in a paused state.
 * @param delay The delay after which to start playing the sound.
 * @return The sound's channel. Can be used to control further playback of the sound.
 *
 * @ingroup Audio
 */
[[nodiscard]] CERLIB_API SoundChannel play_sound(const Sound&             sound,
                                                 float                    volume       = 1.0f,
                                                 float                    pan          = 0.0f,
                                                 bool                     start_paused = false,
                                                 std::optional<SoundTime> delay = std::nullopt);

/**
 * Plays a sound without returning its channel.
 *
 * @param sound The sound to play.
 * @param volume The volume of the sound.
 * @param pan The left/right panning of the sound. -1 is fully left, +1 is fully right.
 * @param delay The delay after which to start playing the sound.
 *
 * @ingroup Audio
 */
CERLIB_API void play_sound_fire_and_forget(const Sound&             sound,
                                           float                    volume = 1.0f,
                                           float                    pan    = 0.0f,
                                           std::optional<SoundTime> delay  = std::nullopt);

/**
 * Plays a sound with its volume set equally to all channels, and without
 * panning.
 *
 * @ingroup Audio
 */
CERLIB_API SoundChannel play_sound_in_background(const Sound& sound,
                                                 float        volume       = -1.0f,
                                                 bool         start_paused = false);

/**
 * Stops the playback of all currently playing sounds.
 *
 * @ingroup Audio
 */
CERLIB_API void stop_all_sounds();

/**
 * Pauses the playback of all currently playing sounds.
 *
 * @ingroup Audio
 */
CERLIB_API void pause_all_sounds();

/**
 * Resumes the playback of all currently paused sounds.
 *
 * @ingroup Audio
 */
CERLIB_API void resume_all_sounds();

/**
 * Gets the global audio volume.
 *
 * @ingroup Audio
 */
CERLIB_API float global_volume();

/**
 * Sets the global audio volume.
 *
 * @ingroup Audio
 */
CERLIB_API void set_global_volume(float value);

/**
 * Changes the global audio volume over time.
 *
 * @param to_volume The target volume.
 * @param fade_duration The duration of the fade.
 *
 * @ingroup Audio
 */
CERLIB_API void fade_global_volume(float to_volume, SoundTime fade_duration);
} // namespace cer
