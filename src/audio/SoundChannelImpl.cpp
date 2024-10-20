// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "SoundChannelImpl.hpp"

#include "AudioDevice.hpp"

namespace cer::details
{
SoundChannelImpl::SoundChannelImpl(gsl::not_null<AudioDevice*> audio_device,
                                   cer::SoundHandle            handle)
    : m_soloud(audio_device)
    , m_soloud_handle(handle)
{
}

auto SoundChannelImpl::id() const -> uint32_t
{
    return uint32_t(m_soloud_handle);
}

auto SoundChannelImpl::is_paused() const -> bool
{
    return m_soloud->pause(m_soloud_handle);
}

void SoundChannelImpl::set_is_paused(bool value)
{
    m_soloud->set_pause(m_soloud_handle, value);
}

auto SoundChannelImpl::relative_play_speed() const -> float
{
    return m_soloud->relative_play_speed(m_soloud_handle);
}

void SoundChannelImpl::set_relative_play_speed(float value)
{
    m_soloud->set_relative_play_speed(m_soloud_handle, value);
}

void SoundChannelImpl::seek(SoundTime seconds)
{
    m_soloud->seek(m_soloud_handle, seconds);
}

void SoundChannelImpl::stop()
{
    m_soloud->stop(m_soloud_handle);
}

auto SoundChannelImpl::volume() const -> float
{
    return m_soloud->volume(m_soloud_handle);
}

void SoundChannelImpl::set_volume(float value)
{
    m_soloud->set_volume(m_soloud_handle, value);
}

auto SoundChannelImpl::pan() const -> float
{
    return m_soloud->pan(m_soloud_handle);
}

void SoundChannelImpl::set_pan(float value)
{
    m_soloud->set_pan(m_soloud_handle, value);
}

void SoundChannelImpl::set_is_protected(bool value)
{
    m_soloud->set_protect_voice(m_soloud_handle, value);
}

auto SoundChannelImpl::is_looping() const -> bool
{
    return m_soloud->is_voice_looping(m_soloud_handle);
}

void SoundChannelImpl::set_is_looping(bool value)
{
    m_soloud->set_looping(m_soloud_handle, value);
}

auto SoundChannelImpl::loop_point() const -> SoundTime
{
    return SoundTime(m_soloud->get_loop_point(m_soloud_handle));
}

void SoundChannelImpl::set_loop_point(SoundTime value)
{
    m_soloud->set_loop_point(m_soloud_handle, value);
}

void SoundChannelImpl::set_inaudible_behavior(SoundInaudibleBehavior value)
{
    const auto [b1, b2] = [value] {
        switch (value)
        {
            case SoundInaudibleBehavior::PauseIfInaudible: return std::pair{false, false};
            case SoundInaudibleBehavior::KillIfInaudible: return std::pair{false, true};
            case SoundInaudibleBehavior::KeepTickingIfInaudible: return std::pair{true, false};
        }
        return std::pair{false, false};
    }();

    m_soloud->set_inaudible_behavior(m_soloud_handle, b1, b2);
}

void SoundChannelImpl::fade_volume(float to_volume, SoundTime fade_duration)
{
    m_soloud->fade_volume(m_soloud_handle, to_volume, fade_duration);
}

void SoundChannelImpl::fade_pan(float to_pan, SoundTime fade_duration)
{
    m_soloud->fade_pan(m_soloud_handle, to_pan, fade_duration);
}

void SoundChannelImpl::fade_relative_play_speed(float to_speed, SoundTime fade_duration)
{
    m_soloud->fade_relative_play_speed(m_soloud_handle, to_speed, fade_duration);
}

void SoundChannelImpl::stop_after(SoundTime after)
{
    m_soloud->schedule_stop(m_soloud_handle, after);
}

void SoundChannelImpl::pause_after(SoundTime after)
{
    m_soloud->schedule_pause(m_soloud_handle, after);
}

auto SoundChannelImpl::stream_position() const -> SoundTime
{
    return SoundTime{m_soloud->stream_position(m_soloud_handle)};
}
} // namespace cer::details