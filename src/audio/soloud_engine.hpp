#pragma once

#include "audio/soloud_misc.hpp"
#include "soloud.hpp"
#include "soloud_audiosource.hpp"
#include <memory>
#include <optional>
#include <span>
#include <vector>

namespace cer
{
class AudioSource;
class AudioSourceInstance;
class Filter;

struct EngineFlags
{
    bool ClipRoundoff : 1        = true;
    bool EnableVisualization : 1 = false;
    bool NoFpuRegisterChange : 1 = false;
};

// Soloud core class.
class Engine
{
  public:
    explicit Engine(EngineFlags           flags       = {},
                    std::optional<size_t> aSamplerate = std::nullopt,
                    std::optional<size_t> aBufferSize = std::nullopt,
                    size_t                aChannels   = 2);

    ~Engine() noexcept;

    void pause();

    void resume();

    // Returns current backend channel count (1 mono, 2 stereo, etc)
    size_t getBackendChannels() const;

    // Returns current backend sample rate
    size_t getBackendSamplerate() const;

    // Returns current backend buffer size
    size_t getBackendBufferSize() const;

    // Set speaker position in 3d space
    void setSpeakerPosition(size_t aChannel, Vector3 value);

    // Get speaker position in 3d space
    Vector3 getSpeakerPosition(size_t aChannel) const;

    // Start playing a sound. Returns voice handle, which can be ignored or used to alter the
    // playing sound's parameters. Negative volume means to use default.
    handle play(AudioSource& aSound,
                float        aVolume = -1.0f,
                float        aPan    = 0.0f,
                bool         aPaused = 0,
                size_t       aBus    = 0);
    // Start playing a sound delayed in relation to other sounds called via this function. Negative
    // volume means to use default.
    handle playClocked(time_t       aSoundTime,
                       AudioSource& aSound,
                       float        aVolume = -1.0f,
                       float        aPan    = 0.0f,
                       size_t       aBus    = 0);
    // Start playing a 3d audio source
    handle play3d(AudioSource& aSound,
                  Vector3      aPos,
                  Vector3      aVel    = {},
                  float        aVolume = 1.0f,
                  bool         aPaused = 0,
                  size_t       aBus    = 0);
    // Start playing a 3d audio source, delayed in relation to other sounds called via this
    // function.
    handle play3dClocked(time_t       aSoundTime,
                         AudioSource& aSound,
                         Vector3      aPos,
                         Vector3      aVel    = {},
                         float        aVolume = 1.0f,
                         size_t       aBus    = 0);
    // Start playing a sound without any panning. It will be played at full volume.
    handle playBackground(AudioSource& aSound,
                          float        aVolume = -1.0f,
                          bool         aPaused = 0,
                          size_t       aBus    = 0);

    // Seek the audio stream to certain point in time. Some streams can't seek backwards. Relative
    // play speed affects time.
    bool seek(handle aVoiceHandle, time_t aSeconds);
    // Stop the sound.
    void stop(handle aVoiceHandle);
    // Stop all voices.
    void stopAll();
    // Stop all voices that play this sound source
    void stopAudioSource(AudioSource& aSound);
    // Count voices that play this audio source
    int countAudioSource(AudioSource& aSound);

    // Set a live filter parameter. Use 0 for the global filters.
    void setFilterParameter(handle aVoiceHandle,
                            size_t aFilterId,
                            size_t aAttributeId,
                            float  aValue);
    // Get a live filter parameter. Use 0 for the global filters.
    std::optional<float> getFilterParameter(handle aVoiceHandle,
                                            size_t aFilterId,
                                            size_t aAttributeId);
    // Fade a live filter parameter. Use 0 for the global filters.
    void fadeFilterParameter(
        handle aVoiceHandle, size_t aFilterId, size_t aAttributeId, float aTo, time_t aTime);
    // Oscillate a live filter parameter. Use 0 for the global filters.
    void oscillateFilterParameter(handle aVoiceHandle,
                                  size_t aFilterId,
                                  size_t aAttributeId,
                                  float  aFrom,
                                  float  aTo,
                                  time_t aTime);

