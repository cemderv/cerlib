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

#include "audio/AudioDevice.hpp"
#include "audio/Thread.hpp"
#include "audio/soloud_internal.hpp"
#include "cerlib/Logging.hpp"

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

#include "SLES/OpenSLES.h"
#include "SLES/OpenSLES_Platform.h"

#if defined(__ANDROID__)
#include "SLES/OpenSLES_Android.h"
#endif

#ifdef __clang__
#pragma GCC diagnostic ignored "-Wdeprecated-volatile"
#endif

namespace cer
{
static constexpr auto NUM_BUFFERS = size_t(2);

struct BackendData
{
    BackendData()
    {
        memset(this, 0, sizeof(BackendData));
    }

    ~BackendData()
    {
        // Wait until thread is done.
        threadrun++;
        while (threadrun == 1)
        {
            thread::sleep(10);
        }

        if (playerObj)
        {
            (*playerObj)->Destroy(playerObj);
        }

        if (outputMixObj)
        {
            (*outputMixObj)->Destroy(outputMixObj);
        }

        if (engineObj)
        {
            (*engineObj)->Destroy(engineObj);
        }

        for (size_t idx = 0; idx < NUM_BUFFERS; ++idx)
        {
            delete[] outputBuffers[idx];
        }
    }

    // Engine.
    SLObjectItf engineObj;
    SLEngineItf engine;

    // Output mix.
    SLObjectItf outputMixObj;
    SLVolumeItf outputMixVol;

    // Data.
    SLDataLocator_OutputMix outLocator;
    SLDataSink              dstDataSink;

    // Player.
    SLObjectItf                   playerObj;
    SLPlayItf                     player;
    SLVolumeItf                   playerVol;
    SLAndroidSimpleBufferQueueItf playerBufferQueue;

    size_t       bufferSize;
    size_t       channels;
    short*       outputBuffers[NUM_BUFFERS];
    int          buffersQueued;
    int          activeBuffer;
    volatile int threadrun;

    SLDataLocator_AndroidSimpleBufferQueue inLocator;
};

void soloud_opensles_deinit(AudioDevice* engine)
{
    BackendData* data = static_cast<BackendData*>(engine->m_backend_data);
    delete data;
    engine->m_backend_data = nullptr;
}

static void opensles_iterate(AudioDevice* engine)
{
    BackendData* data = static_cast<BackendData*>(engine->m_backend_data);

    // If we have no buffered queued, queue one up for playback.
    if (data->buffersQueued == 0)
    {
        // Get next output buffer, advance, next buffer.
        short* outputBuffer = data->outputBuffers[data->activeBuffer];
        data->activeBuffer  = (data->activeBuffer + 1) % NUM_BUFFERS;
        short* nextBuffer   = data->outputBuffers[data->activeBuffer];

        // Mix this buffer.
        const int bufferSizeBytes = data->bufferSize * data->channels * sizeof(short);
        (*data->playerBufferQueue)->Enqueue(data->playerBufferQueue, outputBuffer, bufferSizeBytes);
        ++data->buffersQueued;

        engine->mixSigned16(nextBuffer, data->bufferSize);
    }
}

static void opensles_thread(void* aParam)
{
    AudioDevice* soloud = static_cast<AudioDevice*>(aParam);
    BackendData* data   = static_cast<BackendData*>(soloud->m_backend_data);
    while (data->threadrun == 0)
    {
        opensles_iterate(soloud);

        // TODO: Condition var?
        thread::sleep(1);
    }
    data->threadrun++;
}

static void SLAPIENTRY soloud_opensles_play_callback(SLPlayItf player,
                                                     void*     context,
                                                     SLuint32  event)
{
    AudioDevice* soloud = static_cast<AudioDevice*>(context);
    BackendData* data   = static_cast<BackendData*>(soloud->m_backend_data);
    if (event & SL_PLAYEVENT_HEADATEND && data->buffersQueued > 0)
    {
        data->buffersQueued--;
    }
}
} // namespace cer

