// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "audio/Wav.hpp"
#include "cerlib/Content.hpp"
#include "util/Object.hpp"
#include <cerlib/CopyMoveMacros.hpp>
#include <span>

namespace cer::details
{
class SoundImpl final : public Object, public Asset
{
  public:
    // Creates copy of data.
    explicit SoundImpl(AudioDevice& audio_device, std::span<const std::byte> data);

    explicit SoundImpl(AudioDevice&                 audio_device,
                       std::unique_ptr<std::byte[]> data,
                       size_t                       data_size);

    ~SoundImpl() noexcept override;

    void stop();

    auto audio_source() -> AudioSource&;

  private:
    void init_soloud_audio_source();

    AudioDevice*                 m_audio_device = nullptr;
    std::unique_ptr<std::byte[]> m_data;
    size_t                       m_data_size{};
    std::unique_ptr<AudioSource> m_soloud_audio_source;
};
} // namespace cer::details
