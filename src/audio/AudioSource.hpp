/*
SoLoud audio engine
Copyright (c) 2013-2015 Jari Komppa

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
   distribution.
*/

#pragma once

#include "audio/Fader.hpp"
#include "audio/Filter.hpp"
#include "cerlib/Vector3.hpp"
#include <array>
#include <cerlib/SmartPointers.hpp>

namespace cer
{
class AudioSource;
class AudioSourceInstance;
class AudioSourceInstance3dData;

class AudioCollider
{
  public:
    virtual ~AudioCollider() noexcept = default;

    // Calculate volume multiplier. Assumed to return value between 0 and 1.
    virtual auto collide(AudioDevice*               engine,
                         AudioSourceInstance3dData& audio_instance3d_data,
                         int                        user_data) -> float = 0;
};

class AudioAttenuator
{
  public:
    virtual ~AudioAttenuator() noexcept = default;

    virtual auto attenuate(float distance,
                           float min_distance,
                           float max_distance,
                           float rolloff_factor) -> float = 0;
};

struct AudioSourceInstanceFlagsData
{
    bool none : 1 = false;
    // This audio instance loops (if supported)
    bool loops : 1 = false;
    // This audio instance is protected - won't get stopped if we run out of voices
    bool is_protected : 1 = false;
    // This audio instance is paused
    bool is_paused : 1 = false;
    // This audio instance is affected by 3d processing
    bool process_3d : 1 = false;
    // This audio instance has listener-relative 3d coordinates
    bool listener_relative : 1 = false;
    // Currently inaudible
    bool inaudible : 1 = false;
    // If inaudible, should be killed (default = don't kill kill)
    bool inaudible_kill : 1 = false;
    // If inaudible, should still be ticked (default = pause)
    bool inaudible_tick : 1 = false;
    // Don't auto-stop sound
    bool disable_autostop : 1 = false;
};

class AudioSourceInstance3dData
{
  public:
    AudioSourceInstance3dData() = default;

    explicit AudioSourceInstance3dData(const AudioSource& source);

    // 3d position
    Vector3 position_3d{};

    // 3d velocity
    Vector3 velocity_3d{};

    // 3d min distance
    float min_distance_3d = 0.0f;

    // 3d max distance
    float max_distance_3d = 1000000.0f;

    // 3d attenuation rolloff factor
    float attenuation_rolloff_3d = 1.0f;

    // 3d attenuation model
    AttenuationModel attenuation_model_3d = AttenuationModel::NoAttenuation;

    // 3d doppler factor
    float doppler_factor_3d = 1.0f;

    // Pointer to a custom audio collider object
    AudioCollider* collider = nullptr;

    // Pointer to a custom audio attenuator object
    AudioAttenuator* attenuator = nullptr;

    // User data related to audio collider
    int collider_data = 0;

    // Doppler sample rate multiplier
    float doppler_value = 0.0f;

    // Overall 3d volume
    float volume_3d = 0.0f;

    // Channel volume
    std::array<float, max_channels> channel_volume{};

    // Copy of flags
    AudioSourceInstanceFlagsData flags;

    // Latest handle for this voice
    SoundHandle handle = 0;
};

// Base class for audio instances
class AudioSourceInstance
{
  public:
    AudioSourceInstance();

    virtual ~AudioSourceInstance() noexcept = default;

    // Initialize instance. Mostly internal use.
    void init(const AudioSource& source, size_t play_index);

    // Get N samples from the stream to the buffer. Report samples written.
    virtual auto audio(float* buffer, size_t samples_to_read, size_t buffer_size) -> size_t = 0;

    // Has the stream ended?
    virtual auto has_ended() -> bool = 0;

    // Seek to certain place in the stream. Base implementation is generic "tape" seek (and slow).
    virtual auto seek(SoundTime seconds, float* scratch, size_t scratch_size) -> bool;

    // Rewind stream. Base implementation returns NOT_IMPLEMENTED, meaning it can't rewind.
    virtual auto rewind() -> bool;

    // Get information. Returns 0 by default.
    virtual auto get_info(size_t info_key) -> float;

    // Play index; used to identify instances from handles
    size_t play_index = 0;

    // Loop count
    size_t loop_count = 0;

    AudioSourceInstanceFlagsData flags;

    // Pan value, for getPan()
    float pan = 0.0f;

    // Volume for each channel (panning)
    std::array<float, max_channels> channel_volume{};

    // Set volume
    float set_volume = 1.0f;

