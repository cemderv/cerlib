// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "AudioDevice.hpp"

#include "SoundChannelImpl.hpp"
#include "SoundImpl.hpp"
#include "cerlib/Logging.hpp"
#include "cerlib/SoundChannel.hpp"
#include "soloud.h"
#include "util/InternalError.hpp"

namespace cer::details
{
auto to_soloud_time(const SoundTime& seconds) -> SoLoud::time
{
    return seconds.count();
}

AudioDevice::AudioDevice(bool& success)
{
    if (const SoLoud::result result = m_soloud.init(); result != SoLoud::SO_NO_ERROR)
    {
        log_debug("Failed to initialize the internal audio engine.");
        return;
    }

    m_was_initialized_successfully = true;
    success                        = true;
}

AudioDevice::~AudioDevice() noexcept
{
    log_verbose("Destroying AudioDevice");

    if (m_was_initialized_successfully)
    {
        m_playing_sounds.clear();
        m_soloud.deinit();
    }
}

auto AudioDevice::play_sound(const Sound&             sound,
                             float                    volume,
                             float                    pan,
                             bool                     start_paused,
                             std::optional<SoundTime> delay) -> SoundChannel
{
    if (!sound)
    {
        return {};
    }

    const auto channel_handle =
        delay ? m_soloud.playClocked(to_soloud_time(*delay),
                                     sound.impl()->soloud_audio_source(),
                                     volume,
                                     pan)
              : m_soloud.play(sound.impl()->soloud_audio_source(), volume, pan, start_paused);

    // TODO: Use pool allocation for SoundChannelImpl objects
    auto channel_impl = std::make_unique<SoundChannelImpl>(&m_soloud, channel_handle);

    m_playing_sounds.insert(sound);

    return SoundChannel{channel_impl.release()};
}

void AudioDevice::play_sound_fire_and_forget(const Sound&             sound,
                                             float                    volume,
                                             float                    pan,
                                             std::optional<SoundTime> delay)
{
    if (!sound)
    {
        return;
    }

    if (delay.has_value())
    {
        m_soloud.playClocked(to_soloud_time(*delay),
                             sound.impl()->soloud_audio_source(),
                             volume,
                             pan);
    }
    else
    {
        m_soloud.play(sound.impl()->soloud_audio_source(), volume, pan, false);
    }

    m_playing_sounds.insert(sound);
}

auto AudioDevice::play_sound_in_background(const Sound& sound, float volume, bool start_paused)
    -> SoundChannel
{
    if (!sound)
    {
        return {};
    }

    auto channel = play_sound(sound, volume, 0.0f, start_paused, std::nullopt);

    m_soloud.setPanAbsolute(channel.id(), 1.0f, 1.0f);

    m_playing_sounds.insert(sound);

    return channel;
}

void AudioDevice::stop_all_sounds()
{
    m_soloud.stopAll();
}

void AudioDevice::pause_all_sounds()
{
    m_soloud.setPauseAll(true);
}

void AudioDevice::resume_all_sounds()
{
    m_soloud.setPauseAll(false);
}

auto AudioDevice::global_volume() const -> float
{
    return m_soloud.getGlobalVolume();
}

void AudioDevice::set_global_volume(float value)
{
    m_soloud.setGlobalVolume(value);
}

void AudioDevice::fade_global_volume(float to_volume, SoundTime fade_duration)
{
    m_soloud.fadeGlobalVolume(to_volume, to_soloud_time(fade_duration));
}

auto AudioDevice::soloud() -> SoLoud::Soloud*
{
    return &m_soloud;
}

auto AudioDevice::soloud() const -> const SoLoud::Soloud*
{
    return &m_soloud;
}

void AudioDevice::purge_sounds()
{
    std::erase_if(m_playing_sounds, [this](const Sound& sound) {
        return m_soloud.countAudioSource(sound.impl()->soloud_audio_source()) == 0;
    });
}

auto AudioDevice::SoundHash::operator()(const Sound& sound) const -> size_t
{
    return reinterpret_cast<size_t>(sound.impl());
}
} // namespace cer::details