    // Get current play time, in seconds.
    time_t getStreamTime(handle aVoiceHandle);
    // Get current sample position, in seconds.
    time_t getStreamPosition(handle aVoiceHandle);
    // Get current pause state.
    bool getPause(handle aVoiceHandle);
    // Get current volume.
    float getVolume(handle aVoiceHandle);
    // Get current overall volume (set volume * 3d volume)
    float getOverallVolume(handle aVoiceHandle);
    // Get current pan.
    float getPan(handle aVoiceHandle);
    // Get current sample rate.
    float getSamplerate(handle aVoiceHandle);
    // Get current voice protection state.
    bool getProtectVoice(handle aVoiceHandle);
    // Get the current number of busy voices.
    size_t getActiveVoiceCount();
    // Get the current number of voices in SoLoud
    size_t getVoiceCount();
    // Check if the handle is still valid, or if the sound has stopped.
    bool isValidVoiceHandle(handle aVoiceHandle);
    // Get current relative play speed.
    float getRelativePlaySpeed(handle aVoiceHandle);
    // Get current post-clip scaler value.
    float getPostClipScaler() const;
    // Get the current main resampler
    Resampler getMainResampler() const;
    // Get current global volume
    float getGlobalVolume() const;
    // Get current maximum active voice setting
    size_t getMaxActiveVoiceCount() const;
    // Query whether a voice is set to loop.
    bool getLooping(handle aVoiceHandle);
    // Query whether a voice is set to auto-stop when it ends.
    bool getAutoStop(handle aVoiceHandle);
    // Get voice loop point value
    time_t getLoopPoint(handle aVoiceHandle);

    // Set voice loop point value
    void setLoopPoint(handle aVoiceHandle, time_t aLoopPoint);
    // Set voice's loop state
    void setLooping(handle aVoiceHandle, bool aLooping);
    // Set whether sound should auto-stop when it ends
    void setAutoStop(handle aVoiceHandle, bool aAutoStop);
    // Set current maximum active voice setting
    void setMaxActiveVoiceCount(size_t aVoiceCount);
    // Set behavior for inaudible sounds
    void setInaudibleBehavior(handle aVoiceHandle, bool aMustTick, bool aKill);
    // Set the global volume
    void setGlobalVolume(float aVolume);
    // Set the post clip scaler value
    void setPostClipScaler(float aScaler);
    // Set the main resampler
    void setMainResampler(Resampler aResampler);
    // Set the pause state
    void setPause(handle aVoiceHandle, bool aPause);
    // Pause all voices
    void setPauseAll(bool aPause);
    // Set the relative play speed
    void setRelativePlaySpeed(handle aVoiceHandle, float aSpeed);
    // Set the voice protection state
    void setProtectVoice(handle aVoiceHandle, bool aProtect);
    // Set the sample rate
    void setSamplerate(handle aVoiceHandle, float aSamplerate);
    // Set panning value; -1 is left, 0 is center, 1 is right
    void setPan(handle aVoiceHandle, float aPan);
    // Set absolute left/right volumes
    void setPanAbsolute(handle aVoiceHandle, float aLVolume, float aRVolume);
    // Set channel volume (volume for a specific speaker)
    void setChannelVolume(handle aVoiceHandle, size_t aChannel, float aVolume);
    // Set overall volume
    void setVolume(handle aVoiceHandle, float aVolume);
    // Set delay, in samples, before starting to play samples. Calling this on a live sound will
    // cause glitches.
    void setDelaySamples(handle aVoiceHandle, size_t aSamples);

    // Set up volume fader
    void fadeVolume(handle aVoiceHandle, float aTo, time_t aTime);
    // Set up panning fader
    void fadePan(handle aVoiceHandle, float aTo, time_t aTime);
    // Set up relative play speed fader
    void fadeRelativePlaySpeed(handle aVoiceHandle, float aTo, time_t aTime);
    // Set up global volume fader
    void fadeGlobalVolume(float aTo, time_t aTime);
    // Schedule a stream to pause
    void schedulePause(handle aVoiceHandle, time_t aTime);
    // Schedule a stream to stop
    void scheduleStop(handle aVoiceHandle, time_t aTime);

