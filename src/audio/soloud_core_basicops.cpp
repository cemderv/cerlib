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

#include "soloud_internal.hpp"

// Core "basic" operations - play, stop, etc

namespace cer
{
handle Engine::play(AudioSource& aSound, float aVolume, float aPan, bool aPaused, size_t aBus)
{
    if (aSound.single_instance)
    {
        // Only one instance allowed, stop others
        aSound.stop();
    }

    // Creation of an audio instance may take significant amount of time,
    // so let's not do it inside the audio thread mutex.
    aSound.engine = this;
    auto instance = aSound.createInstance();

    lockAudioMutex_internal();
    int ch = findFreeVoice_internal();
    if (ch < 0)
    {
        unlockAudioMutex_internal();
        return 7; // TODO: this was "UNKNOWN_ERROR"
    }
    if (!aSound.audio_source_id)
    {
        aSound.audio_source_id = mAudioSourceID;
        mAudioSourceID++;
    }
    mVoice[ch]                 = instance;
    mVoice[ch]->mAudioSourceID = aSound.audio_source_id;
    mVoice[ch]->mBusHandle     = aBus;
    mVoice[ch]->init(aSound, mPlayIndex);
    m3dData[ch] = AudioSourceInstance3dData{aSound};

    mPlayIndex++;

    // 20 bits, skip the last one (top bits full = voice group)
    if (mPlayIndex == 0xfffff)
    {
        mPlayIndex = 0;
    }

    if (aPaused)
    {
        mVoice[ch]->mFlags.Paused = true;
    }

    setVoicePan_internal(ch, aPan);
    if (aVolume < 0)
    {
        setVoiceVolume_internal(ch, aSound.volume);
    }
    else
    {
        setVoiceVolume_internal(ch, aVolume);
    }

    // Fix initial voice volume ramp up
    for (size_t i = 0; i < MAX_CHANNELS; ++i)
    {
        mVoice[ch]->mCurrentChannelVolume[i] =
            mVoice[ch]->mChannelVolume[i] * mVoice[ch]->mOverallVolume;
    }

    setVoiceRelativePlaySpeed_internal(ch, 1);

    for (size_t i = 0; i < FILTERS_PER_STREAM; ++i)
    {
        if (aSound.filter[i])
        {
            mVoice[ch]->mFilter[i] = aSound.filter[i]->createInstance();
        }
    }

    mActiveVoiceDirty = true;

    unlockAudioMutex_internal();

    return getHandleFromVoice_internal(ch);
}

handle Engine::playClocked(
    time_t       aSoundTime,
    AudioSource& aSound,
    float        aVolume,
    float        aPan,
    size_t       aBus)
{
    const handle h = play(aSound, aVolume, aPan, 1, aBus);
    lockAudioMutex_internal();
    // mLastClockedTime is cleared to zero at start of every output buffer
    time_t lasttime = mLastClockedTime;
    if (lasttime == 0)
    {
        mLastClockedTime = aSoundTime;
        lasttime         = aSoundTime;
    }
    unlockAudioMutex_internal();
    int samples = (int)floor((aSoundTime - lasttime) * mSamplerate);
    // Make sure we don't delay too much (or overflow)
    if (samples < 0 || samples > 2048)
        samples = 0;
    setDelaySamples(h, samples);
    setPause(h, false);
    return h;
}

handle Engine::playBackground(AudioSource& aSound, float aVolume, bool aPaused, size_t aBus)
{
    const handle h = play(aSound, aVolume, 0.0f, aPaused, aBus);
    setPanAbsolute(h, 1.0f, 1.0f);
    return h;
}

bool Engine::seek(handle aVoiceHandle, time_t aSeconds)
{
    bool res = true;
    FOR_ALL_VOICES_PRE
        const auto singleres = mVoice[ch]->seek(aSeconds, mScratch.mData, mScratchSize);
        if (!singleres)
            res = singleres;
    FOR_ALL_VOICES_POST
    return res;
}


void Engine::stop(handle aVoiceHandle)
{
    FOR_ALL_VOICES_PRE
        stopVoice_internal(ch);
    FOR_ALL_VOICES_POST
}

void Engine::stopAudioSource(AudioSource& aSound)
{
    if (aSound.audio_source_id)
    {
        lockAudioMutex_internal();

        for (size_t i = 0; i < mHighestVoice; ++i)
        {
            if (mVoice[i] && mVoice[i]->mAudioSourceID == aSound.audio_source_id)
            {
                stopVoice_internal(i);
            }
        }
        unlockAudioMutex_internal();
    }
}

void Engine::stopAll()
{
    lockAudioMutex_internal();
    for (size_t i = 0; i < mHighestVoice; ++i)
    {
        stopVoice_internal(i);
    }
    unlockAudioMutex_internal();
}

int Engine::countAudioSource(AudioSource& aSound)
{
    int count = 0;
    if (aSound.audio_source_id)
    {
        lockAudioMutex_internal();

        for (size_t i = 0; i < mHighestVoice; ++i)
        {
            if (mVoice[i] && mVoice[i]->mAudioSourceID == aSound.audio_source_id)
            {
                count++;
            }
        }
        unlockAudioMutex_internal();
    }
    return count;
}

} // namespace cer
