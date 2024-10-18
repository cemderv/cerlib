/*
SoLoud audio engine
Copyright (c) 2013-2014 Jari Komppa

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
#include "audio/Misc.hpp"
#include "audio/Thread.hpp"
#include "audio/soloud_internal.hpp"

#include <windows.h>

#include <mmsystem.h>

#include <array>

#ifdef _MSC_VER
#pragma comment(lib, "winmm.lib")
#endif

namespace cer
{
static constexpr auto BUFFER_COUNT = size_t(2);

struct SoLoudWinMMData
{
    AlignedFloatBuffer                buffer;
    std::array<short*, BUFFER_COUNT>  sample_buffer               = {};
    std::array<WAVEHDR, BUFFER_COUNT> header                      = {};
    HWAVEOUT                          wave_out                    = nullptr;
    HANDLE                            buffer_end_event            = nullptr;
    HANDLE                            audio_processing_done_event = nullptr;
    AudioDevice*                      device                      = nullptr;
    size_t                            samples                     = 0;
    thread::ThreadHandle              thread_handle               = {};
};

static void winMMThread(LPVOID aParam)
{
    SoLoudWinMMData* data = static_cast<SoLoudWinMMData*>(aParam);
    while (WAIT_OBJECT_0 != WaitForSingleObject(data->audio_processing_done_event, 0))
    {
        for (int i = 0; i < BUFFER_COUNT; ++i)
        {
            if (0 != (data->header[i].dwFlags & WHDR_INQUEUE))
            {
                continue;
            }
            short* tgtBuf = data->sample_buffer[i];

            data->device->mixSigned16(tgtBuf, data->samples);

            if (MMSYSERR_NOERROR != waveOutWrite(data->wave_out, &data->header[i], sizeof(WAVEHDR)))
            {
                return;
            }
        }
        WaitForSingleObject(data->buffer_end_event, INFINITE);
    }
}

static void winMMCleanup(AudioDevice* engine)
{
    if (0 == engine->m_backend_data)
    {
        return;
    }
    SoLoudWinMMData* data = static_cast<SoLoudWinMMData*>(engine->m_backend_data);
    if (data->audio_processing_done_event)
    {
        SetEvent(data->audio_processing_done_event);
    }
    if (data->buffer_end_event)
    {
        SetEvent(data->buffer_end_event);
    }
    if (data->thread_handle)
    {
        thread::wait(data->thread_handle);
        thread::release(data->thread_handle);
    }
    if (data->wave_out)
    {
        waveOutReset(data->wave_out);

        for (int i = 0; i < BUFFER_COUNT; ++i)
        {
            waveOutUnprepareHeader(data->wave_out, &data->header[i], sizeof(WAVEHDR));
            if (0 != data->sample_buffer[i])
            {
                delete[] data->sample_buffer[i];
            }
        }
        waveOutClose(data->wave_out);
    }
    if (data->audio_processing_done_event)
    {
        CloseHandle(data->audio_processing_done_event);
    }
    if (data->buffer_end_event)
    {
        CloseHandle(data->buffer_end_event);
    }
    delete data;
    engine->m_backend_data = 0;
}
} // namespace cer

void cer::winmm_init(const AudioBackendArgs& args)
{
    auto* engine = args.engine;

    SoLoudWinMMData* data          = new SoLoudWinMMData;
    engine->m_backend_data         = data;
    engine->m_backend_cleanup_func = winMMCleanup;
    data->samples                  = args.buffer;
    data->device                   = engine;
    data->buffer_end_event         = CreateEvent(0, FALSE, FALSE, 0);
    if (0 == data->buffer_end_event)
    {
        winMMCleanup(engine);
        throw std::runtime_error{"Failed to initialize winMM"};
    }
    data->audio_processing_done_event = CreateEvent(0, FALSE, FALSE, 0);
    if (0 == data->audio_processing_done_event)
    {
        winMMCleanup(engine);
        throw std::runtime_error{"Failed to initialize winMM"};
    }
    WAVEFORMATEX format;
    ZeroMemory(&format, sizeof(WAVEFORMATEX));
    format.nChannels       = WORD(args.channel_count);
    format.nSamplesPerSec  = DWORD(args.sample_rate);
    format.wFormatTag      = WAVE_FORMAT_PCM;
    format.wBitsPerSample  = sizeof(short) * 8;
    format.nBlockAlign     = (format.nChannels * format.wBitsPerSample) / 8;
    format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
    if (MMSYSERR_NOERROR != waveOutOpen(&data->wave_out,
                                        WAVE_MAPPER,
                                        &format,
                                        reinterpret_cast<DWORD_PTR>(data->buffer_end_event),
                                        0,
                                        CALLBACK_EVENT))
    {
        winMMCleanup(engine);
        throw std::runtime_error{"Failed to initialize winMM"};
    }
    data->buffer = AlignedFloatBuffer{size_t(data->samples * format.nChannels)};
    for (int i = 0; i < BUFFER_COUNT; ++i)
    {
        data->sample_buffer[i] = new short[data->samples * format.nChannels];
        ZeroMemory(&data->header[i], sizeof(WAVEHDR));
        data->header[i].dwBufferLength = DWORD(data->samples * sizeof(short) * format.nChannels);
        data->header[i].lpData         = reinterpret_cast<LPSTR>(data->sample_buffer[i]);
        if (MMSYSERR_NOERROR !=
            waveOutPrepareHeader(data->wave_out, &data->header[i], sizeof(WAVEHDR)))
        {
            winMMCleanup(engine);
            throw std::runtime_error{"Failed to initialize winMM"};
        }
    }
    engine->postinit_internal(args.sample_rate,
                              data->samples * format.nChannels,
                              args.flags,
                              args.channel_count);
    data->thread_handle = thread::create_thread(winMMThread, data);
    if (0 == data->thread_handle)
    {
        winMMCleanup(engine);
        throw std::runtime_error{"Failed to initialize winMM"};
    }
}
