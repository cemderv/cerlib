// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "audio/AudioDevice.hpp"
#include "audio/soloud_internal.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_audio.h>

namespace cer
{
static SDL_AudioSpec    gActiveAudioSpec;
static SDL_AudioStream* gAudioStream = nullptr;

void soloud_sdl3static_audiomixer(void* userdata, Uint8* stream, int len)
{
    auto* soloud = static_cast<AudioDevice*>(userdata);

    if (gActiveAudioSpec.format == SDL_AUDIO_F32)
    {
        const auto samples = len / (gActiveAudioSpec.channels * sizeof(float));
        soloud->mix(reinterpret_cast<float*>(stream), samples);
    }
    else // assume s16 if not float
    {
        const auto samples = len / (gActiveAudioSpec.channels * sizeof(short));
        soloud->mixSigned16(reinterpret_cast<int16_t*>(stream), samples);
    }
}

void soloud_sdl3static_audiomixer_new(void*                userdata,
                                      SDL_AudioStream*     stream,
                                      int                  additional_amount,
                                      [[maybe_unused]] int total_amount)
{
    if (additional_amount > 0)
    {
        if (auto* data = SDL_stack_alloc(Uint8, additional_amount))
        {
            soloud_sdl3static_audiomixer(userdata, data, additional_amount);
            SDL_PutAudioStreamData(stream, data, additional_amount);
            SDL_stack_free(data);
        }
    }
}


static void soloud_sdl3static_deinit([[maybe_unused]] AudioDevice* engine)
{
    SDL_DestroyAudioStream(gAudioStream);
}
} // namespace cer

void cer::sdl3static_init(const AudioBackendArgs& args)
{
    auto* engine = args.engine;

    // Open audio device.
    const auto as = SDL_AudioSpec{
        .format   = SDL_AUDIO_F32LE,
        .freq     = gsl::narrow<int>(args.sample_rate),
        .channels = gsl::narrow<Uint8>(args.channel_count),
    };

    gAudioStream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                             &as,
                                             soloud_sdl3static_audiomixer_new,
                                             engine);

    if (gAudioStream == nullptr)
    {
        const auto* msg = SDL_GetError();

        throw std::runtime_error{
            cer_fmt::format("Failed to initialize the SDL audio stream. Reason: {}",
                            msg != nullptr ? msg : "Unknown")};
    }

    const auto audio_device_id = SDL_GetAudioStreamDevice(gAudioStream);

    int active_sample_count = 0;
    SDL_GetAudioDeviceFormat(audio_device_id, &gActiveAudioSpec, &active_sample_count);

    engine->postinit_internal(gActiveAudioSpec.freq,
                              active_sample_count,
                              args.flags,
                              gActiveAudioSpec.channels);

    engine->m_backend_cleanup_func = soloud_sdl3static_deinit;

    SDL_ResumeAudioDevice(audio_device_id);
}