void cer::opensles_init(const AudioBackendArgs& args)
{
    auto* engine = args.engine;

    auto* data = new BackendData();

    // Allocate output buffer to mix into.
    data->bufferSize               = args.buffer;
    data->channels                 = args.channel_count;
    const int buffer_size_in_bytes = data->bufferSize * data->channels * sizeof(short);
    for (size_t idx = 0; idx < NUM_BUFFERS; ++idx)
    {
        data->outputBuffers[idx] = new short[data->bufferSize * data->channels];
        memset(data->outputBuffers[idx], 0, buffer_size_in_bytes);
    }

    // Create engine.
    const SLEngineOption opts[] = {
        {(SLuint32)SL_ENGINEOPTION_THREADSAFE, (SLuint32)SL_BOOLEAN_TRUE},
    };

    if (slCreateEngine(&data->engineObj, 1, opts, 0, nullptr, nullptr) != SL_RESULT_SUCCESS)
    {
        throw std::runtime_error{"Failed to create OpenSLES audio engine."};
    }

    // Realize and get engine interface.
    (*data->engineObj)->Realize(data->engineObj, SL_BOOLEAN_FALSE);
    if ((*data->engineObj)->GetInterface(data->engineObj, SL_IID_ENGINE, &data->engine) !=
        SL_RESULT_SUCCESS)
    {
        throw std::runtime_error{"Failed to obtain OpenSLES audio engine interface."};
    }

    // Create output mix.
    const SLInterfaceID ids[] = {SL_IID_VOLUME};
    const SLboolean     req[] = {SL_BOOLEAN_FALSE};

    if ((*data->engine)->CreateOutputMix(data->engine, &data->outputMixObj, 1, ids, req) !=
        SL_RESULT_SUCCESS)
    {
        throw std::runtime_error{"Failed to create OpenSLES output mix object."};
    }
    (*data->outputMixObj)->Realize(data->outputMixObj, SL_BOOLEAN_FALSE);

    if ((*data->outputMixObj)
            ->GetInterface(data->outputMixObj, SL_IID_VOLUME, &data->outputMixVol) !=
        SL_RESULT_SUCCESS)
    {
        log_info("Failed to get OpenSLES output mix volume interface");
    }

    // Create android buffer queue.
    data->inLocator.locatorType = SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE;
    data->inLocator.numBuffers  = NUM_BUFFERS;

    // Setup data format.
    SLDataFormat_PCM format;
    format.formatType    = SL_DATAFORMAT_PCM;
    format.numChannels   = data->channels;
    format.samplesPerSec = args.sample_rate * 1000; // mHz
    format.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;
    format.containerSize = 16;
    format.endianness    = SL_BYTEORDER_LITTLEENDIAN;

    // Determine channel mask.
    if (data->channels == 2)
    {
        format.channelMask = SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
    }
    else
    {
        format.channelMask = SL_SPEAKER_FRONT_CENTER;
    }

    SLDataSource src;
    src.pLocator = &data->inLocator;
    src.pFormat  = &format;

    // Output mix.
    data->outLocator.locatorType = SL_DATALOCATOR_OUTPUTMIX;
    data->outLocator.outputMix   = data->outputMixObj;

    data->dstDataSink.pLocator = &data->outLocator;
    data->dstDataSink.pFormat  = nullptr;

    // Setup player.
    {
        const SLInterfaceID ids[] = {SL_IID_VOLUME, SL_IID_ANDROIDSIMPLEBUFFERQUEUE};
        const SLboolean     req[] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};

        (*data->engine)
            ->CreateAudioPlayer(data->engine,
                                &data->playerObj,
                                &src,
                                &data->dstDataSink,
                                sizeof(ids) / sizeof(ids[0]),
                                ids,
                                req);

        (*data->playerObj)->Realize(data->playerObj, SL_BOOLEAN_FALSE);

        (*data->playerObj)->GetInterface(data->playerObj, SL_IID_PLAY, &data->player);
        (*data->playerObj)->GetInterface(data->playerObj, SL_IID_VOLUME, &data->playerVol);

        (*data->playerObj)
            ->GetInterface(data->playerObj,
                           SL_IID_ANDROIDSIMPLEBUFFERQUEUE,
                           &data->playerBufferQueue);
    }

    engine->m_backend_data = data; // Must be set before callback

    // Register callback
    (*data->player)->RegisterCallback(data->player, soloud_opensles_play_callback, engine);
    (*data->player)->SetCallbackEventsMask(data->player, SL_PLAYEVENT_HEADATEND);
    (*data->player)->SetPlayState(data->player, SL_PLAYSTATE_PLAYING);

    //
    engine->postinit_internal(args.sample_rate, data->bufferSize, args.flags, 2);
    engine->m_backend_cleanup_func = soloud_opensles_deinit;

    log_info("Creating OpenSLES audio thread");

    thread::create_thread(opensles_thread, engine);
}
