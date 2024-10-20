// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "audio/AudioDevice.hpp"
#include "audio/soloud_internal.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_audio.h>

namespace cer
{
static SDL_AudioStream* s_audio_stream = nullptr;

void sdl3_audio_mixer(void* userdata, Uint8* stream, int len)
{
    auto& device = *static_cast<AudioDevice*>(userdata);

    const auto samples = len / (device.channels() * sizeof(float));
    device.mix(reinterpret_cast<float*>(stream), samples);
}

void sdl3_audio_mixer_new(void*                userdata,
                          SDL_AudioStream*     stream,
                          int                  additional_amount,
                          [[maybe_unused]] int total_amount)
{
    if (additional_amount > 0)
    {
        if (auto* data = SDL_stack_alloc(Uint8, additional_amount))
        {
            sdl3_audio_mixer(userdata, data, additional_amount);
            SDL_PutAudioStreamData(stream, data, additional_amount);
            SDL_stack_free(data);
        }
    }
}


static void sdl3_audio_deinit([[maybe_unused]] AudioDevice* engine)
{
    SDL_DestroyAudioStream(s_audio_stream);
}
} // namespace cer

void cer::audio_sdl3_init(const AudioBackendArgs& args)
{
    auto& device = *args.device;

    // Open audio device.
    const auto as = SDL_AudioSpec{
        .format   = SDL_AUDIO_F32LE,
        .channels = narrow<int>(args.channel_count),
        .freq     = narrow<int>(args.sample_rate),
    };

    s_audio_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                               &as,
                                               sdl3_audio_mixer_new,
                                               &device);

    if (s_audio_stream == nullptr)
    {
        const auto* msg = SDL_GetError();

        throw std::runtime_error{
            cer_fmt::format("Failed to initialize the SDL audio stream. Reason: {}",
                            msg != nullptr ? msg : "Unknown")};
    }

    const auto audio_device_id = SDL_GetAudioStreamDevice(s_audio_stream);

    device.postinit_internal(args.sample_rate, args.buffer, args.channel_count);
    device.set_backend_cleanup_func(sdl3_audio_deinit);

    SDL_ResumeAudioDevice(audio_device_id);
}
