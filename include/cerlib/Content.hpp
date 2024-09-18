// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <cerlib/Export.hpp>

#include <any>
#include <functional>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>

namespace cer
{
class Image;
class Shader;
class Font;
class Sound;

namespace details
{
class ContentManager;
}

/**
 * Represents data of a loaded asset.
 *
 * @ingroup Content
 */
struct AssetData
{
    std::span<const std::byte> as_span() const
    {
        return {data.get(), size};
    }

    std::string_view as_string_view() const
    {
        return {reinterpret_cast<const char*>(data.get()), size};
    }

    std::unique_ptr<std::byte[]> data;
    size_t                       size{};
};

/**
 * Represents the base class of asset types.
 *
 * A type that shall be compatible with `cer::load_custom_asset()` must derive from this class.
 * For further instructions on how to load custom assets, see the documentation of
 * `cer::load_custom_asset()`.
 *
 * @ingroup Content
 */
class CERLIB_API Asset
{
    friend details::ContentManager;

  protected:
    Asset();

  public:
    Asset(const Asset&) = default;

    Asset& operator=(const Asset&) = default;

    Asset(Asset&&) noexcept = default;

    Asset& operator=(Asset&&) noexcept = default;

    virtual ~Asset() noexcept;

    std::string_view asset_name() const;

