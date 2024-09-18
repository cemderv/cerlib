// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "cerlib/Sound.hpp"
#include "cerlib/SoundTypes.hpp"
#include "util/NonCopyable.hpp"

#include <optional>
#include <soloud.h>
#include <unordered_set>

namespace cer
{
class Sound;
class SoundChannel;
} // namespace cer

namespace cer::details
{
SoLoud::time to_soloud_time(const SoundTime& seconds);

class AudioDevice
{
  public:
    explicit AudioDevice(bool& success);

    NON_COPYABLE_NON_MOVABLE(AudioDevice);

    ~AudioDevice() noexcept;

    SoundChannel play_sound(const Sound&             sound,
                            float                    volume,
                            float                    pan,
                            bool                     start_paused,
                            std::optional<SoundTime> delay);

    void play_sound_fire_and_forget(const Sound&             sound,
                                    float                    volume,
                                    float                    pan,
                                    std::optional<SoundTime> delay);

    SoundChannel play_sound_in_background(const Sound& sound, float volume, bool start_paused);

    void stop_all_sounds();

    void pause_all_sounds();

    void resume_all_sounds();

    float global_volume() const;

    void set_global_volume(float value);

    void fade_global_volume(float to_volume, SoundTime fade_duration);

    SoLoud::Soloud* soloud();

    const SoLoud::Soloud* soloud() const;

    void purge_sounds();

  private:
    struct SoundHash
    {
        auto operator()(const Sound& sound) const -> size_t;
    };

    SoLoud::Soloud                       m_soloud{};
    bool                                 m_was_initialized_successfully{};
    std::unordered_set<Sound, SoundHash> m_playing_sounds;
};
} // namespace cer::details
