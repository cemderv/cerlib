/*
SoLoud audio engine
Copyright (c) 2013-2018 Jari Komppa

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

#include "soloud_queue.hpp"
#include "soloud_engine.hpp"

namespace cer
{
QueueInstance::QueueInstance(Queue* aParent)
{
    mParent          = aParent;
    mFlags.Protected = true;
}

size_t QueueInstance::getAudio(float* aBuffer, size_t aSamplesToRead, size_t aBufferSize)
{
    if (mParent->mCount == 0)
    {
        return 0;
    }
    size_t copycount = aSamplesToRead;
    size_t copyofs   = 0;
    while (copycount && mParent->mCount)
    {
        int readcount = mParent->mSource[mParent->mReadIndex]->getAudio(aBuffer + copyofs,
                                                                        copycount,
                                                                        aBufferSize);
        copyofs += readcount;
        copycount -= readcount;
        if (mParent->mSource[mParent->mReadIndex]->hasEnded())
        {
            mParent->mSource[mParent->mReadIndex].reset();
            mParent->mReadIndex = (mParent->mReadIndex + 1) % SOLOUD_QUEUE_MAX;
            mParent->mCount--;
            mLoopCount++;
        }
    }
    return copyofs;
}

bool QueueInstance::hasEnded()
{
    return mLoopCount != 0 && mParent->mCount == 0;
}

std::shared_ptr<AudioSourceInstance> Queue::createInstance()
{
    if (mInstance)
    {
        stop();
    }

    mInstance = std::make_shared<QueueInstance>(this);
    return mInstance;
}

void Queue::findQueueHandle()
{
    // Find the channel the queue is playing on to calculate handle..
    for (int i = 0; mQueueHandle == 0 && i < (signed)engine->mHighestVoice; ++i)
    {
        if (engine->mVoice[i] == mInstance)
        {
            mQueueHandle = engine->getHandleFromVoice_internal(i);
        }
    }
}

void Queue::play(AudioSource& aSound)
{
    assert(engine != nullptr);

    findQueueHandle();

    assert(mQueueHandle != 0);
    assert(mCount < SOLOUD_QUEUE_MAX);

    if (!aSound.audio_source_id)
    {
        aSound.audio_source_id = engine->mAudioSourceID;
        engine->mAudioSourceID++;
    }

    auto instance = aSound.createInstance();

    instance->init(aSound, 0);
    instance->mAudioSourceID = aSound.audio_source_id;

    engine->lockAudioMutex_internal();
    mSource[mWriteIndex] = std::move(instance);
    mWriteIndex          = (mWriteIndex + 1) % SOLOUD_QUEUE_MAX;
    mCount++;
    engine->unlockAudioMutex_internal();
}


size_t Queue::getQueueCount() const
{
    if (!engine)
    {
        return 0;
    }

    engine->lockAudioMutex_internal();
    size_t count = mCount;
    engine->unlockAudioMutex_internal();
    return count;
}

bool Queue::isCurrentlyPlaying(const AudioSource& aSound) const
{
    if (engine == nullptr || mCount == 0 || aSound.audio_source_id == 0)
    {
        return false;
    }

    engine->lockAudioMutex_internal();
    const bool res = mSource[mReadIndex]->mAudioSourceID == aSound.audio_source_id;
    engine->unlockAudioMutex_internal();
    return res;
}

void Queue::setParamsFromAudioSource(const AudioSource& aSound)
{
    channel_count    = aSound.channel_count;
    base_sample_rate = aSound.base_sample_rate;
}

void Queue::setParams(float aSamplerate, size_t aChannels)
{
    assert(aChannels >= 1);
    assert(aChannels <= MAX_CHANNELS);

    channel_count    = aChannels;
    base_sample_rate = aSamplerate;
}
} // namespace cer
