// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "SoundImpl.hpp"

#include "audio/AudioDevice.hpp"
#include <cstring>

namespace cer::details
{
SoundImpl::SoundImpl(AudioDevice& audio_device, std::span<const std::byte> data)
    : m_audio_device(&audio_device)
    , m_data(std::make_unique<std::byte[]>(data.size()))
    , m_data_size(data.size())
{
    std::memcpy(m_data.get(), data.data(), data.size());
    init_soloud_audio_source();
}

SoundImpl::SoundImpl(AudioDevice& audio_device, UniquePtr<std::byte[]> data, size_t data_size)
    : m_audio_device(&audio_device)
    , m_data(std::move(data))
    , m_data_size(data_size)
{
    init_soloud_audio_source();
}

SoundImpl::~SoundImpl() noexcept
{
    stop();
}

void SoundImpl::stop()
{
    m_audio_device->stop_audio_source(*m_soloud_audio_source);
}

auto SoundImpl::audio_source() -> AudioSource&
{
    return *m_soloud_audio_source;
}

void SoundImpl::init_soloud_audio_source()
{
    m_soloud_audio_source = std::make_unique<Wav>(std::span{m_data.get(), m_data_size});
}
} // namespace cer::details
