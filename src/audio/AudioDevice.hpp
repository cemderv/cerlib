// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "audio/AudioSource.hpp"
#include "audio/Common.hpp"
#include "audio/Misc.hpp"
#include "cerlib/Sound.hpp"
#include "cerlib/SoundTypes.hpp"
#include <cerlib/CopyMoveMacros.hpp>
#include <cerlib/HashSet.hpp>
#include <cerlib/List.hpp>
#include <cerlib/Option.hpp>
#include <span>

namespace cer
{
class Sound;
class SoundChannel;

class AudioDevice
{
  public:
    explicit AudioDevice(EngineFlags flags,
                         size_t      sample_rate,
                         size_t      buffer_size,
                         size_t      channels);

    forbid_copy_and_move(AudioDevice);

    ~AudioDevice() noexcept;

    auto play_sound(const Sound&      sound,
                    float             volume,
                    float             pan,
                    bool              start_paused,
                    Option<SoundTime> delay) -> SoundChannel;

    void play_sound_fire_and_forget(const Sound&      sound,
                                    float             volume,
                                    float             pan,
                                    Option<SoundTime> delay);

    auto play_sound_in_background(const Sound& sound, float volume, bool start_paused)
        -> SoundChannel;

    void stop_all_sounds();

    void pause_all_sounds();

    void resume_all_sounds();

    auto global_volume() const -> float;

    void purge_sounds();

    // From SoLoud::Soloud:
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
              size_t       bus    = 0) -> SoundHandle;

    // Start playing a sound delayed in relation to other sounds called via this function. Negative
    // volume means to use default.
    auto play_clocked(SoundTime    sound_time,
                      AudioSource& sound,
                      float        volume = -1.0f,
                      float        pan    = 0.0f,
                      size_t       bus    = 0) -> SoundHandle;

    // Start playing a 3d audio source
    auto play_3d(AudioSource& sound,
                 Vector3      pos,
                 Vector3      vel    = {},
                 float        volume = 1.0f,
                 bool         paused = false,
                 size_t       bus    = 0) -> SoundHandle;

    // Start playing a 3d audio source, delayed in relation to other sounds called via this
    // function.
    auto play3d_clocked(SoundTime    sound_time,
                        AudioSource& sound,
                        Vector3      pos,
                        Vector3      vel    = {},
                        float        volume = 1.0f,
                        size_t       bus    = 0) -> SoundHandle;

    // Start playing a sound without any panning. It will be played at full volume.
    auto play3d_background(AudioSource& sound,
                           float        volume = -1.0f,
                           bool         paused = false,
                           size_t       bus    = 0) -> SoundHandle;

    // Seek the audio stream to certain point in time. Some streams can't seek backwards. Relative
    // play speed affects time.
    auto seek(SoundHandle voice_handle, SoundTime seconds) -> bool;

    // Stop the sound.
    void stop(SoundHandle voice_handle);

    // Stop all voices.
    void stop_all();

    // Stop all voices that play this sound source
    void stop_audio_source(AudioSource& sound);

    // Count voices that play this audio source
    auto count_audio_source(const AudioSource& sound) -> int;

    // Set a live filter parameter. Use 0 for the global filters.
    void set_filter_parameter(SoundHandle voice_handle,
                              size_t      filter_id,
                              size_t      attribute_id,
                              float       value);

    // Get a live filter parameter. Use 0 for the global filters.
    auto filter_parameter(SoundHandle voice_handle, size_t filter_id, size_t attribute_id)
        -> Option<float>;

    // Fade a live filter parameter. Use 0 for the global filters.
    void fade_filter_parameter(
        SoundHandle voice_handle, size_t filter_id, size_t attribute_id, float to, SoundTime time);

    // Oscillate a live filter parameter. Use 0 for the global filters.
    void oscillate_filter_parameter(SoundHandle voice_handle,
                                    size_t      filter_id,
                                    size_t      attribute_id,
                                    float       from,
                                    float       to,
                                    SoundTime   time);

    // Get current play time, in seconds.
    auto stream_time(SoundHandle voice_handle) -> SoundTime;

    // Get current sample position, in seconds.
    auto stream_position(SoundHandle voice_handle) -> SoundTime;

    // Get current pause state.
    auto pause(SoundHandle voice_handle) -> bool;

    // Get current volume.
    auto volume(SoundHandle voice_handle) -> float;

    // Get current overall volume (set volume * 3d volume)
    auto overall_volume(SoundHandle voice_handle) -> float;

    // Get current pan.
    auto pan(SoundHandle voice_handle) -> float;