  private:
    details::ContentManager* m_content_manager{};
    std::string              m_asset_name;
};

/**
 * Represents a function that loads a custom asset.
 *
 * Please see register_custom_asset_loader() for details.
 *
 * @param name The name of the asset that is being loaded.
 * @param data The raw data of the asset, as it is stored on disk.
 * @param extra_info Optional extra information that was passed to the `cer::load_custom_asset()`
 * call. **This value has no effect on how the asset is cached after it is loaded.**
 *
 * @ingroup Content
 */
using CustomAssetLoadFunc = std::function<std::shared_ptr<Asset>(
    std::string_view name, AssetData& data, const std::any& extra_info)>;

/**
 * Sets an optional prefix that should be prepended to asset names when loading.
 *
 * @param prefix The prefix string to use. May be empty.
 * If not empty, all backslashes are converted to forward slashes, and a forward slash
 * is appended.
 *
 * Example:
 * @code{.cpp}
 * cer::set_asset_loading_prefix("MySpecialFolder/Folder2/");
 *
 * const auto image = cer::load_image("MyImage.png");
 *
 * # ^ Same as cer::load_image("MySpecialFolder/Folder2/MyImage.png")
 * @endcode
 *
 * @attention This affects how assets are cached. The content manager
 * remembers a loaded asset by its full path (which includes this prefix).
 *
 * @ingroup Content
 */
CERLIB_API void set_asset_loading_prefix(std::string_view prefix);

/**
 * Gets the prefix that should be prepended to asset names when loading. May be
 * empty. For further information, see `set_asset_loading_prefix()`.
 *
 * @ingroup Content
 */
CERLIB_API std::string asset_loading_prefix();

/**
 * Lazily loads an Image object from the storage.
 *
 * @param name The name of the asset in the storage.
 * @param generate_mipmaps If true, loads a version of the image with mipmaps generated.
 *
 * @throw std::runtime_error If the asset does not exist or could not be read or
 * loaded.
 *
 * @ingroup Content
 */
CERLIB_API Image load_image(std::string_view name, bool generate_mipmaps = false);

/**
 * Lazily loads a Shader object from the storage.
 *
 * @param name The name of the asset in the storage.
 * @param defines Reserved; currently has no effect.
 *
 * @throw std::runtime_error If the asset does not exist or could not be read or
 * loaded.
 *
 * @ingroup Content
 */
CERLIB_API Shader load_shader(std::string_view                  name,
                              std::span<const std::string_view> defines = {});

/**
 * Lazily loads a Font object from the storage.
 *
 * @param name The name of the asset in the storage.
 *
 * @throw std::runtime_error If the asset does not exist or could not be read or
 * loaded.
 *
 * @ingroup Content
 */
CERLIB_API Font load_font(std::string_view name);

/**
 * Lazily loads a Sound object from the storage.
 *
 * @param name The name of the asset in the storage.
 *
 * @throw std::runtime_error If the asset does not exist or could not be read or
 * loaded.
 *
 * @ingroup Content
 */
CERLIB_API Sound load_sound(std::string_view name);

/**
 * Registers a function as a custom asset loader for a specific type ID.
 *
 * @param type_id The ID of the custom asset. May be chosen freely, but must be unique.
 * @param load_func The function that is responsible for loading the custom asset.
 *
 * @throw std::invalid_argument If a loader for the specified typeId is already
 * registered.
 *
 * @ingroup Content
 */
CERLIB_API void register_custom_asset_loader(std::string_view    type_id,
                                             CustomAssetLoadFunc load_func);

/**
 * Removed the custom asset loader for a specific type ID.
 *
 * @param type_id The ID of the custom asset.
 *
 * @ingroup Content
 */
CERLIB_API void unregister_custom_asset_loader(std::string_view type_id);

/**
 * Registers a function as a custom asset loader for a specific type.
 *
 * This is a convenience function for RegisterCustomAssetLoader().
 * It forwards the C++ typeid() information as the `typeId`.
 *
 * @tparam T The type of the custom asset.
 * @param load_func The function that is responsible for loading the custom asset.
 *
 * @ingroup Content
 */
template <typename T>
static void register_custom_asset_loader_for_type(CustomAssetLoadFunc load_func)
{
    static_assert(std::is_base_of_v<Asset, T>,
                  "The specified type must derive from the CustomAsset class.");

    register_custom_asset_loader(typeid(T).name(), std::move(load_func));
}

/**
 * Lazily loads a custom asset object from the storage.
 *
 * @param type_id The ID of the custom asset to load. This must correspond to `typeId`
 * that was passed to RegisterCustomAssetLoader().
 * @param name The name of the asset in the storage.
 * @param extra_info Optional extra information that is passed to the asset loader. Has
 * no effect on how the asset is cached. This means that if an asset with the same
 * `typeId` and `name`, but with different `extraInfo` values is loaded, the first asset
 * that was loaded by such a call is returned.
 *
 * @return The loaded asset. If the asset was previously loaded, its reference count is
 * incremented. The content manager will not store a reference to the asset.
 *
 * @ingroup Content
 */
CERLIB_API std::shared_ptr<Asset> load_custom_asset(std::string_view type_id,
                                                    std::string_view name,
                                                    const std::any&  extra_info);

/**
 * Registers a function as a custom asset loader for a specific type.
 *
 * This is a convenience function for RegisterCustomAssetLoader().
 * It forwards the C++ typeid() information as the `type_id`.
 *
 * @tparam T The type of the custom asset.
 * @param name The name of the asset in the storage.
 * @param extra_info Optional extra information that is passed to the asset loader. Has
 * no effect on how the asset is cached. This means that if an asset with the same
 * `type_id` and `name`, but with different `extra_info` values is loaded, the first
 * asset that was loaded by such a call is returned.
 *
 * @ingroup Content
 */
template <typename T>
static std::shared_ptr<T> load_custom_asset_of_type(std::string_view name,
                                                    const std::any&  extra_info)
    requires(std::is_base_of_v<Asset, T>)
{
    const std::shared_ptr<Asset> asset = load_custom_asset(typeid(T).name(), name, extra_info);

    std::shared_ptr<T> asset_type = std::dynamic_pointer_cast<T>(asset);

    if (!asset_type)
    {
        throw std::invalid_argument("The loaded asset type differs from the desired type T.");
    }

    return asset_type;
}

/**
 * Gets a value indicating whether an asset with a specific name is currently
 * loaded.
 *
 * @param name The name of the asset in the storage.
 *
 * @ingroup Content
 */
CERLIB_API bool is_asset_loaded(std::string_view name);
} // namespace cer
