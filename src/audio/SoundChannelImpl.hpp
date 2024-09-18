// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "cerlib/SoundTypes.hpp"
#include "util/Object.hpp"

#include <gsl/pointers>
#include <soloud.h>

namespace cer::details
{
class SoundChannelImpl final : public Object
{
  public:
    explicit SoundChannelImpl(gsl::not_null<SoLoud::Soloud*> soloud, SoLoud::handle handle);

    uint32_t id() const;

    bool is_paused() const;

    void set_is_paused(bool value);

    float relative_play_speed() const;

    void set_relative_play_speed(float value);

    void seek(SoundTime seconds);

    void stop();

    float volume() const;

    void set_volume(float value);

    float pan() const;

    void set_pan(float value);

    void set_is_protected(bool value);

    bool is_looping() const;

    void set_is_looping(bool value);

    SoundTime loop_point() const;

    void set_loop_point(SoundTime value);

    void set_inaudible_behavior(SoundInaudibleBehavior value);

    void fade_volume(float toVolume, SoundTime fadeDuration);

    void fade_pan(float toPan, SoundTime fadeDuration);

    void fade_relative_play_speed(float toSpeed, SoundTime fadeDuration);

    void stop_after(SoundTime after);

    void pause_after(SoundTime after);

    SoundTime stream_position() const;

  private:
    gsl::not_null<SoLoud::Soloud*> m_soloud;
    SoLoud::handle                 m_soloud_handle;
};
} // namespace cer::details