    // Get current sample rate.
    auto sample_rate(SoundHandle voice_handle) -> float;

    // Get current voice protection state.
    auto is_voice_protected(SoundHandle voice_handle) -> bool;

    // Get the current number of busy voices.
    auto active_voice_count() -> size_t;

    // Get the current number of voices in SoLoud
    auto voice_count() -> size_t;

    // Check if the handle is still valid, or if the sound has stopped.
    auto is_valid_voice_handle(SoundHandle voice_handle) -> bool;

    // Get current relative play speed.
    auto relative_play_speed(SoundHandle voice_handle) -> float;

    // Get current post-clip scaler value.
    auto post_clip_scaler() const -> float;

    // Get the current main resampler
    auto main_resampler() const -> Resampler;

    // Get current maximum active voice setting
    auto max_active_voice_count() const -> size_t;

    // Query whether a voice is set to loop.
    auto is_voice_looping(SoundHandle voice_handle) -> bool;

    // Query whether a voice is set to auto-stop when it ends.
    auto get_auto_stop(SoundHandle voice_handle) -> bool;

    // Get voice loop point value
    auto get_loop_point(SoundHandle voice_handle) -> SoundTime;

    // Set voice loop point value
    void set_loop_point(SoundHandle voice_handle, SoundTime loop_point);

    // Set voice's loop state
    void set_looping(SoundHandle voice_handle, bool looping);

    // Set whether sound should auto-stop when it ends
    void set_auto_stop(SoundHandle voice_handle, bool auto_stop);

    // Set current maximum active voice setting
    void set_max_active_voice_count(size_t voice_count);

    // Set behavior for inaudible sounds
    void set_inaudible_behavior(SoundHandle voice_handle, bool must_tick, bool kill);

    // Set the global volume
    void set_global_volume(float volume);

    // Set the post clip scaler value
    void set_post_clip_scaler(float scaler);

    // Set the main resampler
    void set_main_resampler(Resampler resampler);

    // Set the pause state
    void set_pause(SoundHandle voice_handle, bool pause);

    // Pause all voices
    void set_pause_all(bool pause);

    // Set the relative play speed
    void set_relative_play_speed(SoundHandle voice_handle, float speed);

    // Set the voice protection state
    void set_protect_voice(SoundHandle voice_handle, bool protect);

    // Set the sample rate
    void set_sample_rate(SoundHandle voice_handle, float sample_rate);

    // Set panning value; -1 is left, 0 is center, 1 is right
    void set_pan(SoundHandle voice_handle, float pan);

    // Set absolute left/right volumes
    void set_pan_absolute(SoundHandle voice_handle, float left_volume, float right_volume);

    // Set channel volume (volume for a specific speaker)
    void set_channel_volume(SoundHandle voice_handle, size_t channel, float volume);

    // Set overall volume
    void set_volume(SoundHandle voice_handle, float volume);

    // Set delay, in samples, before starting to play samples. Calling this on a live sound will
    // cause glitches.
    void set_delay_samples(SoundHandle voice_handle, size_t samples);

    // Set up volume fader
    void fade_volume(SoundHandle voice_handle, float to, SoundTime time);

    // Set up panning fader
    void fade_pan(SoundHandle voice_handle, float to, SoundTime time);

    // Set up relative play speed fader
    void fade_relative_play_speed(SoundHandle voice_handle, float to, SoundTime time);

    // Set up global volume fader
    void fade_global_volume(float to_volume, SoundTime fade_duration);

    // Schedule a stream to pause
    void schedule_pause(SoundHandle voice_handle, SoundTime time);

    // Schedule a stream to stop
    void schedule_stop(SoundHandle voice_handle, SoundTime time);

    // Set up volume oscillator
    void oscillate_volume(SoundHandle voice_handle, float from, float to, SoundTime time);

    // Set up panning oscillator
    void oscillate_pan(SoundHandle voice_handle, float from, float aTo, SoundTime time);

    // Set up relative play speed oscillator
    void oscillate_relative_play_speed(SoundHandle voice_handle,
                                       float       from,
                                       float       to,
                                       SoundTime   time);
    // Set up global volume oscillator
    void oscillate_global_volume(float from, float to, SoundTime time);

    // Set global filters. Set to nullptr to clear the filter.
    void set_global_filter(size_t filter_id, Filter* filter);

    // Enable or disable visualization data gathering
    void set_visualization_enable(bool enable);

    // Calculate and get 256 floats of FFT data for visualization. Visualization has to be enabled
    // before use.
    auto calc_fft() -> float*;

