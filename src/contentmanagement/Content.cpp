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

auto Asset::asset_name() const -> std::string_view
{
    return m_asset_name;
}
} // namespace cer

void cer::set_asset_loading_prefix(std::string_view prefix)
{
    LOAD_CONTENT_MANAGER;
    content.set_asset_loading_prefix(prefix);
}

auto cer::asset_loading_prefix() -> std::string
{
    LOAD_CONTENT_MANAGER;
    return std::string{content.asset_loading_prefix()};
}

auto cer::load_image(std::string_view name) -> cer::Image
{
    LOAD_CONTENT_MANAGER;
    return content.load_image(name);
}

auto cer::load_shader(std::string_view name, std::span<const std::string_view> defines)
    -> cer::Shader
{
    LOAD_CONTENT_MANAGER;
    return content.load_shader(name, defines);
}

auto cer::load_font(std::string_view name) -> cer::Font
{
    LOAD_CONTENT_MANAGER;
    return content.load_font(name);
}

auto cer::load_sound(std::string_view name) -> cer::Sound
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

auto cer::load_custom_asset(std::string_view type_id,
                            std::string_view name,
                            const std::any&  extra_info) -> std::shared_ptr<cer::Asset>
{
    LOAD_CONTENT_MANAGER;
    return content.load_custom_asset(type_id, name, extra_info);
}

auto cer::is_asset_loaded(std::string_view name) -> bool
{
    LOAD_CONTENT_MANAGER;
    return content.is_loaded(name);
}
