// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "SoundChannelImpl.hpp"

#include "AudioDevice.hpp"

namespace cer::details
{
SoundChannelImpl::SoundChannelImpl(gsl::not_null<SoLoud::Soloud*> soloud, SoLoud::handle handle)
    : m_soloud(soloud)
    , m_soloud_handle(handle)
{
}

uint32_t SoundChannelImpl::id() const
{
    return static_cast<uint32_t>(m_soloud_handle);
}

bool SoundChannelImpl::is_paused() const
{
    return m_soloud->getPause(m_soloud_handle);
}

void SoundChannelImpl::set_is_paused(bool value)
{
    m_soloud->setPause(m_soloud_handle, value);
}

float SoundChannelImpl::relative_play_speed() const
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

float SoundChannelImpl::volume() const
{
    return m_soloud->getVolume(m_soloud_handle);
}

void SoundChannelImpl::set_volume(float value)
{
    m_soloud->setVolume(m_soloud_handle, value);
}

float SoundChannelImpl::pan() const
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

bool SoundChannelImpl::is_looping() const
{
    return m_soloud->getLooping(m_soloud_handle);
}

void SoundChannelImpl::set_is_looping(bool value)
{
    m_soloud->setLooping(m_soloud_handle, value);
}

SoundTime SoundChannelImpl::loop_point() const
{
    return SoundTime(m_soloud->getLoopPoint(m_soloud_handle));
}

void SoundChannelImpl::set_loop_point(SoundTime value)
{
    m_soloud->setLoopPoint(m_soloud_handle, to_soloud_time(value));
}

void SoundChannelImpl::set_inaudible_behavior(SoundInaudibleBehavior value)
{
    const auto [b1, b2] = [value]() -> std::pair<bool, bool> {
        switch (value)
        {
            case SoundInaudibleBehavior::PauseIfInaudible: return {false, false};
            case SoundInaudibleBehavior::KillIfInaudible: return {false, true};
            case SoundInaudibleBehavior::KeepTickingIfInaudible: return {true, false};
        }
        return {false, false};
    }();

    m_soloud->setInaudibleBehavior(m_soloud_handle, b1, b2);
}

void SoundChannelImpl::fade_volume(float toVolume, SoundTime fadeDuration)
{
    m_soloud->fadeVolume(m_soloud_handle, toVolume, to_soloud_time(fadeDuration));
}

void SoundChannelImpl::fade_pan(float toPan, SoundTime fadeDuration)
{
    m_soloud->fadePan(m_soloud_handle, toPan, to_soloud_time(fadeDuration));
}

void SoundChannelImpl::fade_relative_play_speed(float toSpeed, SoundTime fadeDuration)
{
    m_soloud->fadeRelativePlaySpeed(m_soloud_handle, toSpeed, to_soloud_time(fadeDuration));
}

void SoundChannelImpl::stop_after(SoundTime after)
{
    m_soloud->scheduleStop(m_soloud_handle, to_soloud_time(after));
}

void SoundChannelImpl::pause_after(SoundTime after)
{
    m_soloud->schedulePause(m_soloud_handle, to_soloud_time(after));
}

SoundTime SoundChannelImpl::stream_position() const
{
    return SoundTime{m_soloud->getStreamPosition(m_soloud_handle)};
}
} // namespace cer::details