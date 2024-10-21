// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "ContentManager.hpp"

#include "FileSystem.hpp"
#include "audio/AudioDevice.hpp"
#include "audio/SoundImpl.hpp"
#include "cerlib/Audio.hpp"
#include "cerlib/Font.hpp"
#include "cerlib/Image.hpp"
#include "cerlib/Logging.hpp"
#include "cerlib/Shader.hpp"
#include "cerlib/Sound.hpp"
#include "game/GameImpl.hpp"
#include "graphics/FontImpl.hpp"
#include "graphics/ImageImpl.hpp"
#include "graphics/ShaderImpl.hpp"
#include "util/Platform.hpp"
#include <algorithm>
#include <ranges>

#ifdef CERLIB_PLATFORM_WINDOWS
#include <Windows.h>
#endif

namespace cer::details
{
static auto root_directory() -> std::string
{
#ifdef CERLIB_PLATFORM_WINDOWS
    TCHAR szFileName[MAX_PATH];
    GetModuleFileName(NULL, szFileName, MAX_PATH);

    auto ret = std::string(szFileName);

    for (auto& ch : ret)
    {
        if (ch == '\\')
            ch = '/';
    }

    const auto idx = ret.rfind('/');
    if (idx != std::string::npos)
    {
        ret.erase(idx);
    }

    if (ret.back() != '/')
    {
        ret += '/';
    }

    return ret;
#else
    return std::string{};
#endif
}

ContentManager::ContentManager()
{
    m_root_directory = root_directory();

    log_verbose("Root directory: {}", m_root_directory);

    filesystem::set_file_loading_root_directory(m_root_directory);
}

ContentManager::~ContentManager() noexcept
{
    log_verbose("Destroying ContentManager");

    for (auto& asset : m_loaded_assets | std::views::values)
    {
        // Prevent the asset from calling ContentManager::notify_asset_destroyed()
        // when its destroyed later. Because by then, the ContentManager is gone.

        // NOTE: This is ugly, but necessary until pattern matching is available.
        // std::visit is not noexcept and therefore not allowed in this destructor.

        if (auto** image = std::get_if<ImageImpl*>(&asset))
        {
            (*image)->m_content_manager = nullptr;
        }
        else if (auto** sound = std::get_if<SoundImpl*>(&asset))
        {
            if (*sound != nullptr)
            {
                (*sound)->m_content_manager = nullptr;
            }
        }
        else if (auto** shader = std::get_if<ShaderImpl*>(&asset))
        {
            (*shader)->m_content_manager = nullptr;
        }
        else if (auto** font = std::get_if<FontImpl*>(&asset))
        {
            (*font)->m_content_manager = nullptr;
        }
        else if (const auto* custom_asset = std::get_if<CustomAsset>(&asset))
        {
            if (const auto asset_shared = custom_asset->lock(); asset_shared != nullptr)
            {
                asset_shared->m_content_manager = nullptr;
            }
        }
        else
        {
            assert(false && "unhandled asset type");
        }
    }
}

void ContentManager::set_asset_loading_prefix(std::string_view prefix)
{
    m_asset_loading_prefix = prefix;

    if (!m_asset_loading_prefix.empty())
    {
        std::ranges::replace(m_asset_loading_prefix, '\\', '/');

        if (m_asset_loading_prefix.back() != '/')
        {
            m_asset_loading_prefix += '/';
        }
    }
}

auto ContentManager::asset_loading_prefix() const -> std::string_view
{
    return m_asset_loading_prefix;
}

auto ContentManager::load_image(std::string_view name) -> Image
{
    const auto key = std::string{name};

    return lazy_load<Image, ImageImpl>(key, name, [](std::string_view name) {
        const auto data  = filesystem::load_asset_data(name);
        auto       image = Image{data.as_span()};
        image.set_name(name);
        return image;
    });
}

static auto build_shader_key(std::string_view asset_name, std::span<const std::string_view> defines)
    -> std::string
{
    auto key = std::string{asset_name};

    for (const auto& define : defines)
    {
        key += '|';
        key += define;
    }

    return key;
}

auto ContentManager::load_shader(std::string_view                                   name,
                                 [[maybe_unused]] std::span<const std::string_view> defines)
    -> Shader
{
    const auto key = std::string{build_shader_key(name, defines)};

    return lazy_load<Shader, ShaderImpl>(key, name, [](std::string_view full_name) {
        const auto data   = filesystem::load_asset_data(full_name);
        auto       shader = Shader{full_name, data.as_string_view()};
        shader.set_name(full_name);
        return shader;
    });
}

auto ContentManager::load_font(std::string_view name) -> Font
{
    return lazy_load<Font, FontImpl>(name, name, [](std::string_view full_name) {
        auto data      = filesystem::load_asset_data(full_name);
        auto font_impl = std::make_unique<FontImpl>(std::move(data.data));

        return Font{font_impl.release()};
    });
}

auto ContentManager::load_sound(std::string_view name) -> Sound
{
    return lazy_load<Sound, SoundImpl>(name, name, [](std::string_view full_name) {
        if (!is_audio_device_initialized())
        {
            return Sound{};
        }

        auto& audio_device = GameImpl::instance().audio_device();
        auto  data         = filesystem::load_asset_data(full_name);

        auto sound_impl =
            std::make_unique<SoundImpl>(audio_device, std::move(data.data), data.size);

        return Sound{sound_impl.release()};
    });
}

auto ContentManager::load_custom_asset(std::string_view type_id,
                                       std::string_view name,
                                       const std::any&  extra_info) -> std::shared_ptr<Asset>
{
    if (type_id.empty())
    {
        CER_THROW_INVALID_ARG_STR("No type ID specified.");
    }

    const auto it_load_func = m_custom_asset_loaders.find(std::string(type_id));

    if (it_load_func == m_custom_asset_loaders.cend())
    {
        CER_THROW_INVALID_ARG("No custom asset loader is registered for type ID '{}'", type_id);
    }

    // Loading a custom asset requires special handling, as the types are not Object
    // based. They are reference-counted using shared_ptr instead.

    const auto key_str = std::string{m_asset_loading_prefix + std::string{name}};

    const auto it_asset = m_loaded_assets.find(key_str);

    if (it_asset != m_loaded_assets.cend())
    {
        const auto* weak_ptr = std::get_if<CustomAsset>(&it_asset->second);

        if (weak_ptr == nullptr)
        {
            CER_THROW_LOGIC_ERROR("Attempting to load custom asset '{}' with type ID '{}'. "
                                  "However, the asset was previously "
                                  "loaded as a different type.",
                                  name,
                                  type_id);
        }

        if (!weak_ptr->expired())
        {
            return weak_ptr->lock();
        }

        // The asset was somehow destroyed and we weren't notified.
        // Fall through in this case, we'll load the asset again.
    }

    // Load the asset as a shared_ptr.
    auto file_data = filesystem::load_asset_data(key_str);
    auto asset     = it_load_func->second(name, file_data, extra_info);

    asset->m_content_manager = this;
    asset->m_asset_name      = key_str;

    // Store a weak_ptr in the map.
    m_loaded_assets.emplace(key_str, asset);

    log_verbose("[ContentManager] Loaded custom asset '{}'", key_str);

    // ... but return the shared_ptr.
    return asset;
}

auto ContentManager::is_loaded(std::string_view name) const -> bool
{
    return m_loaded_assets.contains(name);
}

void ContentManager::register_custom_asset_loader(std::string_view    type_id,
                                                  CustomAssetLoadFunc load_func)
{
    if (const auto it = m_custom_asset_loaders.find(type_id); it != m_custom_asset_loaders.cend())
    {
        CER_THROW_INVALID_ARG("A custom asset loader for type '{}' is already registered.",
                              type_id);
    }

    m_custom_asset_loaders.emplace(std::string{type_id}, std::move(load_func));

    log_verbose("[ContentManager] Registered custom asset loader for type ID '{}'", type_id);
}

void ContentManager::unregister_custom_asset_loader(std::string_view type_id)
{
    const auto it = m_custom_asset_loaders.find(type_id);
    m_custom_asset_loaders.erase(it);

    log_verbose("[ContentManager] Unregistered custom asset loader for type ID '{}'", type_id);
}

void ContentManager::notify_asset_destroyed(std::string_view name)
{
    log_verbose("[ContentManager] Removing asset '{}'", name);
    m_loaded_assets.erase(std::string{name});
}
} // namespace cer::details
