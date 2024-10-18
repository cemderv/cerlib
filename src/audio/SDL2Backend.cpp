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

#include "soloud.hpp"
#include "soloud_engine.hpp"
#include <cstdlib>

#include "SDL.h"
#include <math.h>

namespace cer
{
static SDL_AudioSpec     gActiveAudioSpec;
static SDL_AudioDeviceID gAudioDeviceID;

void soloud_sdl2static_audiomixer(void* userdata, Uint8* stream, int len)
{
    short*       buf    = (short*)stream;
    AudioDevice* soloud = (AudioDevice*)userdata;
    if (gActiveAudioSpec.format == AUDIO_F32)
    {
        int samples = len / (gActiveAudioSpec.channels * sizeof(float));
        soloud->mix((float*)buf, samples);
    }
    else // assume s16 if not float
    {
        int samples = len / (gActiveAudioSpec.channels * sizeof(short));
        soloud->mixSigned16(buf, samples);
    }
}

static void soloud_sdl2static_deinit(AudioDevice* engine)
{
    SDL_CloseAudioDevice(gAudioDeviceID);
}

void sdl2static_init(
    AudioDevice* engine, EngineFlags aFlags, size_t aSamplerate, size_t aBuffer, size_t aChannels)
{
    if (!SDL_WasInit(SDL_INIT_AUDIO))
    {
        if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
        {
            throw std::runtime_error{"Failed to initialize the SDL audio backend"};
        }
    }

    SDL_AudioSpec as;
    as.freq     = aSamplerate;
    as.format   = AUDIO_F32;
    as.channels = aChannels;
    as.samples  = aBuffer;
    as.callback = soloud_sdl2static_audiomixer;
    as.userdata = engine;

    gAudioDeviceID =
        SDL_OpenAudioDevice(nullptr,
                            0,
                            &as,
                            &gActiveAudioSpec,
                            SDL_AUDIO_ALLOW_ANY_CHANGE &
                                ~(SDL_AUDIO_ALLOW_FORMAT_CHANGE | SDL_AUDIO_ALLOW_CHANNELS_CHANGE));
    if (gAudioDeviceID == 0)
    {
        as.format = AUDIO_S16;
        gAudioDeviceID =
            SDL_OpenAudioDevice(nullptr,
                                0,
                                &as,
                                &gActiveAudioSpec,
                                SDL_AUDIO_ALLOW_ANY_CHANGE & ~(SDL_AUDIO_ALLOW_FORMAT_CHANGE |
                                                               SDL_AUDIO_ALLOW_CHANNELS_CHANGE));
        if (gAudioDeviceID == 0)
        {
            throw std::runtime_error{"Failed to initialize the SDL audio backend"};
        }
    }

    engine->postinit_internal(gActiveAudioSpec.freq,
                              gActiveAudioSpec.samples,
                              aFlags,
                              gActiveAudioSpec.channels);

    engine->mBackendCleanupFunc = soloud_sdl2static_deinit;

    SDL_PauseAudioDevice(gAudioDeviceID, 0);
}
}; // namespace cer
