// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "cerlib/Content.hpp"
#include "cerlib/Logging.hpp"
#include "cerlib/Shader.hpp"
#include "graphics/ShaderImpl.hpp"
#include "util/NonCopyable.hpp"
#include "util/StringUnorderedMap.hpp"
#include "util/Util.hpp"
#include <gsl/pointers>
#include <span>
#include <string>
#include <string_view>
#include <variant>

namespace cer::details
{
class ImageImpl;
class ShaderImpl;
class FontImpl;
class SoundImpl;

class ContentManager final
{
  public:
    ContentManager();

    NON_COPYABLE_NON_MOVABLE(ContentManager);

    ~ContentManager() noexcept;

    void set_asset_loading_prefix(std::string_view prefix);

    auto asset_loading_prefix() const -> std::string_view;

    auto load_image(std::string_view name, bool generate_mipmaps) -> Image;

    auto load_shader(std::string_view name, std::span<const std::string_view> defines = {})
        -> Shader;

    auto load_font(std::string_view name) -> Font;

    auto load_sound(std::string_view name) -> Sound;

    auto load_custom_asset(std::string_view type_id,
                           std::string_view name,
                           const std::any&  extra_info) -> std::shared_ptr<Asset>;

    auto is_loaded(std::string_view name) const -> bool;

    void register_custom_asset_loader(std::string_view type_id, CustomAssetLoadFunc load_func);

    void unregister_custom_asset_loader(std::string_view type_id);

    void notify_asset_destroyed(std::string_view name);

  private:
    using CustomAsset = std::weak_ptr<Asset>;

    // The content manager doesn't manage loaded assets, only non-owning references are
    // stored. When assets are destroyed (because their external references have
    // vanished), they notify the content manager. The content manager then removes those
    // assets from its list.
    using ReferenceToLoadedAsset =
        std::variant<ImageImpl*, SoundImpl*, ShaderImpl*, FontImpl*, CustomAsset>;

    using MapOfLoadedAssets = StringUnorderedMap<ReferenceToLoadedAsset>;

    using CustomAssetLoaderMap = StringUnorderedMap<CustomAssetLoadFunc>;

    template <typename TBase, typename TImpl, typename TLoadFunc>
    auto lazy_load(std::string_view key, std::string_view name, const TLoadFunc& load_func);

    std::string          m_root_directory;
    std::string          m_asset_loading_prefix;
    MapOfLoadedAssets    m_loaded_assets;
    CustomAssetLoaderMap m_custom_asset_loaders;
};

template <typename TBase, typename TImpl, typename TLoadFunc>
auto ContentManager::lazy_load(std::string_view key,
                               std::string_view name,
                               const TLoadFunc& load_func)
{
    static_assert(std::is_base_of_v<Asset, TImpl>, "Type must derive from Asset");

    // TODO: we can optimize this: use key directly if asset prefix is empty
    const auto key_str = m_asset_loading_prefix + std::string{key};

    if (const auto it = m_loaded_assets.find(key_str); it != m_loaded_assets.cend())
    {
        const auto ref = std::get_if<TImpl*>(&it->second);

        if (!ref)
        {
            CER_THROW_LOGIC_ERROR("Attempting to load asset '{}' as a '{}'. However, the "
                                  "asset was previously loaded as a different type.",
                                  name,
                                  typeid(TBase).name());
        }

        // Construct object, increment reference count to impl object.
        auto obj = TBase{};

        set_impl(obj, *ref);

        return obj;
    }

    // Load fresh object, store its impl pointer in the map, but return the object.
    const auto name_str = m_asset_loading_prefix + std::string(name);
    auto       asset    = load_func(name_str);
    auto       impl     = asset.impl();

    impl->m_content_manager = this;
    impl->m_asset_name      = key_str;

    log_verbose("Loaded asset '{}'", key_str);

    m_loaded_assets.emplace(key_str, static_cast<TImpl*>(impl));

    return asset;
}
} // namespace cer::details
