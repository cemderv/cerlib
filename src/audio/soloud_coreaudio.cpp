/*
SoLoud audio engine
Copyright (c) 2015 Jari Komppa

Core Audio backend for Mac OS X
Copyright (c) 2015 Petri Häkkinen

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

#include "soloud_engine.hpp"
#include <stdexcept>

#include <AudioToolbox/AudioToolbox.h>

#define NUM_BUFFERS 2

static AudioQueueRef audioQueue = 0;

namespace cer
{
void soloud_coreaudio_deinit(Engine* engine)
{
    AudioQueueStop(audioQueue, true);
    AudioQueueDispose(audioQueue, false);
}

bool soloud_coreaudio_pause(Engine* engine)
{
    if (!audioQueue)
        return false;

    AudioQueuePause(audioQueue); // TODO: Error code

    return true;
}

bool soloud_coreaudio_resume(Engine* engine)
{
    if (!audioQueue)
        return false;

    AudioQueueStart(audioQueue, nil); // TODO: Error code

    return true;
}

static void coreaudio_fill_buffer(void* context, AudioQueueRef queue, AudioQueueBufferRef buffer)
{
    auto engine = static_cast<Engine*>(context);
    engine->mixSigned16(static_cast<short*>(buffer->mAudioData), buffer->mAudioDataByteSize / 4);
    AudioQueueEnqueueBuffer(queue, buffer, 0, nullptr);
}

void coreaudio_init(
    Engine* engine, EngineFlags aFlags, size_t aSamplerate, size_t aBuffer, size_t aChannels)
{
    engine->postinit_internal(aSamplerate, aBuffer, aFlags, 2);
    engine->mBackendCleanupFunc = soloud_coreaudio_deinit;
    engine->mBackendPauseFunc   = soloud_coreaudio_pause;
    engine->mBackendResumeFunc  = soloud_coreaudio_resume;

    AudioStreamBasicDescription audioFormat;
    audioFormat.mSampleRate       = aSamplerate;
    audioFormat.mFormatID         = kAudioFormatLinearPCM;
    audioFormat.mFormatFlags      = kLinearPCMFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
    audioFormat.mBytesPerPacket   = 4;
    audioFormat.mFramesPerPacket  = 1;
    audioFormat.mBytesPerFrame    = 4;
    audioFormat.mChannelsPerFrame = 2;
    audioFormat.mBitsPerChannel   = 16;
    audioFormat.mReserved         = 0;

    // create the audio queue
    OSStatus result = AudioQueueNewOutput(&audioFormat,
                                          coreaudio_fill_buffer,
                                          engine,
                                          nullptr,
                                          nullptr,
                                          0,
                                          &audioQueue);
    if (result)
    {
        throw std::runtime_error{"AudioQueueNewOutput failed"};
    }

    // allocate and prime audio buffers
    for (int i = 0; i < NUM_BUFFERS; ++i)
    {
        AudioQueueBufferRef buffer;
        result = AudioQueueAllocateBuffer(audioQueue, aBuffer * 4, &buffer);
        if (result)
        {
            throw std::runtime_error{"AudioQueueAllocateBuffer failed"};
        }
        buffer->mAudioDataByteSize = aBuffer * 4;
        memset(buffer->mAudioData, 0, buffer->mAudioDataByteSize);
        AudioQueueEnqueueBuffer(audioQueue, buffer, 0, nullptr);
    }

    // start playback
    result = AudioQueueStart(audioQueue, nullptr);
    if (result)
    {
        throw std::runtime_error{"AudioQueueStart failed"};
    }
}
}; // namespace cer
