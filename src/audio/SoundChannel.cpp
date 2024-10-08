// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "cerlib/SoundChannel.hpp"

#include "SoundChannelImpl.hpp"
#include "cerlib/Math.hpp"
#include "util/Util.hpp"

namespace cer
{
CERLIB_IMPLEMENT_OBJECT(SoundChannel);

uint32_t SoundChannel::id() const
{
    DECLARE_THIS_IMPL;
    return impl->id();
}

bool SoundChannel::is_paused() const
{
    DECLARE_THIS_IMPL;
    return impl->is_paused();
}

void SoundChannel::set_paused(bool value)
{
    DECLARE_THIS_IMPL;
    impl->set_is_paused(value);
}

float SoundChannel::relative_playback_speed() const
{
    DECLARE_THIS_IMPL;
    return impl->relative_play_speed();
}

void SoundChannel::set_relative_playback_speed(float value)
{
    DECLARE_THIS_IMPL;
    impl->set_relative_play_speed(value);
}

void SoundChannel::seek(SoundTime seconds)
{
    DECLARE_THIS_IMPL;
    impl->seek(seconds);
}

void SoundChannel::stop()
{
    DECLARE_THIS_IMPL;
    impl->stop();
}

float SoundChannel::volume() const
{
    DECLARE_THIS_IMPL;
    return impl->volume();
}

void SoundChannel::set_volume(float value)
{
    DECLARE_THIS_IMPL;
    impl->set_volume(clamp(value, 0.0f, 3.0f));
}

float SoundChannel::pan() const
{
    DECLARE_THIS_IMPL;
    return impl->pan();
}

void SoundChannel::set_pan(float value)
{
    DECLARE_THIS_IMPL;
    impl->set_pan(clamp(value, -1.0f, 1.0f));
}

void SoundChannel::set_protected(bool value)
{
    DECLARE_THIS_IMPL;
    impl->set_is_protected(value);
}

bool SoundChannel::is_looping() const
{
    DECLARE_THIS_IMPL;
    return impl->is_looping();
}

void SoundChannel::set_looping(bool value)
{
    DECLARE_THIS_IMPL;
    impl->set_is_looping(value);
}

SoundTime SoundChannel::loop_point() const
{
    DECLARE_THIS_IMPL;
    return impl->loop_point();
}

void SoundChannel::set_loop_point(SoundTime value)
{
    DECLARE_THIS_IMPL;
    impl->set_loop_point(value);
}

void SoundChannel::set_inaudible_behavior(SoundInaudibleBehavior value)
{
    DECLARE_THIS_IMPL;
    impl->set_inaudible_behavior(value);
}

void SoundChannel::fade_volume(float to_volume, SoundTime fade_duration)
{
    DECLARE_THIS_IMPL;
    impl->fade_volume(to_volume, fade_duration);
}

void SoundChannel::fade_pan(float to_pan, SoundTime fade_duration)
{
    DECLARE_THIS_IMPL;
    impl->fade_pan(to_pan, fade_duration);
}

void SoundChannel::fade_relative_playback_speed(float to_speed, SoundTime fade_duration)
{
    DECLARE_THIS_IMPL;
    impl->fade_relative_play_speed(to_speed, fade_duration);
}

void SoundChannel::stop_after(SoundTime after)
{
    DECLARE_THIS_IMPL;
    impl->stop_after(after);
}

void SoundChannel::pause_after(SoundTime after)
{
    DECLARE_THIS_IMPL;
    impl->pause_after(after);
}

SoundTime SoundChannel::stream_position() const
{
    DECLARE_THIS_IMPL;
    return impl->stream_position();
}
} // namespace cer