    // Overall volume overall = set * 3d
    float overall_volume = 0.0f;

    // Base samplerate; samplerate = base samplerate * relative play speed
    float base_sample_rate = 44100.0f;

    // Samplerate; samplerate = base samplerate * relative play speed
    float sample_rate = 44100.0f;

    // Number of channels this audio source produces
    size_t channel_count = 1;

    // Relative play speed; samplerate = base samplerate * relative play speed
    float set_relative_play_speed = 1.0f;

    // Overall relative plays peed; overall = set * 3d
    float overall_relative_play_speed = 1.0f;

    // How long this stream has played, in seconds.
    SoundTime stream_time = 0.0f;

    // Position of this stream, in seconds.
    SoundTime stream_position = 0.0f;

    // Fader for the audio panning
    Fader pan_fader;

    // Fader for the audio volume
    Fader volume_fader;

    // Fader for the relative play speed
    Fader relative_play_speed_fader;

    // Fader used to schedule pausing of the stream
    Fader pause_scheduler;

    // Fader used to schedule stopping of the stream
    Fader stop_scheduler;

    // Affected by some fader
    int active_fader = 0;

    // Current channel volumes, used to ramp the volume changes to avoid clicks
    std::array<float, max_channels> current_channel_volume{};

    // ID of the sound source that generated this instance
    size_t audio_source_id = 0;

    // Handle of the bus this audio instance is playing on. 0 for root.
    size_t bus_handle = ~0u;

    // Filter pointer
    std::array<SharedPtr<FilterInstance>, filters_per_stream> filter{};

    // Pointers to buffers for the resampler
    std::array<float*, 2> resample_data{};

    // Sub-sample playhead; 16.16 fixed point
    size_t src_offset = 0;

    // Samples left over from earlier pass
    size_t leftover_samples = 0;

    // Number of samples to delay streaming
    size_t delay_samples = 0;

    // When looping, start playing from this time
    SoundTime loop_point = 0;
};

// Base class for audio sources
class AudioSource
{
  public:
    AudioSource() = default;

    virtual ~AudioSource() noexcept;

    // Create instance from the audio source. Called from within Soloud class.
    virtual auto create_instance() -> SharedPtr<AudioSourceInstance> = 0;

    // Set filter. Set to nullptr to clear the filter.
    virtual void set_filter(size_t filter_id, Filter* filter);

    // Stop all instances of this audio source
    void stop();

    // The instances from this audio source should loop
    bool should_loop : 1 = false;

    // Only one instance of this audio source should play at the same time
    bool single_instance : 1 = false;

    // Visualization data gathering enabled. Only for busses.
    bool visualization_data : 1 = false;

    // Audio instances created from this source are affected by 3d processing
    bool process_3d : 1 = false;

    // Audio instances created from this source have listener-relative 3d coordinates
    bool listener_relative : 1 = false;

    // Delay start of sound by the distance from listener
    bool distance_delay : 1 = false;

    // If inaudible, should be killed (default)
    bool inaudible_kill : 1 = false;

    // If inaudible, should still be ticked (default = pause)
    bool inaudible_tick : 1 = false;

    // Disable auto-stop
    bool disable_autostop : 1 = false;

    // Base sample rate, used to initialize instances
    float base_sample_rate = 44'100.0f;

    // Default volume for created instances
    float volume = 1.0f;

    // Number of channels this audio source produces
    size_t channel_count = 1;

    // Sound source ID. Assigned by SoLoud the first time it's played.
    size_t audio_source_id = 0;

    // 3d min distance
    float min_distance_3d = 1.0f;

    // 3d max distance
    float max_distance_3d = 1'000'000.0f;

    // 3d attenuation rolloff factor
    float attenuation_rolloff_3d = 1.0f;

    // 3d attenuation model
    AttenuationModel attenuation_model_3d = AttenuationModel::NoAttenuation;

    // 3d doppler factor
    float doppler_factor_3d = 1.0f;

    // Filter pointer
    std::array<Filter*, filters_per_stream> filter{};

    // Pointer to the Soloud object. Needed to stop all instances in dtor.
    AudioDevice* engine = nullptr;

    // Pointer to a custom audio collider object
    AudioCollider* collider = nullptr;

    // Pointer to custom attenuator object
    AudioAttenuator* attenuator = nullptr;

    // User data related to audio collider
    int collider_data = 0;

    // When looping, start playing from this time
    SoundTime loop_point = 0;
};
}; // namespace cer
