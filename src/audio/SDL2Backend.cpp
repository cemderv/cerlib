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
#include "audio/soloud_internal.hpp"
#include <SDL2/SDL.h>

namespace cer
{
static SDL_AudioSpec     s_active_audio_spec;
static SDL_AudioDeviceID s_audio_device_id;

void cerlib_sdl2_audio_mixer(void* userdata, Uint8* stream, int len)
{
    auto soloud = static_cast<AudioDevice*>(userdata);

    if (s_active_audio_spec.format == AUDIO_F32)
    {
        const auto samples = len / (s_active_audio_spec.channels * sizeof(float));
        soloud->mix(reinterpret_cast<float*>(stream), samples);
    }
    else // assume s16 if not float
    {
        const auto samples = len / (s_active_audio_spec.channels * sizeof(short));
        soloud->mix_signed16(reinterpret_cast<int16_t*>(stream), samples);
    }
}

static void cerlib_sdl2_deinit([[maybe_unused]] AudioDevice* engine)
{
    SDL_CloseAudioDevice(s_audio_device_id);
}
} // namespace cer

void cer::audio_sdl2_init(const AudioBackendArgs& args)
{
    auto* engine = args.device;

    auto as = SDL_AudioSpec{
        .freq     = gsl::narrow<int>(args.sample_rate),
        .format   = AUDIO_F32,
        .channels = gsl::narrow<Uint8>(args.channel_count),
        .silence  = 0,
        .samples  = gsl::narrow<Uint16>(args.buffer),
        .padding  = 0,
        .size     = 0,
        .callback = cerlib_sdl2_audio_mixer,
        .userdata = engine,
    };

    s_audio_device_id =
        SDL_OpenAudioDevice(nullptr,
                            0,
                            &as,
                            &s_active_audio_spec,
                            SDL_AUDIO_ALLOW_ANY_CHANGE &
                                ~(SDL_AUDIO_ALLOW_FORMAT_CHANGE | SDL_AUDIO_ALLOW_CHANNELS_CHANGE));
    if (s_audio_device_id == 0)
    {
        as.format = AUDIO_S16;

        s_audio_device_id =
            SDL_OpenAudioDevice(nullptr,
                                0,
                                &as,
                                &s_active_audio_spec,
                                SDL_AUDIO_ALLOW_ANY_CHANGE & ~(SDL_AUDIO_ALLOW_FORMAT_CHANGE |
                                                               SDL_AUDIO_ALLOW_CHANNELS_CHANGE));
    }

    if (s_audio_device_id == 0)
    {
        const auto* msg = SDL_GetError();

        throw std::runtime_error{
            cer_fmt::format("Failed to initialize the SDL audio device. Reason: {}",
                            msg != nullptr ? msg : "Unknown")};
    }

    engine->postinit_internal(s_active_audio_spec.freq,
                              s_active_audio_spec.samples,
                              s_active_audio_spec.channels);

    engine->set_backend_cleanup_func(cerlib_sdl2_deinit);

    SDL_PauseAudioDevice(s_audio_device_id, 0);
}
