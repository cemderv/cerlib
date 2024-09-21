// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "cerlib/Content.hpp"
#include "ContentManager.hpp"
#include "cerlib/Font.hpp"
#include "cerlib/Image.hpp"
#include "cerlib/Shader.hpp"
#include "cerlib/Sound.hpp"
#include "game/GameImpl.hpp"
#include <cassert>

// NOLINTBEGIN
#define LOAD_CONTENT_MANAGER auto& content = details::GameImpl::instance().content_manager()
// NOLINTEND

namespace cer
{
Asset::Asset() = default;

Asset::~Asset() noexcept
{
    if (m_content_manager != nullptr)
    {
        assert(!m_asset_name.empty());
        m_content_manager->notify_asset_destroyed(m_asset_name);
    }
}

std::string_view Asset::asset_name() const
{
    return m_asset_name;
}
} // namespace cer

void cer::set_asset_loading_prefix(std::string_view prefix)
{
    LOAD_CONTENT_MANAGER;
    content.set_asset_loading_prefix(prefix);
}

std::string cer::asset_loading_prefix()
{
    LOAD_CONTENT_MANAGER;
    return std::string(content.asset_loading_prefix());
}

cer::Image cer::load_image(std::string_view name, bool generate_mipmaps)
{
    LOAD_CONTENT_MANAGER;
    return content.load_image(name, generate_mipmaps);
}

cer::Shader cer::load_shader(std::string_view name, std::span<const std::string_view> defines)
{
    LOAD_CONTENT_MANAGER;
    return content.load_shader(name, defines);
}

cer::Font cer::load_font(std::string_view name)
{
    LOAD_CONTENT_MANAGER;
    return content.load_font(name);
}

cer::Sound cer::load_sound(std::string_view name)
{
    LOAD_CONTENT_MANAGER;
    return content.load_sound(name);
}

void cer::register_custom_asset_loader(std::string_view type_id, CustomAssetLoadFunc load_func)
{
    LOAD_CONTENT_MANAGER;
    content.register_custom_asset_loader(type_id, std::move(load_func));
}

void cer::unregister_custom_asset_loader(std::string_view type_id)
{
    LOAD_CONTENT_MANAGER;
    content.unregister_custom_asset_loader(type_id);
}

std::shared_ptr<cer::Asset> cer::load_custom_asset(std::string_view type_id,
                                                   std::string_view name,
                                                   const std::any&  extra_info)
{
    LOAD_CONTENT_MANAGER;
    return content.load_custom_asset(type_id, name, extra_info);
}

bool cer::is_asset_loaded(std::string_view name)
{
    LOAD_CONTENT_MANAGER;
    return content.is_loaded(name);
}
