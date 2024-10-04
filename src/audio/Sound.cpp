// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "cerlib/Sound.hpp"

#include "AudioDevice.hpp"
#include "SoundImpl.hpp"
#include "game/GameImpl.hpp"
#include "util/Util.hpp"

namespace cer
{
CERLIB_IMPLEMENT_OBJECT(Sound);

Sound::Sound(std::span<const std::byte> data)
    : m_impl(nullptr)
{
    details::AudioDevice& audio_device = details::GameImpl::instance().audio_device();

    auto impl = std::make_unique<details::SoundImpl>(audio_device.soloud(), data);

    set_impl(*this, impl.release());
}

void Sound::stop()
{
    DECLARE_THIS_IMPL;
    impl->stop();
}
} // namespace cer
