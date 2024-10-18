/*
SoLoud audio engine
Copyright (c) 2013-2020 Jari Komppa

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

#include "audio/soloud_misc.hpp"
#include "soloud_audiosource.hpp"
#include <array>

namespace cer
{
class Bus;
class DuckFilterInstance;

class BusInstance final : public AudioSourceInstance
{
    friend Bus;
    friend DuckFilterInstance;

  public:
    explicit BusInstance(Bus* aParent);

    size_t getAudio(float* aBuffer, size_t aSamplesToRead, size_t aBufferSize) override;

    bool hasEnded() override;

    ~BusInstance() noexcept override;

  private:
    Bus*               mParent;
    size_t             mScratchSize;
    AlignedFloatBuffer mScratch;

    // Approximate volume for channels.
    std::array<float, max_channels> mVisualizationChannelVolume{};

    // Mono-mixed wave data for visualization and for visualization FFT input
    std::array<float, 256> mVisualizationWaveData{};
};

class Bus final : public AudioSource
{
    friend BusInstance;

  public:
    Bus();

    std::shared_ptr<AudioSourceInstance> createInstance() override;

    // Set filter. Set to nullptr to clear the filter.
    void setFilter(size_t aFilterId, Filter* aFilter) override;

    // Play sound through the bus
    handle play(AudioSource& aSound, float aVolume = 1.0f, float aPan = 0.0f, bool aPaused = 0);

    // Play sound through the bus, delayed in relation to other sounds called via this function.
    handle playClocked(time_t       aSoundTime,
                       AudioSource& aSound,
                       float        aVolume = 1.0f,
                       float        aPan    = 0.0f);

    // Start playing a 3d audio source through the bus
    handle play3d(AudioSource& aSound,
                  Vector3      aPos,
                  Vector3      aVel    = {},
                  float        aVolume = 1.0f,
                  bool         aPaused = 0);

    // Start playing a 3d audio source through the bus, delayed in relation to other sounds called
    // via this function.
    handle play3dClocked(time_t       aSoundTime,
                         AudioSource& aSound,
                         Vector3      aPos,
                         Vector3      aVel    = {},
                         float        aVolume = 1.0f);

    // Set number of channels for the bus (default 2)
    void setChannels(size_t aChannels);

    // Enable or disable visualization data gathering
    void setVisualizationEnable(bool aEnable);

    // Move a live sound to this bus
    void annexSound(handle voice_handle);

    // Calculate and get 256 floats of FFT data for visualization. Visualization has to be enabled
    // before use.
    float* calcFFT();

    // Get 256 floats of wave data for visualization. Visualization has to be enabled before use.
    float* getWave();

    // Get approximate volume for output channel for visualization. Visualization has to be enabled
    // before use.
    float getApproximateVolume(size_t aChannel);

    // Get number of immediate child voices to this bus
    size_t getActiveVoiceCount();

    // Get current the resampler for this bus
    Resampler getResampler() const;

    // Set the resampler for this bus
    void setResampler(Resampler aResampler);

  private:
    // Internal: find the bus' channel
    void findBusHandle();

    std::shared_ptr<BusInstance> mInstance;
    size_t                       mChannelHandle = 0;
    Resampler                    mResampler     = default_resampler;

    // FFT output data
    std::array<float, 256> mFFTData{};

    // Snapshot of wave data for visualization
    std::array<float, 256> mWaveData{};
};
}; // namespace cer
