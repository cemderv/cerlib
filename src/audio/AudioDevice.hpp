// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "audio/AudioSource.hpp"
#include "audio/Common.hpp"
#include "audio/Misc.hpp"
#include "cerlib/Sound.hpp"
#include "cerlib/SoundTypes.hpp"
#include "util/NonCopyable.hpp"
#include <memory>
#include <optional>
#include <span>
#include <unordered_set>
#include <vector>

namespace cer
{
class Sound;
class SoundChannel;

auto to_soloud_time(const SoundTime& seconds) -> cer::time_t;

class AudioDevice
{
  public:
    explicit AudioDevice(EngineFlags           flags       = {},
                         std::optional<size_t> sample_rate = std::nullopt,
                         std::optional<size_t> buffer_size = std::nullopt,
                         size_t                channels    = 2);

    NON_COPYABLE_NON_MOVABLE(AudioDevice);

    ~AudioDevice() noexcept;

    auto play_sound(const Sound&             sound,
                    float                    volume,
                    float                    pan,
                    bool                     start_paused,
                    std::optional<SoundTime> delay) -> SoundChannel;

    void play_sound_fire_and_forget(const Sound&             sound,
                                    float                    volume,
                                    float                    pan,
                                    std::optional<SoundTime> delay);

    auto play_sound_in_background(const Sound& sound, float volume, bool start_paused)
        -> SoundChannel;

    void stop_all_sounds();

    void pause_all_sounds();

    void resume_all_sounds();

    auto global_volume() const -> float;

    void set_global_volume(float value);

    void fade_global_volume(float to_volume, SoundTime fade_duration);

    void purge_sounds();

    // From SoLoud::Soloud:
  public:
    void pause();

    void resume();

    // Returns current backend channel count (1 mono, 2 stereo, etc)
    auto backend_channels() const -> size_t;

    // Returns current backend sample rate
    auto backend_sample_rate() const -> size_t;

    // Returns current backend buffer size
    auto backend_buffer_size() const -> size_t;

    // Set speaker position in 3d space
    void speaker_position(size_t channel, Vector3 value);

    // Get speaker position in 3d space
    auto speaker_position(size_t channel) const -> Vector3;

    // Start playing a sound. Returns voice handle, which can be ignored or used to alter the
    // playing sound's parameters. Negative volume means to use default.
    auto play(AudioSource& sound,
              float        volume = -1.0f,
              float        pan    = 0.0f,
              bool         paused = false,
              size_t       bus    = 0) -> handle;

    // Start playing a sound delayed in relation to other sounds called via this function. Negative
    // volume means to use default.
    auto play_clocked(time_t       sound_time,
                      AudioSource& sound,
                      float        volume = -1.0f,
                      float        pan    = 0.0f,
                      size_t       bus    = 0) -> handle;

    // Start playing a 3d audio source
    auto play3d(AudioSource& sound,
                Vector3      pos,
                Vector3      vel    = {},
                float        volume = 1.0f,
                bool         paused = false,
                size_t       bus    = 0) -> handle;

    // Start playing a 3d audio source, delayed in relation to other sounds called via this
    // function.
    auto play3d_clocked(time_t       sound_time,
                        AudioSource& sound,
                        Vector3      pos,
                        Vector3      vel    = {},
                        float        volume = 1.0f,
                        size_t       bus    = 0) -> handle;

    // Start playing a sound without any panning. It will be played at full volume.
    auto play3d_background(AudioSource& sound,
                           float        volume = -1.0f,
                           bool         paused = false,
                           size_t       bus    = 0) -> handle;

    // Seek the audio stream to certain point in time. Some streams can't seek backwards. Relative
    // play speed affects time.
    auto seek(handle voice_handle, time_t seconds) -> bool;

    // Stop the sound.
    void stop(handle voice_handle);

    // Stop all voices.
    void stop_all();

    // Stop all voices that play this sound source
    void stop_audio_source(AudioSource& sound);

    // Count voices that play this audio source
    auto count_audio_source(AudioSource& sound) -> int;

