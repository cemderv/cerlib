// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "cerlib/Sound.hpp"

#include "AudioDevice.hpp"
#include "SoundImpl.hpp"
#include "contentmanagement/ContentManager.hpp"
#include "game/GameImpl.hpp"

namespace cer
{
CERLIB_IMPLEMENT_OBJECT(Sound);

Sound::Sound(std::span<const std::byte> data)
    : m_impl(nullptr)
{
    auto& audio_device = details::GameImpl::instance().audio_device();

    auto impl = std::make_unique<details::SoundImpl>(audio_device, data);

    set_impl(*this, impl.release());
}

Sound::Sound(std::string_view asset_name)
    : m_impl(nullptr)
{
    auto& content = details::GameImpl::instance().content_manager();
    *this         = content.load_sound(asset_name);
}

void Sound::stop()
{
    DECLARE_THIS_IMPL;
    impl->stop();
}
} // namespace cer
