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

    auto id() const -> uint32_t;

    auto is_paused() const -> bool;

    void set_is_paused(bool value);

    auto relative_play_speed() const -> float;

    void set_relative_play_speed(float value);

    void seek(SoundTime seconds);

    void stop();

    auto volume() const -> float;

    void set_volume(float value);

    auto pan() const -> float;

    void set_pan(float value);

    void set_is_protected(bool value);

    auto is_looping() const -> bool;

    void set_is_looping(bool value);

    auto loop_point() const -> SoundTime;

    void set_loop_point(SoundTime value);

    void set_inaudible_behavior(SoundInaudibleBehavior value);

    void fade_volume(float to_volume, SoundTime fade_duration);

    void fade_pan(float to_pan, SoundTime fade_duration);

    void fade_relative_play_speed(float to_speed, SoundTime fade_duration);

    void stop_after(SoundTime after);

    void pause_after(SoundTime after);

    auto stream_position() const -> SoundTime;

  private:
    gsl::not_null<SoLoud::Soloud*> m_soloud;
    SoLoud::handle                 m_soloud_handle;
};
} // namespace cer::details