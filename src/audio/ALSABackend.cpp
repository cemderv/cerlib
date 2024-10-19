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

#define ALSA_PCM_NEW_HW_PARAMS_API
#include <alsa/asoundlib.h>

#include <fcntl.h>
#include <string.h>
#include <unistd.h>

namespace cer
{
struct ALSAData
{
    float*               buffer;
    short*               sampleBuffer;
    snd_pcm_t*           alsaDeviceHandle;
    AudioDevice*         soloud;
    int                  samples;
    int                  channels;
    bool                 audioProcessingDone;
    thread::ThreadHandle threadHandle;
};


static void alsaThread(void* aParam)
{

    ALSAData* data = static_cast<ALSAData*>(aParam);
    while (!data->audioProcessingDone)
    {
        data->soloud->mix(data->buffer, data->samples);
        for (int i = 0; i < data->samples * data->channels; ++i)
        {
            data->sampleBuffer[i] =
                static_cast<short>(floor(data->buffer[i] * static_cast<float>(0x7fff)));
        }
        if (snd_pcm_writei(data->alsaDeviceHandle, data->sampleBuffer, data->samples) == -EPIPE)
            snd_pcm_prepare(data->alsaDeviceHandle);
    }
}

static void alsaCleanup(AudioDevice* engine)
{
    if (0 == engine->m_backend_data)
    {
        return;
    }
    ALSAData* data            = static_cast<ALSAData*>(engine->m_backend_data);
    data->audioProcessingDone = true;
    if (data->threadHandle)
    {
        thread::wait(data->threadHandle);
        thread::release(data->threadHandle);
    }
    snd_pcm_drain(data->alsaDeviceHandle);
    snd_pcm_close(data->alsaDeviceHandle);
    if (0 != data->sampleBuffer)
    {
        delete[] data->sampleBuffer;
    }
    if (0 != data->buffer)
    {
        delete[] data->buffer;
    }
    delete data;
    engine->m_backend_data = 0;
}
} // namespace cer

void cer::alsa_init(const AudioBackendArgs& args)
{
    auto* engine = args.engine;

    ALSAData* data = new ALSAData;
    memset(data, 0, sizeof(ALSAData));
    engine->m_backend_data         = data;
    engine->m_backend_cleanup_func = alsaCleanup;
    data->samples                  = args.buffer;
    data->channels                 = 2;
    data->soloud                   = engine;

    int        rc;
    snd_pcm_t* handle;
    rc = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
    if (rc < 0)
    {
        throw std::runtime_error{"Failed to initialize the audio device"};
    }

    data->alsaDeviceHandle = handle;

    snd_pcm_hw_params_t* params;
    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(handle, params);

    snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(handle, params, 2);
    snd_pcm_hw_params_set_buffer_size(handle, params, args.buffer);

    auto val = static_cast<unsigned int>(args.sample_rate);
    int  dir = 0;
    rc       = snd_pcm_hw_params_set_rate_near(handle, params, &val, &dir);
    if (rc < 0)
    {
        throw std::runtime_error{"Failed to initialize the audio device"};
    }

    rc = snd_pcm_hw_params(handle, params);
    if (rc < 0)
    {
        throw std::runtime_error{"Failed to initialize the audio device"};
    }

    snd_pcm_hw_params_get_rate(params, &val, &dir);
    snd_pcm_hw_params_get_channels(params, &val);
    data->channels = val;

    data->buffer       = new float[data->samples * data->channels];
    data->sampleBuffer = new short[data->samples * data->channels];
    engine->postinit_internal(val, data->samples * data->channels, args.flags, 2);
    data->threadHandle = thread::create_thread(alsaThread, data);

    if (0 == data->threadHandle)
    {
        throw std::runtime_error{"Failed to initialize the audio device"};
    }
}
