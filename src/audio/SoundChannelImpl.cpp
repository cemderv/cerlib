// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "SoundChannelImpl.hpp"

#include "AudioDevice.hpp"

namespace cer::details
{
SoundChannelImpl::SoundChannelImpl(gsl::not_null<cer::Engine*> soloud, cer::handle handle)
    : m_soloud(soloud)
    , m_soloud_handle(handle)
{
}

uint32_t SoundChannelImpl::id() const
{
    return uint32_t(m_soloud_handle);
}

auto SoundChannelImpl::is_paused() const -> bool
{
    return m_soloud->getPause(m_soloud_handle);
}

void SoundChannelImpl::set_is_paused(bool value)
{
    m_soloud->setPause(m_soloud_handle, value);
}

auto SoundChannelImpl::relative_play_speed() const -> float
{
    return m_soloud->getRelativePlaySpeed(m_soloud_handle);
}

void SoundChannelImpl::set_relative_play_speed(float value)
{
    m_soloud->setRelativePlaySpeed(m_soloud_handle, value);
}

void SoundChannelImpl::seek(SoundTime seconds)
{
    m_soloud->seek(m_soloud_handle, to_soloud_time(seconds));
}

void SoundChannelImpl::stop()
{
    m_soloud->stop(m_soloud_handle);
}

auto SoundChannelImpl::volume() const -> float
{
    return m_soloud->getVolume(m_soloud_handle);
}

void SoundChannelImpl::set_volume(float value)
{
    m_soloud->setVolume(m_soloud_handle, value);
}

auto SoundChannelImpl::pan() const -> float
{
    return m_soloud->getPan(m_soloud_handle);
}

void SoundChannelImpl::set_pan(float value)
{
    m_soloud->setPan(m_soloud_handle, value);
}

void SoundChannelImpl::set_is_protected(bool value)
{
    m_soloud->setProtectVoice(m_soloud_handle, value);
}

auto SoundChannelImpl::is_looping() const -> bool
{
    return m_soloud->getLooping(m_soloud_handle);
}

void SoundChannelImpl::set_is_looping(bool value)
{
    m_soloud->setLooping(m_soloud_handle, value);
}

auto SoundChannelImpl::loop_point() const -> SoundTime
{
    return SoundTime(m_soloud->getLoopPoint(m_soloud_handle));
}

void SoundChannelImpl::set_loop_point(SoundTime value)
{
    m_soloud->setLoopPoint(m_soloud_handle, to_soloud_time(value));
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

    m_soloud->setInaudibleBehavior(m_soloud_handle, b1, b2);
}

void SoundChannelImpl::fade_volume(float to_volume, SoundTime fade_duration)
{
    m_soloud->fadeVolume(m_soloud_handle, to_volume, to_soloud_time(fade_duration));
}

void SoundChannelImpl::fade_pan(float to_pan, SoundTime fade_duration)
{
    m_soloud->fadePan(m_soloud_handle, to_pan, to_soloud_time(fade_duration));
}

void SoundChannelImpl::fade_relative_play_speed(float to_speed, SoundTime fade_duration)
{
    m_soloud->fadeRelativePlaySpeed(m_soloud_handle, to_speed, to_soloud_time(fade_duration));
}

void SoundChannelImpl::stop_after(SoundTime after)
{
    m_soloud->scheduleStop(m_soloud_handle, to_soloud_time(after));
}

void SoundChannelImpl::pause_after(SoundTime after)
{
    m_soloud->schedulePause(m_soloud_handle, to_soloud_time(after));
}

auto SoundChannelImpl::stream_position() const -> SoundTime
{
    return SoundTime{m_soloud->getStreamPosition(m_soloud_handle)};
}
} // namespace cer::details