    // Set up volume oscillator
    void oscillateVolume(handle aVoiceHandle, float aFrom, float aTo, time_t aTime);
    // Set up panning oscillator
    void oscillatePan(handle aVoiceHandle, float aFrom, float aTo, time_t aTime);
    // Set up relative play speed oscillator
    void oscillateRelativePlaySpeed(handle aVoiceHandle, float aFrom, float aTo, time_t aTime);
    // Set up global volume oscillator
    void oscillateGlobalVolume(float aFrom, float aTo, time_t aTime);

    // Set global filters. Set to nullptr to clear the filter.
    void setGlobalFilter(size_t aFilterId, Filter* aFilter);

    // Enable or disable visualization data gathering
    void setVisualizationEnable(bool aEnable);

    // Calculate and get 256 floats of FFT data for visualization. Visualization has to be enabled
    // before use.
    float* calcFFT();

    // Get 256 floats of wave data for visualization. Visualization has to be enabled before use.
    float* getWave();

    // Get approximate output volume for a channel for visualization. Visualization has to be
    // enabled before use.
    float getApproximateVolume(size_t aChannel);

    // Get current loop count. Returns 0 if handle is not valid. (All audio sources may not update
    // loop count)
    size_t getLoopCount(handle aVoiceHandle);

    // Get audiosource-specific information from a voice.
    float getInfo(handle aVoiceHandle, size_t aInfoKey);

    // Create a voice group. Returns 0 if unable (out of voice groups / out of memory)
    handle createVoiceGroup();
    // Destroy a voice group.
    void destroyVoiceGroup(handle aVoiceGroupHandle);
    // Add a voice handle to a voice group
    void addVoiceToGroup(handle aVoiceGroupHandle, handle aVoiceHandle);
    // Is this handle a valid voice group?
    bool isVoiceGroup(handle aVoiceGroupHandle);
    // Is this voice group empty?
    bool isVoiceGroupEmpty(handle aVoiceGroupHandle);

    // Perform 3d audio parameter update
    void update3dAudio();

    // Set the speed of sound constant for doppler
    void set3dSoundSpeed(float aSpeed);
    // Get the current speed of sound constant for doppler
    float get3dSoundSpeed() const;
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
    void set3dSourceParameters(handle aVoiceHandle, Vector3 aPos, Vector3 aVelocity = {});

    // Set 3d audio source position
    void set3dSourcePosition(handle aVoiceHandle, Vector3 pos);
    // Set 3d audio source velocity
    void set3dSourceVelocity(handle aVoiceHandle, Vector3 velocity);
    // Set 3d audio source min/max distance (distance < min means max volume)
    void set3dSourceMinMaxDistance(handle aVoiceHandle, float aMinDistance, float aMaxDistance);
    // Set 3d audio source attenuation parameters
    void set3dSourceAttenuation(handle           aVoiceHandle,
                                AttenuationModel aAttenuationModel,
                                float            aAttenuationRolloffFactor);
    // Set 3d audio source doppler factor to reduce or enhance doppler effect. Default = 1.0
    void set3dSourceDopplerFactor(handle aVoiceHandle, float aDopplerFactor);

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
    void postinit_internal(size_t      aSamplerate,
                           size_t      aBufferSize,
                           EngineFlags aFlags,
                           size_t      aChannels);

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
    int findFreeVoice_internal();
    // Converts handle to voice, if the handle is valid. Returns -1 if not.
    int getVoiceFromHandle_internal(handle aVoiceHandle) const;
    // Converts voice + playindex into handle
    handle getHandleFromVoice_internal(size_t aVoice) const;
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
    void* mBackendData = nullptr;
    // Pointer for the audio thread mutex.
    void* mAudioThreadMutex = nullptr;
    // Flag for when we're inside the mutex, used for debugging.
    bool mInsideAudioThreadMutex = false;
    // Called by SoLoud to shut down the back-end. If nullptr, not called. Should be set by
    // back-end.
    soloudCallFunction mBackendCleanupFunc = nullptr;