    // Set a live filter parameter. Use 0 for the global filters.
    void set_filter_parameter(handle voice_handle,
                              size_t filter_id,
                              size_t attribute_id,
                              float  value);

    // Get a live filter parameter. Use 0 for the global filters.
    auto filter_parameter(handle voice_handle, size_t filter_id, size_t attribute_id)
        -> std::optional<float>;
    // Fade a live filter parameter. Use 0 for the global filters.
    void fade_filter_parameter(
        handle voice_handle, size_t filter_id, size_t attribute_id, float to, time_t time);

    // Oscillate a live filter parameter. Use 0 for the global filters.
    void oscillate_filter_parameter(handle voice_handle,
                                    size_t filter_id,
                                    size_t attribute_id,
                                    float  from,
                                    float  to,
                                    time_t time);

    // Get current play time, in seconds.
    auto stream_time(handle voice_handle) -> time_t;
    // Get current sample position, in seconds.
    auto stream_position(handle voice_handle) -> time_t;
    // Get current pause state.
    auto pause(handle voice_handle) -> bool;
    // Get current volume.
    auto volume(handle voice_handle) -> float;
    // Get current overall volume (set volume * 3d volume)
    auto overall_volume(handle voice_handle) -> float;
    // Get current pan.
    auto pan(handle voice_handle) -> float;
    // Get current sample rate.
    auto sample_rate(handle voice_handle) -> float;
    // Get current voice protection state.
    auto is_voice_protected(handle voice_handle) -> bool;
    // Get the current number of busy voices.
    auto active_voice_count() -> size_t;
    // Get the current number of voices in SoLoud
    auto voice_count() -> size_t;
    // Check if the handle is still valid, or if the sound has stopped.
    auto is_valid_voice_handle(handle voice_handle) -> bool;
    // Get current relative play speed.
    auto relative_play_speed(handle voice_handle) -> float;
    // Get current post-clip scaler value.
    auto post_clip_scaler() const -> float;
    // Get the current main resampler
    auto main_resampler() const -> Resampler;
    // Get current maximum active voice setting
    auto max_active_voice_count() const -> size_t;
    // Query whether a voice is set to loop.
    auto is_voice_looping(handle voice_handle) -> bool;
    // Query whether a voice is set to auto-stop when it ends.
    auto getAutoStop(handle voice_handle) -> bool;
    // Get voice loop point value
    auto getLoopPoint(handle voice_handle) -> time_t;

    // Set voice loop point value
    void setLoopPoint(handle voice_handle, time_t aLoopPoint);
    // Set voice's loop state
    void setLooping(handle voice_handle, bool aLooping);
    // Set whether sound should auto-stop when it ends
    void setAutoStop(handle voice_handle, bool aAutoStop);
    // Set current maximum active voice setting
    void setMaxActiveVoiceCount(size_t aVoiceCount);
    // Set behavior for inaudible sounds
    void setInaudibleBehavior(handle voice_handle, bool aMustTick, bool aKill);
    // Set the global volume
    void setGlobalVolume(float aVolume);
    // Set the post clip scaler value
    void setPostClipScaler(float aScaler);
    // Set the main resampler
    void setMainResampler(Resampler aResampler);
    // Set the pause state
    void setPause(handle voice_handle, bool aPause);
    // Pause all voices
    void setPauseAll(bool aPause);
    // Set the relative play speed
    void setRelativePlaySpeed(handle voice_handle, float aSpeed);
    // Set the voice protection state
    void setProtectVoice(handle voice_handle, bool aProtect);
    // Set the sample rate
    void setSamplerate(handle voice_handle, float aSamplerate);
    // Set panning value; -1 is left, 0 is center, 1 is right
    void setPan(handle voice_handle, float aPan);
    // Set absolute left/right volumes
    void setPanAbsolute(handle voice_handle, float aLVolume, float aRVolume);
    // Set channel volume (volume for a specific speaker)
    void setChannelVolume(handle voice_handle, size_t aChannel, float aVolume);
    // Set overall volume
    void setVolume(handle voice_handle, float aVolume);
    // Set delay, in samples, before starting to play samples. Calling this on a live sound will
    // cause glitches.
    void setDelaySamples(handle voice_handle, size_t aSamples);

