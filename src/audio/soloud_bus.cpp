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

#include "audio/soloud_bus.hpp"
#include "audio/soloud_fft.hpp"
#include "audio/soloud_internal.hpp"
#include "audio/AudioDevice.hpp"
#include <algorithm>
#include <cassert>

namespace cer
{
BusInstance::BusInstance(Bus* aParent)
    : mParent(aParent)
      , mScratchSize(sample_granularity)
      , mScratch(mScratchSize * max_channels)
{
    mFlags.Protected     = true;
    mFlags.InaudibleTick = true;
}

size_t BusInstance::getAudio(float* aBuffer, size_t aSamplesToRead, size_t aBufferSize)
{
    const auto handle = mParent->mChannelHandle;

    if (handle == 0)
    {
        // Avoid reuse of scratch data if this bus hasn't played anything yet
        for (size_t i = 0; i < aBufferSize * mChannels; ++i)
        {
            aBuffer[i] = 0;
        }

        return aSamplesToRead;
    }

    auto* s = mParent->engine;

    s->mixBus_internal(aBuffer,
                       aSamplesToRead,
                       aBufferSize,
                       mScratch.mData,
                       handle,
                       mSamplerate,
                       mChannels,
                       mParent->mResampler);

    if (mParent->visualization_data)
    {
        std::ranges::fill(mVisualizationChannelVolume, 0.0f);

        if (aSamplesToRead > 255)
        {
            for (size_t i = 0; i < 256; ++i)
            {
                mVisualizationWaveData[i] = 0;

                for (size_t j = 0; j < mChannels; ++j)
                {
                    const auto sample = aBuffer[i + aBufferSize * j];

                    if (const auto absvol = fabs(sample); absvol > mVisualizationChannelVolume[j])
                    {
                        mVisualizationChannelVolume[j] = absvol;
                    }

                    mVisualizationWaveData[i] += sample;
                }
            }
        }
        else
        {
            // Very unlikely failsafe branch
            for (size_t i = 0; i < 256; ++i)
            {
                mVisualizationWaveData[i] = 0;
                for (size_t j = 0; j < mChannels; ++j)
                {
                    const float sample = aBuffer[(i % aSamplesToRead) + aBufferSize * j];

                    if (const float absvol = fabs(sample); absvol > mVisualizationChannelVolume[j])
                    {
                        mVisualizationChannelVolume[j] = absvol;
                    }

                    mVisualizationWaveData[i] += sample;
                }
            }
        }
    }
    return aSamplesToRead;
}

bool BusInstance::hasEnded()
{
    return false;
}

BusInstance::~BusInstance() noexcept
{
    auto* s = mParent->engine;
    for (size_t i = 0; i < s->mHighestVoice; ++i)
    {
        if (s->mVoice[i] && s->mVoice[i]->mBusHandle == mParent->mChannelHandle)
        {
            s->stopVoice_internal(i);
        }
    }
}

Bus::Bus()
{
    channel_count = 2;
}

std::shared_ptr<AudioSourceInstance> Bus::createInstance()
{
    if (mChannelHandle)
    {
        stop();
        mChannelHandle = 0;
    }
    mInstance = std::make_shared<BusInstance>(this);
    return mInstance;
}

void Bus::findBusHandle()
{
    if (mChannelHandle == 0)
    {
        // Find the channel the bus is playing on to calculate handle..
        for (size_t i = 0; mChannelHandle == 0 && i < engine->mHighestVoice; ++i)
        {
            if (engine->mVoice[i].get() == mInstance.get())
            {
                mChannelHandle = engine->getHandleFromVoice_internal(i);
            }
        }
    }
}

handle Bus::play(AudioSource& aSound, float aVolume, float aPan, bool aPaused)
{
    if (!mInstance || !engine)
    {
        return 0;
    }

    findBusHandle();

    if (mChannelHandle == 0)
    {
        return 0;
    }
    return engine->play(aSound, aVolume, aPan, aPaused, mChannelHandle);
}


handle Bus::playClocked(time_t aSoundTime, AudioSource& aSound, float aVolume, float aPan)
{
    if (!mInstance || !engine)
    {
        return 0;
    }

    findBusHandle();

    if (mChannelHandle == 0)
    {
        return 0;
    }

    return engine->play_clocked(aSoundTime, aSound, aVolume, aPan, mChannelHandle);
}

handle Bus::play3d(AudioSource& aSound, Vector3 aPos, Vector3 aVel, float aVolume, bool aPaused)
{
    if (!mInstance || !engine)
    {
        return 0;
    }

    findBusHandle();

    if (mChannelHandle == 0)
    {
        return 0;
    }
    return engine->play3d(aSound, aPos, aVel, aVolume, aPaused, mChannelHandle);
}

handle Bus::play3dClocked(
    time_t       aSoundTime,
    AudioSource& aSound,
    Vector3      aPos,
    Vector3      aVel,
    float        aVolume)
{
    if (!mInstance || !engine)
    {
        return 0;
    }

    findBusHandle();

    if (mChannelHandle == 0)
    {
        return 0;
    }
    return engine->play3dClocked(aSoundTime, aSound, aPos, aVel, aVolume, mChannelHandle);
}

void Bus::annexSound(handle aVoiceHandle)
{
    findBusHandle();
    FOR_ALL_VOICES_PRE_EXT
        engine->mVoice[ch]->mBusHandle = mChannelHandle;
    FOR_ALL_VOICES_POST_EXT
}

void Bus::setFilter(size_t aFilterId, Filter* aFilter)
{
    if (aFilterId >= filters_per_stream)
        return;

    filter[aFilterId] = aFilter;

    if (mInstance)
    {
        engine->lockAudioMutex_internal();
        if (aFilter)
        {
            mInstance->mFilter[aFilterId] = filter[aFilterId]->createInstance();
        }
        engine->unlockAudioMutex_internal();
    }
}

void Bus::setChannels(size_t aChannels)
{
    assert(aChannels != 0 && aChannels != 3 && aChannels != 5 && aChannels != 7);
    assert(aChannels <= max_channels);

    channel_count = aChannels;
}

void Bus::setVisualizationEnable(bool aEnable)
{
    visualization_data = aEnable;
}

float* Bus::calcFFT()
{
    if (mInstance && engine)
    {
        engine->lockAudioMutex_internal();
        auto temp = std::array<float, 1024>{};
        for (int i = 0; i < 256; ++i)
        {
            temp[i * 2]     = mInstance->mVisualizationWaveData[i];
            temp[i * 2 + 1] = 0;
            temp[i + 512]   = 0;
            temp[i + 768]   = 0;
        }
        engine->unlockAudioMutex_internal();

        FFT::fft1024(temp.data());

        for (int i = 0; i < 256; ++i)
        {
            const float real = temp[i * 2];
            const float imag = temp[i * 2 + 1];
            mFFTData[i]      = sqrt(real * real + imag * imag);
        }
    }

    return mFFTData.data();
}

float* Bus::getWave()
{
    if (mInstance && engine)
    {
        engine->lockAudioMutex_internal();
        for (int i       = 0; i < 256; ++i)
            mWaveData[i] = mInstance->mVisualizationWaveData[i];
        engine->unlockAudioMutex_internal();
    }
    return mWaveData.data();
}

float Bus::getApproximateVolume(size_t aChannel)
{
    if (aChannel > channel_count)
    {
        return 0;
    }
    auto vol = 0.0f;
    if (mInstance && engine)
    {
        engine->lockAudioMutex_internal();
        vol = mInstance->mVisualizationChannelVolume[aChannel];
        engine->unlockAudioMutex_internal();
    }
    return vol;
}

size_t Bus::getActiveVoiceCount()
{
    size_t count = 0;
    findBusHandle();
    engine->lockAudioMutex_internal();
    for (size_t i = 0; i < voice_count; ++i)
    {
        if (engine->mVoice[i] && engine->mVoice[i]->mBusHandle == mChannelHandle)
        {
            count++;
        }
    }
    engine->unlockAudioMutex_internal();
    return count;
}

Resampler Bus::getResampler() const
{
    return mResampler;
}

void Bus::setResampler(Resampler aResampler)
{
    mResampler = aResampler;
}
}; // namespace cer
