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

#include "audio/AudioDevice.hpp"
#include "soloud_internal.hpp"
#include <stdexcept>

#include <AudioToolbox/AudioToolbox.h>

static constexpr size_t s_buffer_count = 2;

static AudioQueueRef s_audio_queue = nullptr;

static void soloud_coreaudio_deinit(cer::AudioDevice* /*engine*/)
{
    AudioQueueStop(s_audio_queue, 1u);
    AudioQueueDispose(s_audio_queue, 0u);
}

static auto soloud_coreaudio_pause(cer::AudioDevice* /*engine*/) -> bool
{
    if (s_audio_queue == nullptr)
    {
        return false;
    }

    AudioQueuePause(s_audio_queue); // TODO: Error code

    return true;
}

static auto soloud_coreaudio_resume(cer::AudioDevice* /*engine*/) -> bool
{
    if (s_audio_queue == nullptr)
    {
        return false;
    }

    AudioQueueStart(s_audio_queue, nil); // TODO: Error code

    return true;
}

static void coreaudio_fill_buffer(void* context, AudioQueueRef queue, AudioQueueBufferRef buffer)
{
    const auto engine = static_cast<cer::AudioDevice*>(context);
    engine->mixSigned16(static_cast<short*>(buffer->mAudioData), buffer->mAudioDataByteSize / 4);

    AudioQueueEnqueueBuffer(queue, buffer, 0, nullptr);
}

void cer::coreaudio_init(const AudioBackendArgs& args)
{
    auto* engine = args.engine;

    engine->postinit_internal(args.sample_rate, args.buffer, args.flags, 2);
    engine->m_backend_cleanup_func = soloud_coreaudio_deinit;
    engine->m_backend_pause_func   = soloud_coreaudio_pause;
    engine->m_backend_resume_func  = soloud_coreaudio_resume;

    AudioStreamBasicDescription audio_format;
    audio_format.mSampleRate       = Float64(args.sample_rate);
    audio_format.mFormatID         = kAudioFormatLinearPCM;
    audio_format.mFormatFlags      = kLinearPCMFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
    audio_format.mBytesPerPacket   = 4;
    audio_format.mFramesPerPacket  = 1;
    audio_format.mBytesPerFrame    = 4;
    audio_format.mChannelsPerFrame = 2;
    audio_format.mBitsPerChannel   = 16;
    audio_format.mReserved         = 0;

    // create the audio queue
    {
        const auto result = AudioQueueNewOutput(&audio_format,
                                                coreaudio_fill_buffer,
                                                engine,
                                                nullptr,
                                                nullptr,
                                                0,
                                                &s_audio_queue);
        if (result != 0)
        {
            throw std::runtime_error{"AudioQueueNewOutput failed"};
        }
    }

    // allocate and prime audio buffers
    for (size_t i = 0; i < s_buffer_count; ++i)
    {
        AudioQueueBufferRef buffer = nullptr;

        if (const auto result = AudioQueueAllocateBuffer(s_audio_queue, args.buffer * 4, &buffer);
            result != 0)
        {
            throw std::runtime_error{"AudioQueueAllocateBuffer failed"};
        }

        buffer->mAudioDataByteSize = args.buffer * 4;
        memset(buffer->mAudioData, 0, buffer->mAudioDataByteSize);
        AudioQueueEnqueueBuffer(s_audio_queue, buffer, 0, nullptr);
    }

    // start playback
    if (const auto result = AudioQueueStart(s_audio_queue, nullptr); result != 0)
    {
        throw std::runtime_error{"AudioQueueStart failed"};
    }
}