    // Set up volume fader
    void fadeVolume(handle voice_handle, float aTo, time_t aTime);
    // Set up panning fader
    void fadePan(handle voice_handle, float aTo, time_t aTime);
    // Set up relative play speed fader
    void fadeRelativePlaySpeed(handle voice_handle, float aTo, time_t aTime);
    // Set up global volume fader
    void fadeGlobalVolume(float aTo, time_t aTime);
    // Schedule a stream to pause
    void schedulePause(handle voice_handle, time_t aTime);
    // Schedule a stream to stop
    void scheduleStop(handle voice_handle, time_t aTime);

    // Set up volume oscillator
    void oscillateVolume(handle voice_handle, float aFrom, float aTo, time_t aTime);
    // Set up panning oscillator
    void oscillatePan(handle voice_handle, float aFrom, float aTo, time_t aTime);
    // Set up relative play speed oscillator
    void oscillateRelativePlaySpeed(handle voice_handle, float aFrom, float aTo, time_t aTime);
    // Set up global volume oscillator
    void oscillateGlobalVolume(float aFrom, float aTo, time_t aTime);

    // Set global filters. Set to nullptr to clear the filter.
    void setGlobalFilter(size_t aFilterId, Filter* aFilter);

    // Enable or disable visualization data gathering
    void setVisualizationEnable(bool aEnable);

    // Calculate and get 256 floats of FFT data for visualization. Visualization has to be enabled
    // before use.
    auto calcFFT() -> float*;

    // Get 256 floats of wave data for visualization. Visualization has to be enabled before use.
    auto getWave() -> float*;

    // Get approximate output volume for a channel for visualization. Visualization has to be
    // enabled before use.
    auto getApproximateVolume(size_t aChannel) -> float;

    // Get current loop count. Returns 0 if handle is not valid. (All audio sources may not update
    // loop count)
    auto getLoopCount(handle voice_handle) -> size_t;

    // Get audiosource-specific information from a voice.
    auto getInfo(handle voice_handle, size_t aInfoKey) -> float;

    // Create a voice group. Returns 0 if unable (out of voice groups / out of memory)
    auto createVoiceGroup() -> handle;
    // Destroy a voice group.
    void destroyVoiceGroup(handle aVoiceGroupHandle);
    // Add a voice handle to a voice group
    void addVoiceToGroup(handle aVoiceGroupHandle, handle voice_handle);
    // Is this handle a valid voice group?
    auto isVoiceGroup(handle aVoiceGroupHandle) -> bool;
    // Is this voice group empty?
    auto isVoiceGroupEmpty(handle aVoiceGroupHandle) -> bool;

    // Perform 3d audio parameter update
    void update3dAudio();

    // Set the speed of sound constant for doppler
    void set3dSoundSpeed(float aSpeed);
    // Get the current speed of sound constant for doppler
    auto get3dSoundSpeed() const -> float;
    // Set 3d listener parameters
    void set3dListenerParameters(Vector3 pos, Vector3 at, Vector3 up, Vector3 velocity = {});
    // Set 3d listener position
    void set3dListenerPosition(Vector3 value);
    // Set 3d listener "at" vector
    void set3dListenerAt(Vector3 value);
    // set 3d listener "up" vector
    void set3dListenerUp(Vector3 value);
    // Set 3d listener velocity
    void set3dListenerVelocity(Vector3 value);

    // Set 3d audio source parameters
    void set3dSourceParameters(handle voice_handle, Vector3 aPos, Vector3 aVelocity = {});

