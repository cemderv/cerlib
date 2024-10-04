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
SoundImpl::SoundImpl(gsl::not_null<SoLoud::Soloud*> soloud, std::span<const std::byte> data)
    : m_soloud(soloud)
    , m_data(std::make_unique<std::byte[]>(data.size()))
    , m_data_size(data.size())
{
    std::memcpy(m_data.get(), data.data(), data.size());
    init_soloud_audio_source();
}

SoundImpl::SoundImpl(gsl::not_null<SoLoud::Soloud*> soloud,
                     std::unique_ptr<std::byte[]>   data,
                     size_t                         data_size)
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
    m_soloud->stopAudioSource(m_soloud_audio_source);
}

auto SoundImpl::soloud_audio_source() -> SoLoud::Wav&
{
    return m_soloud_audio_source;
}

void SoundImpl::init_soloud_audio_source()
{
    const auto result =
        m_soloud_audio_source.loadMem(reinterpret_cast<const unsigned char*>(m_data.get()),
                                      gsl::narrow<unsigned int>(m_data_size),
                                      /*aCopy:*/ false,
                                      /*aTakeOwnership:*/ false);

    if (result != SoLoud::SO_NO_ERROR)
    {
        CER_THROW_RUNTIME_ERROR("Failed to create the sound ({})", [result]() -> std::string_view {
            switch (result)
            {
                case SoLoud::INVALID_PARAMETER: return "invalid parameter";
                case SoLoud::FILE_LOAD_FAILED: return "invalid or unknown audio data";
                case SoLoud::DLL_NOT_FOUND: return "library not found";
                case SoLoud::OUT_OF_MEMORY: return "out of memory";
                case SoLoud::NOT_IMPLEMENTED: return "not implemented";
                default: return "internal error";
            }
        }());
    }
}
} // namespace cer::details