    // Some backends like CoreAudio on iOS must be paused/resumed in some cases. On incoming call as
    // instance.
    soloudResultFunction mBackendPauseFunc  = nullptr;
    soloudResultFunction mBackendResumeFunc = nullptr;

    // Max. number of active voices. Busses and tickable inaudibles also count against this.
    size_t mMaxActiveVoices = 16;

    // Highest voice in use so far
    size_t mHighestVoice = 0;

    // Scratch buffer, used for resampling.
    AlignedFloatBuffer mScratch;

    // Current size of the scratch, in samples.
    size_t mScratchSize = 0;

    // Output scratch buffer, used in mix_().
    AlignedFloatBuffer mOutputScratch;

    // Pointers to resampler buffers, two per active voice.
    std::vector<float*> mResampleData;

    // Actual allocated memory for resampler buffers
    AlignedFloatBuffer mResampleDataBuffer;

    // Owners of the resample data
    std::vector<std::shared_ptr<AudioSourceInstance>> mResampleDataOwner;

    // Audio voices.
    std::array<std::shared_ptr<AudioSourceInstance>, VOICE_COUNT> mVoice;

    // Resampler for the main bus
    Resampler mResampler = default_resampler;

    // Output sample rate (not float)
    size_t mSamplerate = 0;

    // Output channel count
    size_t mChannels = 2;

    // Maximum size of output buffer; used to calculate needed scratch.
    size_t mBufferSize = 0;

    EngineFlags mFlags;

    // Global volume. Applied before clipping.
    float mGlobalVolume = 0.0f;

    // Post-clip scaler. Applied after clipping.
    float mPostClipScaler = 0.0f;

    // Current play index. Used to create audio handles.
    size_t mPlayIndex = 0;

    // Current sound source index. Used to create sound source IDs.
    size_t mAudioSourceID = 1;

    // Fader for the global volume.
    Fader mGlobalVolumeFader;

    // Global stream time, for the global volume fader.
    time_t mStreamTime = 0;

    // Last time seen by the playClocked call
    time_t mLastClockedTime = 0;

    // Global filter
    std::array<Filter*, FILTERS_PER_STREAM> mFilter{};

    // Global filter instance
    std::array<std::shared_ptr<FilterInstance>, FILTERS_PER_STREAM> mFilterInstance{};

    // Approximate volume for channels.
    std::array<float, MAX_CHANNELS> mVisualizationChannelVolume{};

    // Mono-mixed wave data for visualization and for visualization FFT input
    std::array<float, 256> mVisualizationWaveData{};

    // FFT output data
    std::array<float, 256> mFFTData{};

    // Snapshot of wave data for visualization
    std::array<float, 256> mWaveData{};

    Vector3 m3dPosition{};
    Vector3 m3dAt{0, 0, -1};
    Vector3 m3dUp{0, 1, 0};
    Vector3 m3dVelocity;

    // 3d speed of sound (for doppler)
    float m3dSoundSpeed = 343.3f;

    // 3d position of speakers
    std::array<Vector3, MAX_CHANNELS> m3dSpeakerPosition;

    // Data related to 3d processing, separate from AudioSource so we can do 3d calculations without
    // audio mutex.
    AudioSourceInstance3dData m3dData[VOICE_COUNT];

    // For each voice group, first int is number of ints alocated.
    size_t** mVoiceGroup;
    size_t   mVoiceGroupCount;

    // List of currently active voices
    std::array<size_t, VOICE_COUNT> mActiveVoice{};

    // Number of currently active voices
    size_t mActiveVoiceCount = 0;

    // Active voices list needs to be recalculated
    bool mActiveVoiceDirty = true;
};
}; // namespace cer