    // Set 3d audio source position
    void set3dSourcePosition(handle voice_handle, Vector3 pos);
    // Set 3d audio source velocity
    void set3dSourceVelocity(handle voice_handle, Vector3 velocity);
    // Set 3d audio source min/max distance (distance < min means max volume)
    void set3dSourceMinMaxDistance(handle voice_handle, float aMinDistance, float aMaxDistance);
    // Set 3d audio source attenuation parameters
    void set3dSourceAttenuation(handle           voice_handle,
                                AttenuationModel aAttenuationModel,
                                float            aAttenuationRolloffFactor);
    // Set 3d audio source doppler factor to reduce or enhance doppler effect. Default = 1.0
    void set3dSourceDopplerFactor(handle voice_handle, float aDopplerFactor);

    // Rest of the stuff is used internally.

    // Returns mixed float samples in buffer. Called by the back-end, or user with null driver.
    void mix(float* aBuffer, size_t aSamples);
    // Returns mixed 16-bit signed integer samples in buffer. Called by the back-end, or user with
    // null driver.
    void mixSigned16(short* aBuffer, size_t aSamples);

  public:
    // Mix N samples * M channels. Called by other mix_ functions.
    void mix_internal(size_t aSamples, size_t aStride);

    // Handle rest of initialization (called from backend)
    void postinit_internal(size_t      sample_rate,
                           size_t      buffer_size,
                           EngineFlags flags,
                           size_t      channels);

    // Update list of active voices
    void calcActiveVoices_internal();
    // Map resample buffers to active voices
    void mapResampleBuffers_internal();
    // Perform mixing for a specific bus
    void mixBus_internal(float*    aBuffer,
                         size_t    aSamplesToRead,
                         size_t    aBufferSize,
                         float*    aScratch,
                         size_t    aBus,
                         float     aSamplerate,
                         size_t    aChannels,
                         Resampler aResampler);
    // Find a free voice, stopping the oldest if no free voice is found.
    auto findFreeVoice_internal() -> int;
    // Converts handle to voice, if the handle is valid. Returns -1 if not.
    auto getVoiceFromHandle_internal(handle voice_handle) const -> int;
    // Converts voice + playindex into handle
    auto getHandleFromVoice_internal(size_t aVoice) const -> handle;
    // Stop voice (not handle).
    void stopVoice_internal(size_t aVoice);
    // Set voice (not handle) pan.
    void setVoicePan_internal(size_t aVoice, float aPan);
    // Set voice (not handle) relative play speed.
    void setVoiceRelativePlaySpeed_internal(size_t aVoice, float aSpeed);
    // Set voice (not handle) volume.
    void setVoiceVolume_internal(size_t aVoice, float aVolume);
    // Set voice (not handle) pause state.
    void setVoicePause_internal(size_t aVoice, int aPause);
    // Update overall volume from set and 3d volumes
    void updateVoiceVolume_internal(size_t aVoice);
    // Update overall relative play speed from set and 3d speeds
    void updateVoiceRelativePlaySpeed_internal(size_t aVoice);
    // Perform 3d audio calculation for array of voices
    void update3dVoices_internal(std::span<const size_t> voiceList);
    // Clip the samples in the buffer

    void clip_internal(const AlignedFloatBuffer& aBuffer,
                       const AlignedFloatBuffer& aDestBuffer,
                       size_t                    aSamples,
                       float                     aVolume0,
                       float                     aVolume1);
    // Remove all non-active voices from group
    void trimVoiceGroup_internal(handle aVoiceGroupHandle);

    // Get pointer to the zero-terminated array of voice handles in a voice group
    handle* voiceGroupHandleToArray_internal(handle aVoiceGroupHandle) const;

    // Lock audio thread mutex.
    void lockAudioMutex_internal();

    // Unlock audio thread mutex.
    void unlockAudioMutex_internal();

    // Back-end data; content is up to the back-end implementation.
    void* m_backend_data = nullptr;
    // Pointer for the audio thread mutex.
    void* m_audio_thread_mutex = nullptr;
    // Flag for when we're inside the mutex, used for debugging.
    bool m_inside_audio_thread_mutex = false;
    // Called by SoLoud to shut down the back-end. If nullptr, not called. Should be set by
    // back-end.
    soloudCallFunction m_backend_cleanup_func = nullptr;

