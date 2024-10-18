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

#include "audio/soloud_engine.hpp"

// Getters - return information about SoLoud state

namespace cer
{
float Engine::getPostClipScaler() const
{
    return mPostClipScaler;
}

Resampler Engine::getMainResampler() const
{
    return mResampler;
}

float Engine::getGlobalVolume() const
{
    return mGlobalVolume;
}

handle Engine::getHandleFromVoice_internal(size_t aVoice) const
{
    if (mVoice[aVoice] == nullptr)
    {
        return 0;
    }

    return (aVoice + 1) | (mVoice[aVoice]->mPlayIndex << 12);
}

int Engine::getVoiceFromHandle_internal(handle aVoiceHandle) const
{
    // If this is a voice group handle, pick the first handle from the group
    if (const auto* h = voiceGroupHandleToArray_internal(aVoiceHandle); h != nullptr)
    {
        aVoiceHandle = *h;
    }

    if (aVoiceHandle == 0)
    {
        return -1;
    }

    const int    ch  = (aVoiceHandle & 0xfff) - 1;
    const size_t idx = aVoiceHandle >> 12;

    if (mVoice[ch] != nullptr && (mVoice[ch]->mPlayIndex & 0xfffff) == idx)
    {
        return ch;
    }

    return -1;
}

size_t Engine::getMaxActiveVoiceCount() const
{
    return mMaxActiveVoices;
}

size_t Engine::getActiveVoiceCount()
{
    lockAudioMutex_internal();
    if (mActiveVoiceDirty)
    {
        calcActiveVoices_internal();
    }
    const size_t c = mActiveVoiceCount;
    unlockAudioMutex_internal();

    return c;
}

size_t Engine::getVoiceCount()
{
    lockAudioMutex_internal();
    int c = 0;
    for (size_t i = 0; i < mHighestVoice; ++i)
    {
        if (mVoice[i])
        {
            ++c;
        }
    }
    unlockAudioMutex_internal();
    return c;
}

bool Engine::isValidVoiceHandle(handle aVoiceHandle)
{
    // voice groups are not valid voice handles
    if ((aVoiceHandle & 0xfffff000) == 0xfffff000)
    {
        return false;
    }

    lockAudioMutex_internal();
    if (getVoiceFromHandle_internal(aVoiceHandle) != -1)
    {
        unlockAudioMutex_internal();
        return true;
    }
    unlockAudioMutex_internal();
    return false;
}


time_t Engine::getLoopPoint(handle aVoiceHandle)
{
    lockAudioMutex_internal();
    const int ch = getVoiceFromHandle_internal(aVoiceHandle);
    if (ch == -1)
    {
        unlockAudioMutex_internal();
        return 0;
    }
    const time_t v = mVoice[ch]->mLoopPoint;
    unlockAudioMutex_internal();
    return v;
}

bool Engine::getLooping(handle aVoiceHandle)
{
    lockAudioMutex_internal();
    const int ch = getVoiceFromHandle_internal(aVoiceHandle);
    if (ch == -1)
    {
        unlockAudioMutex_internal();
        return 0;
    }
    const bool v = mVoice[ch]->mFlags.Looping;
    unlockAudioMutex_internal();
    return v;
}

bool Engine::getAutoStop(handle aVoiceHandle)
{
    lockAudioMutex_internal();
    const int ch = getVoiceFromHandle_internal(aVoiceHandle);
    if (ch == -1)
    {
        unlockAudioMutex_internal();
        return false;
    }
    const auto v = mVoice[ch]->mFlags.DisableAutostop;
    unlockAudioMutex_internal();
    return !v;
}

float Engine::getInfo(handle aVoiceHandle, size_t mInfoKey)
{
    lockAudioMutex_internal();
    const int ch = getVoiceFromHandle_internal(aVoiceHandle);
    if (ch == -1)
    {
        unlockAudioMutex_internal();
        return 0;
    }
    const float v = mVoice[ch]->getInfo(mInfoKey);
    unlockAudioMutex_internal();
    return v;
}

float Engine::getVolume(handle aVoiceHandle)
{
    lockAudioMutex_internal();
    const int ch = getVoiceFromHandle_internal(aVoiceHandle);
    if (ch == -1)
    {
        unlockAudioMutex_internal();
        return 0;
    }
    const float v = mVoice[ch]->mSetVolume;
    unlockAudioMutex_internal();
    return v;
}

float Engine::getOverallVolume(handle aVoiceHandle)
{
    lockAudioMutex_internal();
    const int ch = getVoiceFromHandle_internal(aVoiceHandle);
    if (ch == -1)
    {
        unlockAudioMutex_internal();
        return 0;
    }
    const float v = mVoice[ch]->mOverallVolume;
    unlockAudioMutex_internal();
    return v;
}

float Engine::getPan(handle aVoiceHandle)
{
    lockAudioMutex_internal();
    const int ch = getVoiceFromHandle_internal(aVoiceHandle);
    if (ch == -1)
    {
        unlockAudioMutex_internal();
        return 0;
    }
    const float v = mVoice[ch]->mPan;
    unlockAudioMutex_internal();
    return v;
}

time_t Engine::getStreamTime(handle aVoiceHandle)
{
    lockAudioMutex_internal();
    const int ch = getVoiceFromHandle_internal(aVoiceHandle);
    if (ch == -1)
    {
        unlockAudioMutex_internal();
        return 0;
    }
    const double v = mVoice[ch]->mStreamTime;
    unlockAudioMutex_internal();
    return v;
}

time_t Engine::getStreamPosition(handle aVoiceHandle)
{
    lockAudioMutex_internal();
    const int ch = getVoiceFromHandle_internal(aVoiceHandle);
    if (ch == -1)
    {
        unlockAudioMutex_internal();
        return 0;
    }
    const double v = mVoice[ch]->mStreamPosition;
    unlockAudioMutex_internal();
    return v;
}

float Engine::getRelativePlaySpeed(handle aVoiceHandle)
{
    lockAudioMutex_internal();
    const int ch = getVoiceFromHandle_internal(aVoiceHandle);
    if (ch == -1)
    {
        unlockAudioMutex_internal();
        return 1;
    }
    const float v = mVoice[ch]->mSetRelativePlaySpeed;
    unlockAudioMutex_internal();
    return v;
}

float Engine::getSamplerate(handle aVoiceHandle)
{
    lockAudioMutex_internal();
    const int ch = getVoiceFromHandle_internal(aVoiceHandle);
    if (ch == -1)
    {
        unlockAudioMutex_internal();
        return 0;
    }
    const float v = mVoice[ch]->mBaseSamplerate;
    unlockAudioMutex_internal();
    return v;
}

bool Engine::getPause(handle aVoiceHandle)
{
    lockAudioMutex_internal();
    const int ch = getVoiceFromHandle_internal(aVoiceHandle);
    if (ch == -1)
    {
        unlockAudioMutex_internal();
        return false;
    }
    const auto v = mVoice[ch]->mFlags.Paused;
    unlockAudioMutex_internal();
    return v;
}

bool Engine::getProtectVoice(handle aVoiceHandle)
{
    lockAudioMutex_internal();
    const int ch = getVoiceFromHandle_internal(aVoiceHandle);
    if (ch == -1)
    {
        unlockAudioMutex_internal();
        return false;
    }
    const auto v = mVoice[ch]->mFlags.Protected;
    unlockAudioMutex_internal();
    return v;
}

int Engine::findFreeVoice_internal()
{
    size_t lowest_play_index_value = 0xffffffff;
    int    lowest_play_index       = -1;

    // (slowly) drag the highest active voice index down
    if (mHighestVoice > 0 && mVoice[mHighestVoice - 1] == nullptr)
    {
        mHighestVoice--;
    }

    for (size_t i = 0; i < VOICE_COUNT; ++i)
    {
        if (mVoice[i] == nullptr)
        {
            if (i + 1 > mHighestVoice)
            {
                mHighestVoice = i + 1;
            }
            return i;
        }

        if (!mVoice[i]->mFlags.Protected && mVoice[i]->mPlayIndex < lowest_play_index_value)
        {
            lowest_play_index_value = mVoice[i]->mPlayIndex;
            lowest_play_index       = i;
        }
    }
    stopVoice_internal(lowest_play_index);
    return lowest_play_index;
}

size_t Engine::getLoopCount(handle aVoiceHandle)
{
    lockAudioMutex_internal();
    const int ch = getVoiceFromHandle_internal(aVoiceHandle);
    if (ch == -1)
    {
        unlockAudioMutex_internal();
        return 0;
    }
    const int v = mVoice[ch]->mLoopCount;
    unlockAudioMutex_internal();
    return v;
}

// Returns current backend channel count (1 mono, 2 stereo, etc)
size_t Engine::getBackendChannels() const
{
    return mChannels;
}

// Returns current backend sample rate
size_t Engine::getBackendSamplerate() const
{
    return mSamplerate;
}

// Returns current backend buffer size
size_t Engine::getBackendBufferSize() const
{
    return mBufferSize;
}

// Get speaker position in 3d space
Vector3 Engine::getSpeakerPosition(size_t aChannel) const
{
    return m3dSpeakerPosition.at(aChannel);
}
} // namespace cer