    // Get 256 floats of wave data for visualization. Visualization has to be enabled before use.
    auto get_wave() -> float*;

    // Get approximate output volume for a channel for visualization. Visualization has to be
    // enabled before use.
    auto get_approximate_volume(size_t channel) -> float;

    // Get current loop count. Returns 0 if handle is not valid. (All audio sources may not update
    // loop count)
    auto get_loop_count(SoundHandle voice_handle) -> size_t;

    // Get audiosource-specific information from a voice.
    auto get_info(SoundHandle voice_handle, size_t info_key) -> float;

    // Create a voice group. Returns 0 if unable (out of voice groups / out of memory)
    auto create_voice_group() -> SoundHandle;

    // Destroy a voice group.
    void destroy_voice_group(SoundHandle voice_group_handle);

    // Add a voice handle to a voice group
    void add_voice_to_group(SoundHandle voice_group_handle, SoundHandle voice_handle);

    // Is this handle a valid voice group?
    auto is_voice_group(SoundHandle voice_group_handle) -> bool;

    // Is this voice group empty?
    auto is_voice_group_empty(SoundHandle voice_group_handle) -> bool;

    // Perform 3d audio parameter update
    void update_3d_audio();

    // Set the speed of sound constant for doppler
    void set_3d_sound_speed(float speed);

    // Get the current speed of sound constant for doppler
    auto get_3d_sound_speed() const -> float;

    // Set 3d listener parameters
    void set_3d_listener_parameters(Vector3 pos, Vector3 at, Vector3 up, Vector3 velocity = {});

    // Set 3d listener position
    void set_3d_listener_position(Vector3 value);

    // Set 3d listener "at" vector
    void set_3d_listener_at(Vector3 value);

    // set 3d listener "up" vector
    void set_3d_listener_up(Vector3 value);

    // Set 3d listener velocity
    void set_3d_listener_velocity(Vector3 value);

    // Set 3d audio source parameters
    void set_3d_source_parameters(SoundHandle voice_handle, Vector3 pos, Vector3 velocity = {});

    // Set 3d audio source position
    void set_3d_source_position(SoundHandle voice_handle, Vector3 pos);

    // Set 3d audio source velocity
    void set_3d_source_velocity(SoundHandle voice_handle, Vector3 velocity);

    // Set 3d audio source min/max distance (distance < min means max volume)
    void set_3d_source_min_max_distance(SoundHandle voice_handle,
                                        float       min_distance,
                                        float       max_distance);

    // Set 3d audio source attenuation parameters
    void set_3d_source_attenuation(SoundHandle      voice_handle,
                                   AttenuationModel attenuation_model,
                                   float            attenuation_rolloff_factor);

    // Set 3d audio source doppler factor to reduce or enhance doppler effect. Default = 1.0
    void set_3d_source_doppler_factor(SoundHandle voice_handle, float doppler_factor);

    // Rest of the stuff is used internally.

    // Returns mixed float samples in buffer. Called by the back-end, or user with null driver.
    void mix(float* buffer, size_t samples);

    // Returns mixed 16-bit signed integer samples in buffer. Called by the back-end, or user with
    // null driver.
    void mix_signed16(short* buffer, size_t samples);

  public:
    // Mix N samples * M channels. Called by other mix_ functions.
    void mix_internal(size_t samples, size_t stride);

    // Handle rest of initialization (called from backend)
    void postinit_internal(size_t sample_rate, size_t buffer_size, size_t channels);

    // Update list of active voices
    void calc_active_voices_internal();

    // Map resample buffers to active voices
    void map_resample_buffers_internal();

    // Perform mixing for a specific bus
    void mix_bus_internal(float*    buffer,
                          size_t    samples_to_read,
                          size_t    buffer_size,
                          float*    scratch,
                          size_t    bus,
                          float     sample_rate,
                          size_t    channels,
                          Resampler resampler);

    // Find a free voice, stopping the oldest if no free voice is found.
    auto find_free_voice_internal() -> size_t;

    // Converts handle to voice, if the handle is valid. Returns -1 if not.
    auto get_voice_from_handle_internal(SoundHandle voice_handle) const -> int;

    // Converts voice + playindex into handle
    auto get_handle_from_voice_internal(size_t voice) const -> SoundHandle;

    // Stop voice (not handle).
    void stop_voice_internal(size_t voice);

    // Set voice (not handle) pan.
    void set_voice_pan_internal(size_t voice, float pan);

    // Set voice (not handle) relative play speed.
    void set_voice_relative_play_speed_internal(size_t voice, float speed);