    // Some backends like CoreAudio on iOS must be paused/resumed in some cases. On incoming call as
    // instance.
    soloudResultFunction m_backend_pause_func  = nullptr;
    soloudResultFunction m_backend_resume_func = nullptr;

    // Max. number of active voices. Busses and tickable inaudibles also count against this.
    size_t m_max_active_voices = 16;

    // Highest voice in use so far
    size_t m_highest_voice = 0;

    // Scratch buffer, used for resampling.
    AlignedFloatBuffer m_scratch;

    // Current size of the scratch, in samples.
    size_t m_scratch_size = 0;

    // Output scratch buffer, used in mix_().
    AlignedFloatBuffer m_output_scratch;

    // Pointers to resampler buffers, two per active voice.
    std::vector<float*> m_resample_data;

    // Actual allocated memory for resampler buffers
    AlignedFloatBuffer m_resample_data_buffer;

    // Owners of the resample data
    std::vector<std::shared_ptr<AudioSourceInstance>> m_resample_data_owner;

    // Audio voices.
    std::array<std::shared_ptr<AudioSourceInstance>, cer::voice_count> m_voice;

    // Resampler for the main bus
    Resampler m_resampler = default_resampler;

    // Output sample rate (not float)
    size_t m_samplerate = 0;

    // Output channel count
    size_t m_channels = 2;

    // Maximum size of output buffer; used to calculate needed scratch.
    size_t m_buffer_size = 0;

    EngineFlags m_flags;

    // Global volume. Applied before clipping.
    float m_global_volume = 0.0f;

    // Post-clip scaler. Applied after clipping.
    float m_post_clip_scaler = 0.0f;

    // Current play index. Used to create audio handles.
    size_t m_play_index = 0;

    // Current sound source index. Used to create sound source IDs.
    size_t m_audio_source_id = 1;

    // Fader for the global volume.
    Fader m_global_volume_fader;

    // Global stream time, for the global volume fader.
    time_t m_stream_time = 0;

    // Last time seen by the playClocked call
    time_t m_last_clocked_time = 0;

    // Global filter
    std::array<Filter*, filters_per_stream> m_filter{};

    // Global filter instance
    std::array<std::shared_ptr<FilterInstance>, filters_per_stream> m_filter_instance{};

    // Approximate volume for channels.
    std::array<float, max_channels> m_visualization_channel_volume{};

    // Mono-mixed wave data for visualization and for visualization FFT input
    std::array<float, 256> m_visualization_wave_data{};

    // FFT output data
    std::array<float, 256> m_fft_data{};

    // Snapshot of wave data for visualization
    std::array<float, 256> m_wave_data{};

    Vector3 m_3d_position{};
    Vector3 m_3d_at{0, 0, -1};
    Vector3 m_3d_up{0, 1, 0};
    Vector3 m_3d_velocity;

    // 3d speed of sound (for doppler)
    float m_3d_sound_speed = 343.3f;

    // 3d position of speakers
    std::array<Vector3, max_channels> m_3d_speaker_position;

    // Data related to 3d processing, separate from AudioSource so we can do 3d calculations without
    // audio mutex.
    AudioSourceInstance3dData m_3d_data[cer::voice_count];

    // For each voice group, first int is number of ints alocated.
    size_t** m_voice_group       = nullptr;
    size_t   m_voice_group_count = 0;

    // List of currently active voices
    std::array<size_t, cer::voice_count> m_active_voice{};

    // Number of currently active voices
    size_t m_active_voice_count = 0;

    // Active voices list needs to be recalculated
    bool m_active_voice_dirty = true;

    // Previous AudioDevice members:
  private:
    struct SoundHash
    {
        auto operator()(const Sound& sound) const -> size_t;
    };

    bool                                 m_was_initialized_successfully{};
    std::unordered_set<Sound, SoundHash> m_playing_sounds;
};
} // namespace cer
