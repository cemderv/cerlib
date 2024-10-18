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
#include <memory>

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
    virtual float collide(AudioDevice*               engine,
                          AudioSourceInstance3dData& aAudioInstance3dData,
                          int                        aUserData) = 0;
};

class AudioAttenuator
{
  public:
    virtual ~AudioAttenuator() noexcept = default;

    virtual float attenuate(float aDistance,
                            float aMinDistance,
                            float aMaxDistance,
                            float aRolloffFactor) = 0;
};

struct AudioSourceInstanceFlagsData
{
    bool None : 1 = false;
    // This audio instance loops (if supported)
    bool Looping : 1 = false;
    // This audio instance is protected - won't get stopped if we run out of voices
    bool Protected : 1 = false;
    // This audio instance is paused
    bool Paused : 1 = false;
    // This audio instance is affected by 3d processing
    bool Process3D : 1 = false;
    // This audio instance has listener-relative 3d coordinates
    bool ListenerRelative : 1 = false;
    // Currently inaudible
    bool Inaudible : 1 = false;
    // If inaudible, should be killed (default = don't kill kill)
    bool InaudibleKill : 1 = false;
    // If inaudible, should still be ticked (default = pause)
    bool InaudibleTick : 1 = false;
    // Don't auto-stop sound
    bool DisableAutostop : 1 = false;
};

class AudioSourceInstance3dData
{
  public:
    AudioSourceInstance3dData() = default;

    explicit AudioSourceInstance3dData(const AudioSource& source);

    // 3d position
    Vector3 m3dPosition;

    // 3d velocity
    Vector3 m3dVelocity;

    // 3d min distance
    float m3dMinDistance = 0.0f;

    // 3d max distance
    float m3dMaxDistance = 1000000.0f;

    // 3d attenuation rolloff factor
    float m3dAttenuationRolloff = 1.0f;

    // 3d attenuation model
    AttenuationModel m3dAttenuationModel = AttenuationModel::NoAttenuation;

    // 3d doppler factor
    float m3dDopplerFactor = 1.0f;

    // Pointer to a custom audio collider object
    AudioCollider* mCollider = nullptr;

    // Pointer to a custom audio attenuator object
    AudioAttenuator* mAttenuator = nullptr;

    // User data related to audio collider
    int mColliderData = 0;

    // Doppler sample rate multiplier
    float mDopplerValue = 0.0f;

    // Overall 3d volume
    float m3dVolume = 0.0f;

    // Channel volume
    std::array<float, max_channels> mChannelVolume{};

    // Copy of flags
    AudioSourceInstanceFlagsData mFlags;

    // Latest handle for this voice
    handle mHandle = 0;
};

// Base class for audio instances
class AudioSourceInstance
{
  public:
    AudioSourceInstance();

    virtual ~AudioSourceInstance() noexcept = default;

    // Play index; used to identify instances from handles
    size_t mPlayIndex = 0;

    // Loop count
    size_t mLoopCount = 0;

    AudioSourceInstanceFlagsData mFlags;

    // Pan value, for getPan()
    float mPan = 0.0f;

    // Volume for each channel (panning)
    std::array<float, max_channels> mChannelVolume{};

    // Set volume
    float mSetVolume = 1.0f;

    // Overall volume overall = set * 3d
    float mOverallVolume = 0.0f;

    // Base samplerate; samplerate = base samplerate * relative play speed
    float mBaseSamplerate = 44100.0f;

    // Samplerate; samplerate = base samplerate * relative play speed
    float mSamplerate = 44100.0f;

    // Number of channels this audio source produces
    size_t mChannels = 1;

    // Relative play speed; samplerate = base samplerate * relative play speed
    float mSetRelativePlaySpeed = 1.0f;

    // Overall relative plays peed; overall = set * 3d
    float mOverallRelativePlaySpeed = 1.0f;

    // How long this stream has played, in seconds.
    time_t mStreamTime = 0.0f;

    // Position of this stream, in seconds.
    time_t mStreamPosition = 0.0f;

    // Fader for the audio panning
    Fader mPanFader;

    // Fader for the audio volume
    Fader mVolumeFader;

    // Fader for the relative play speed
    Fader mRelativePlaySpeedFader;

    // Fader used to schedule pausing of the stream
    Fader mPauseScheduler;

    // Fader used to schedule stopping of the stream
    Fader mStopScheduler;

    // Affected by some fader
    int mActiveFader = 0;

    // Current channel volumes, used to ramp the volume changes to avoid clicks
    std::array<float, max_channels> mCurrentChannelVolume{};

    // ID of the sound source that generated this instance
    size_t mAudioSourceID = 0;

    // Handle of the bus this audio instance is playing on. 0 for root.
    size_t mBusHandle = ~0u;

    // Filter pointer
    std::array<std::shared_ptr<FilterInstance>, filters_per_stream> mFilter{};

    // Initialize instance. Mostly internal use.
    void init(const AudioSource& source, int aPlayIndex);

    // Pointers to buffers for the resampler
    std::array<float*, 2> mResampleData{};

    // Sub-sample playhead; 16.16 fixed point
    size_t mSrcOffset = 0;

    // Samples left over from earlier pass
    size_t mLeftoverSamples = 0;

    // Number of samples to delay streaming
    size_t mDelaySamples = 0;

    // When looping, start playing from this time
    time_t mLoopPoint = 0;

    // Get N samples from the stream to the buffer. Report samples written.
    virtual size_t getAudio(float* aBuffer, size_t aSamplesToRead, size_t aBufferSize) = 0;

    // Has the stream ended?
    virtual bool hasEnded() = 0;

    // Seek to certain place in the stream. Base implementation is generic "tape" seek (and slow).
    virtual bool seek(time_t aSeconds, float* mScratch, size_t mScratchSize);

    // Rewind stream. Base implementation returns NOT_IMPLEMENTED, meaning it can't rewind.
    virtual bool rewind();

    // Get information. Returns 0 by default.
    virtual float getInfo(size_t aInfoKey);
};

// Base class for audio sources
class AudioSource
{
  public:
    AudioSource() = default;

    virtual ~AudioSource() noexcept;

    // Create instance from the audio source. Called from within Soloud class.
    virtual std::shared_ptr<AudioSourceInstance> createInstance() = 0;

    // Set filter. Set to nullptr to clear the filter.
    virtual void setFilter(size_t aFilterId, Filter* aFilter);

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
    time_t loop_point = 0;
};
}; // namespace cer
