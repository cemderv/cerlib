// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "SoundImpl.hpp"

#include "util/InternalError.hpp"
#include <cstdint>
#include <cstring>
#include <gsl/narrow>

namespace cer::details
{
SoundImpl::SoundImpl(gsl::not_null<Engine*> soloud, std::span<const std::byte> data)
    : m_soloud(soloud)
      , m_data(std::make_unique<std::byte[]>(data.size()))
      , m_data_size(data.size())
{
    std::memcpy(m_data.get(), data.data(), data.size());
    init_soloud_audio_source();
}

SoundImpl::SoundImpl(gsl::not_null<Engine*>       soloud,
                     std::unique_ptr<std::byte[]> data,
                     size_t                       data_size)
    : m_soloud(soloud)
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
    m_soloud->stopAudioSource(*m_soloud_audio_source);
}

auto SoundImpl::soloud_audio_source() -> cer::Wav&
{
    return *m_soloud_audio_source;
}

void SoundImpl::init_soloud_audio_source()
{
    m_soloud_audio_source = std::make_unique<Wav>(std::span{m_data.get(), m_data_size});
}
} // namespace cer::details