    // Set voice (not handle) volume.
    void set_voice_volume_internal(size_t voice, float volume);

    // Set voice (not handle) pause state.
    void set_voice_pause_internal(size_t aVoice, bool pause);

    // Update overall volume from set and 3d volumes
    void update_voice_volume_internal(size_t voice);

    // Update overall relative play speed from set and 3d speeds
    void update_voice_relative_play_speed_internal(size_t voice);

    // Perform 3d audio calculation for array of voices
    void update_3d_voices_internal(std::span<const size_t> voice_list);

    // Clip the samples in the buffer
    void clip_internal(const AlignedFloatBuffer& buffer,
                       AlignedFloatBuffer&       dst_buffer,
                       size_t                    samples,
                       float                     volume0,
                       float                     volume1);
    // Remove all non-active voices from group
    void trim_voice_group_internal(SoundHandle voice_group_handle);

    // Get pointer to the zero-terminated array of voice handles in a voice group
    SoundHandle* voice_group_handle_to_array_internal(SoundHandle voice_group_handle) const;

    // Lock audio thread mutex.
    void lock_audio_mutex_internal();

    // Unlock audio thread mutex.
    void unlock_audio_mutex_internal();

    template <typename Action>
    void foreach_voice(SoundHandle voice_handle, const Action& action)
    {
        const auto  handles = std::array<SoundHandle, 2>{voice_handle, 0};
        const auto* handle  = voice_group_handle_to_array_internal(voice_handle);

        lock_audio_mutex_internal();

        if (handle == nullptr)
        {
            handle = handles.data();
        }

        while (*handle)
        {
            if (const auto ch = get_voice_from_handle_internal(*handle); ch != -1)
            {
                action(ch);
            }

            ++handle;
        }

        unlock_audio_mutex_internal();
    }

    template <typename Action>
    void foreach_voice_3d(SoundHandle voice_handle, const Action& action)
    {
        const auto  handles = std::array<SoundHandle, 2>{voice_handle, 0};
        const auto* handle  = voice_group_handle_to_array_internal(voice_handle);

        if (handle == nullptr)
        {
            handle = handles.data();
        }

        while (*handle)
        {
            if (const auto ch = int((*handle & 0xfff)) - 1;
                ch != -1 && m_3d_data[ch].handle == *handle)
            {
                action(ch);
            }

            ++handle;
        }
    }

    auto voices() const -> std::span<const SharedPtr<AudioSourceInstance>, max_voice_count>
    {
        return m_voice;
    }

    auto highest_voice() const -> size_t
    {
        return m_highest_voice;
    }

    auto audio_source_id() const -> size_t
    {
        return m_audio_source_id;
    }

    void increment_audio_source_id()
    {
        ++m_audio_source_id;
    }

    auto channels() const -> size_t
    {
        return m_channels;
    }

    void set_backend_cleanup_func(soloudCallFunction func)
    {
        m_backend_cleanup_func = func;
    }

    void set_backend_pause_func(soloudResultFunction func)
    {
        m_backend_pause_func = func;
    }

    void set_backend_resume_func(soloudResultFunction func)
    {
        m_backend_resume_func = func;
    }

  private:
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
    List<float*> m_resample_data;

    // Actual allocated memory for resampler buffers
    AlignedFloatBuffer m_resample_data_buffer;

    // Owners of the resample data
    List<SharedPtr<AudioSourceInstance>> m_resample_data_owner;

    // Audio voices.
    std::array<SharedPtr<AudioSourceInstance>, cer::max_voice_count> m_voice;

    // Resampler for the main bus
    Resampler m_resampler = default_resampler;

    // Output sample rate (not float)
    size_t m_sample_rate = 0;

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
    SoundTime m_stream_time = 0;

    // Last time seen by the playClocked call
    SoundTime m_last_clocked_time = 0;

    // Global filter
    std::array<Filter*, filters_per_stream> m_filter{};

    // Global filter instance
    std::array<SharedPtr<FilterInstance>, filters_per_stream> m_filter_instance{};

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
    AudioSourceInstance3dData m_3d_data[cer::max_voice_count];

    // For each voice group, first int is number of ints alocated.
    size_t** m_voice_group       = nullptr;
    size_t   m_voice_group_count = 0;

    // List of currently active voices
    std::array<size_t, cer::max_voice_count> m_active_voice{};

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

    bool                      m_was_initialized_successfully{};
    HashSet<Sound, SoundHash> m_playing_sounds;
};
} // namespace cer
