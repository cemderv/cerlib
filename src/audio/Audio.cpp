// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "cerlib/Audio.hpp"

#include "AudioDevice.hpp"
#include "cerlib/SoundChannel.hpp"
#include "game/GameImpl.hpp"

// NOLINTBEGIN

#define LOAD_AUDIO_ENGINE_IMPL_OR_RETURN                                                           \
    if (!is_audio_device_initialized())                                                            \
        return;                                                                                    \
    auto& impl = details::GameImpl::instance().audio_device()

#define LOAD_AUDIO_ENGINE_IMPL_OR_RETURN_VALUE(value)                                              \
    if (!is_audio_device_initialized())                                                            \
        return value;                                                                              \
    auto& impl = details::GameImpl::instance().audio_device()

// NOLINTEND

bool cer::is_audio_device_initialized()
{
    return details::GameImpl::instance().is_audio_device_initialized();
}

cer::SoundChannel cer::play_sound(
    const Sound& sound, float volume, float pan, bool start_paused, std::optional<SoundTime> delay)
{
    LOAD_AUDIO_ENGINE_IMPL_OR_RETURN_VALUE(SoundChannel{});
    return impl.play_sound(sound, volume, pan, start_paused, delay);
}

void cer::play_sound_fire_and_forget(const Sound&             sound,
                                     float                    volume,
                                     float                    pan,
                                     std::optional<SoundTime> delay)
{
    LOAD_AUDIO_ENGINE_IMPL_OR_RETURN;
    impl.play_sound_fire_and_forget(sound, volume, pan, delay);
}

cer::SoundChannel cer::play_sound_in_background(const Sound& sound, float volume, bool start_paused)
{
    LOAD_AUDIO_ENGINE_IMPL_OR_RETURN_VALUE(SoundChannel{});
    return impl.play_sound_in_background(sound, volume, start_paused);
}

void cer::stop_all_sounds()
{
    LOAD_AUDIO_ENGINE_IMPL_OR_RETURN;
    impl.stop_all_sounds();
}

void cer::pause_all_sounds()
{
    LOAD_AUDIO_ENGINE_IMPL_OR_RETURN;
    impl.pause_all_sounds();
}

void cer::resume_all_sounds()
{
    LOAD_AUDIO_ENGINE_IMPL_OR_RETURN;
    impl.resume_all_sounds();
}

void cer::set_global_volume(float value)
{
    LOAD_AUDIO_ENGINE_IMPL_OR_RETURN;
    impl.set_global_volume(value);
}

void cer::fade_global_volume(float to_volume, SoundTime fade_duration)
{
    LOAD_AUDIO_ENGINE_IMPL_OR_RETURN;
    impl.fade_global_volume(to_volume, fade_duration);
}
