// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "cerlib/Content.hpp"
#include "util/Object.hpp"
#include <gsl/pointers>
#include <soloud.h>
#include <soloud_wav.h>
#include <span>

namespace cer::details
{
class SoundImpl final : public Object, public Asset
{
  public:
    // Creates copy of data.
    explicit SoundImpl(gsl::not_null<SoLoud::Soloud*> soloud, std::span<const std::byte> data);

    explicit SoundImpl(gsl::not_null<SoLoud::Soloud*> soloud,
                       std::unique_ptr<std::byte[]>   data,
                       size_t                         data_size);

    ~SoundImpl() noexcept override;

    void stop();

    SoLoud::Wav& soloud_audio_source();

  private:
    void init_soloud_audio_source();

    gsl::not_null<SoLoud::Soloud*> m_soloud;
    std::unique_ptr<std::byte[]>   m_data;
    size_t                         m_data_size{};
    SoLoud::Wav                    m_soloud_audio_source{};
};
} // namespace cer::details
