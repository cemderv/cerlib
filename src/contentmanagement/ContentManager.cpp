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
#include <format>

namespace cer::details
{
static std::string root_directory()
{
#if defined(_WIN32)
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

    for (auto& loaded_asset_pair : m_loaded_assets)
    {
        // Prevent the asset from calling ContentManager::notify_asset_destroyed()
        // when its destroyed later. Because by then, the ContentManager is gone.

        struct Visitor
        {
            void operator()(ImageImpl* image) const
            {
                image->m_content_manager = nullptr;
            }

            void operator()(SoundImpl* sound) const
            {
                sound->m_content_manager = nullptr;
            }

            void operator()(ShaderImpl* shader) const
            {
                shader->m_content_manager = nullptr;
            }

            void operator()(FontImpl* font) const
            {
                font->m_content_manager = nullptr;
            }

            void operator()(const CustomAsset& custom_asset) const
            {
                if (const std::shared_ptr<Asset> asset = custom_asset.lock(); asset != nullptr)
                {
                    asset->m_content_manager = nullptr;
                }
            }
        };

        std::visit(Visitor{}, loaded_asset_pair.second);
    }
}

void ContentManager::set_asset_loading_prefix(std::string_view prefix)
{
    m_asset_loading_prefix = prefix;

    if (!m_asset_loading_prefix.empty())
    {
        for (auto& ch : m_asset_loading_prefix)
        {
            if (ch == '\\')
            {
                ch = '/';
            }
        }

        if (m_asset_loading_prefix.back() != '/')
        {
            m_asset_loading_prefix += '/';
        }
    }
}

std::string_view ContentManager::asset_loading_prefix() const
{
    return m_asset_loading_prefix;
}

Image ContentManager::load_image(std::string_view name, bool generate_mipmaps)
{
    std::string key{name};

    if (generate_mipmaps)
    {
        key += "_mipmapped";
    }

    return lazy_load<Image, ImageImpl>(key, name, [generate_mipmaps](std::string_view name) {
        const AssetData data{filesystem::load_asset_data(name)};
        Image           image{data.as_span(), generate_mipmaps};
        image.set_name(name);
        return image;
    });
}

static std::string build_shader_key(std::string_view                  assetName,
                                    std::span<const std::string_view> defines)
{
    std::string key{assetName};

    for (const std::string_view& define : defines)
    {
        key += '|';
        key += define;
    }

    return key;
}

Shader ContentManager::load_shader(std::string_view name, std::span<const std::string_view> defines)
{
    const std::string key{build_shader_key(name, defines)};

    return lazy_load<Shader, ShaderImpl>(key, name, [defines](std::string_view fullName) {
        const AssetData data = filesystem::load_asset_data(fullName);
        Shader          shader{fullName, data.as_string_view(), defines};
        shader.set_name(fullName);
        return shader;
    });
}

Font ContentManager::load_font(std::string_view name)
{
    return lazy_load<Font, FontImpl>(name, name, [](std::string_view fullName) {
        AssetData data = filesystem::load_asset_data(fullName);

        std::unique_ptr<FontImpl> fontImpl = std::make_unique<FontImpl>(std::move(data.data));

        return Font{fontImpl.release()};
    });
}

Sound ContentManager::load_sound(std::string_view name)
{
    return lazy_load<Sound, SoundImpl>(name, name, [](std::string_view fullName) {
        if (!is_audio_device_initialized())
        {
            return Sound{};
        }

        AudioDevice& audioDevice = GameImpl::instance().audio_device();
        AssetData    data        = filesystem::load_asset_data(fullName);

        std::unique_ptr<SoundImpl> soundImpl =
            std::make_unique<SoundImpl>(audioDevice.soloud(), std::move(data.data), data.size);

        return Sound{soundImpl.release()};
    });
}

std::shared_ptr<Asset> ContentManager::load_custom_asset(std::string_view type_id,
                                                         std::string_view name,
                                                         const std::any&  extra_info)
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

    const std::string key_str{m_asset_loading_prefix + std::string{name}};

    const auto it_asset = m_loaded_assets.find(key_str);

    if (it_asset != m_loaded_assets.cend())
    {
        const CustomAsset* weak_ptr = std::get_if<CustomAsset>(&it_asset->second);

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
    AssetData              file_data = filesystem::load_asset_data(key_str);
    std::shared_ptr<Asset> asset     = it_load_func->second(name, file_data, extra_info);

    asset->m_content_manager = this;
    asset->m_asset_name      = key_str;

    // Store a weak_ptr in the map.
    m_loaded_assets.emplace(key_str, asset);

    log_verbose("[ContentManager] Loaded custom asset '{}'", key_str);

    // ... but return the shared_ptr.
    return asset;
}

bool ContentManager::is_loaded(std::string_view name) const
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

    m_custom_asset_loaders.emplace(std::string(type_id), std::move(load_func));

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
