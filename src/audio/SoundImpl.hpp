// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "audio/soloud_wav.hpp"
#include "cerlib/Content.hpp"
#include "util/Object.hpp"
#include <gsl/pointers>
#include <span>

namespace cer::details
{
class SoundImpl final : public Object, public Asset
{
public:
    // Creates copy of data.
    explicit SoundImpl(gsl::not_null<AudioDevice*> soloud, std::span<const std::byte> data);

    explicit SoundImpl(gsl::not_null<AudioDevice*>  soloud,
                       std::unique_ptr<std::byte[]> data,
                       size_t                       data_size);

    ~SoundImpl() noexcept override;

    void stop();

    auto soloud_audio_source() -> Wav&;

private:
    void init_soloud_audio_source();

    gsl::not_null<AudioDevice*>  m_soloud;
    std::unique_ptr<std::byte[]> m_data;
    size_t                       m_data_size{};
    std::unique_ptr<AudioSource> m_soloud_audio_source;
};
} // namespace cer::details
