// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "FileSystem.hpp"

#include "cerlib/Logging.hpp"
#include "util/InternalError.hpp"
#include "util/StringUtil.hpp"
#include "util/Util.hpp"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <fstream>
#include <gsl/narrow>
#include <gsl/util>
#include <optional>
#include <string_view>

#ifdef CERLIB_ENABLE_TESTS
#include <stb_image.h>
#include <stb_image_write.h>
#endif

#if defined(__APPLE__)
#include <CoreFoundation/CFBundle.h>
#include <TargetConditionals.h>
#elif defined(__ANDROID__)
#include "cerlib/details/Android.hpp"
#include <android/asset_manager.h>
static AAssetManager* s_cerlib_android_asset_manager;
#endif

namespace cer::details
{
void set_android_asset_manager(void* asset_manager)
{
    if (asset_manager == nullptr)
    {
        CER_THROW_INVALID_ARG_STR("No Android asset manager specified.");
    }

#ifdef __ANDROID__
    s_cerlib_android_asset_manager = static_cast<AAssetManager*>(asset_manager);
#else
    CERLIB_UNUSED(asset_manager);
#endif
}
} // namespace cer::details

static std::string s_file_loading_root_directory;

auto cer::filesystem::set_file_loading_root_directory(std::string_view prefix) -> void
{
    s_file_loading_root_directory = prefix;
}

std::string cer::filesystem::filename_extension(std::string_view filename)
{
    if (const size_t dot_idx = filename.rfind('.'); dot_idx != std::string_view::npos)
    {
        return details::to_lower_case(filename.substr(dot_idx));
    }

    return {};
}

#ifdef __ANDROID__
static AAssetManager* get_android_asset_manager()
{
    if (s_cerlib_android_asset_manager == nullptr)
    {
        CER_THROW_LOGIC_ERROR_STR("Attempting to load a file, however no Android asset "
                                  "manager (AAssetManager) is set. Please set "
                                  "one using set_android_asset_manager() first.");
    }

    return s_cerlib_android_asset_manager;
}
#endif

struct MemoryStream
{
    MemoryStream()
        : MemoryStream(nullptr, 0)
    {
    }

    explicit MemoryStream(const void* data, size_t size)
        : data(data)
        , size(static_cast<std::streamsize>(size))
    {
    }

    explicit operator bool() const
    {
        return data != nullptr;
    }

    bool is_open() const
    {
        return data != nullptr;
    }

    std::streamsize tellg() const
    {
        return size;
    }

    void seekg(const std::streampos& offset, std::ios::seekdir dir)
    {
        const auto new_pos = [&]() -> std::streamoff {
            if (dir == std::ios_base::beg)
            {
                return offset;
            }

            if (dir == std::ios_base::cur)
            {
                return pos + offset;
            }

            if (dir == std::ios_base::end)
            {
                return size - offset;
            }

            return -1;
        }();

        assert(new_pos < size);
        pos = new_pos;
    }

    void read(char* dst, std::streamsize byte_count)
    {
        assert(pos + byte_count <= size);
        std::memcpy(dst, static_cast<const char*>(data) + pos, byte_count); // NOLINT
        pos += byte_count;
    }

    const void*     data{};
    std::streamsize size{};
    std::streamoff  pos{};
};

// #endif

std::string cer::filesystem::filename_without_extension(std::string_view filename)
{
    if (const auto dot_idx = filename.rfind('.'); dot_idx != std::string_view::npos)
    {
        return std::string{filename.substr(0, dot_idx)};
    }

    return std::string{filename};
}

static void clean_path(std::string& str, std::optional<bool> with_ending_slash)
{
    std::ranges::replace(str.begin(), str.end(), '\\', '/');

    if (!str.empty())
    {
        if (with_ending_slash.value_or(false))
        {
            if (str.back() != '/')
            {
                str.push_back('/');
            }
        }
        else
        {
            if (str.back() == '/')
            {
                str.pop_back();
            }
        }
    }

    // some/path/../
    // some/path/../to

    size_t idx = str.find("../");

    while (idx != std::string::npos)
    {
        const size_t idx_of_previous = str.rfind('/', idx);
        if (idx_of_previous == std::string::npos)
        {
            break;
        }

        const size_t idx_of_previous2 = str.rfind('/', idx_of_previous - 1);
        if (idx_of_previous2 == std::string::npos)
        {
            break;
        }

        const size_t end = idx + 2;

        str.erase(idx_of_previous2, end - idx_of_previous2);

        idx = str.find("../");
    }
}

std::string cer::filesystem::parent_directory(std::string_view filename)
{
    auto result = std::string(filename);
    clean_path(result, {});

    if (const auto idx_of_last_slash = result.rfind('/'); idx_of_last_slash != std::string::npos)
    {
        result.erase(idx_of_last_slash + 1);
    }

    return result;
}

std::string cer::filesystem::combine_paths(std::string_view path1, std::string_view path2)
{
    std::string first{path1};
    std::string second{path2};
    clean_path(first, true);
    clean_path(second, {});

    return first + second;
}

cer::AssetData cer::filesystem::load_asset_data(std::string_view filename)
{
    log_verbose("Loading binary file '{}'", filename);

    std::string filename_str{};

    filename_str += s_file_loading_root_directory;

    clean_path(filename_str, true);
    filename_str += filename;
    clean_path(filename_str, false);


#if TARGET_OS_IPHONE || TARGET_OS_OSX
    std::ifstream     ifs{};
    const std::string ext           = filename_extension(filename_str);
    const std::string resource_name = filename_without_extension(filename_str);

    CFStringRef resource_name_ref{};
    CFStringRef resource_type_ref{};
    CFURLRef    asset_url{};

    const auto _ = gsl::finally([&] {
        if (resource_type_ref != nullptr)
        {
            CFRelease(resource_type_ref);
        }

        if (resource_name_ref != nullptr)
        {
            CFRelease(resource_name_ref);
        }

        if (asset_url != nullptr)
        {
            CFRelease(asset_url);
        }
    });

    resource_name_ref = CFStringCreateWithCString(kCFAllocatorDefault,
                                                  resource_name.c_str(),
                                                  kCFStringEncodingMacRoman);

    assert(resource_name_ref && "Failed to create resource_name_ref");

    resource_type_ref =
        CFStringCreateWithCString(kCFAllocatorDefault, ext.c_str(), kCFStringEncodingMacRoman);

    assert(resource_type_ref && "Failed to create resource_type_ref");

    asset_url = CFBundleCopyResourceURL(CFBundleGetMainBundle(),
                                        resource_name_ref,
                                        resource_type_ref,
                                        nullptr);

    if (asset_url != nullptr)
    {
        std::array<UInt8, 512> full_asset_path{};
        CFURLGetFileSystemRepresentation(asset_url,
                                         static_cast<Boolean>(1),
                                         full_asset_path.data(),
                                         sizeof(full_asset_path));

        const std::string_view full_asset_path_str{
            reinterpret_cast<const char*>(full_asset_path.data())};

        if (!full_asset_path_str.empty())
        {
            ifs = std::ifstream(full_asset_path_str.data(), std::ios::binary | std::ios::ate);
        }
        else
        {
            log_verbose("Full asset path was empty; skipping");
        }
    }

    if (!ifs.is_open())
    {
        log_verbose("Falling back to file '{}'", filename_str);
        ifs = std::ifstream{filename_str.c_str(), std::ios::binary | std::ios::ate};

        if (ifs)
        {
            log_verbose("Found the file");
        }
        else
        {
            log_verbose("Did not find the file");
        }
    }

#elif defined(__ANDROID__)
    const auto   asset_manager = get_android_asset_manager();
    MemoryStream ifs;
    const auto   asset_handle =
        AAssetManager_open(asset_manager, filename_str.c_str(), AASSET_MODE_BUFFER);

    const auto _ = gsl::finally([&] { AAsset_close(asset_handle); });

    if (asset_handle != nullptr)
    {
        ifs = MemoryStream{AAsset_getBuffer(asset_handle),
                           static_cast<size_t>(AAsset_getLength64(asset_handle))};
    }
#else
    std::ifstream ifs{filename_str.c_str(), std::ios::binary | std::ios::ate};
#endif

    if (!ifs.is_open())
    {
        if (filename == filename_str)
        {
            CER_THROW_RUNTIME_ERROR("Failed to open file '{}' for reading.", filename);
        }

        CER_THROW_RUNTIME_ERROR("Failed to open file '{}' for reading ({}).",
                                filename,
                                filename_str);
    }

    const size_t data_size = ifs.tellg();
    auto         data      = std::make_unique<std::byte[]>(data_size);

    ifs.seekg(0, std::ios::beg);
    ifs.read(reinterpret_cast<char*>(data.get()), static_cast<std::streamsize>(data_size));

    return {
        .data = std::move(data),
        .size = data_size,
    };
}

std::vector<std::byte> cer::filesystem::load_file_data_from_disk(std::string_view filename)
{
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__) || TARGET_OS_IPHONE
    CERLIB_UNUSED(filename);
    CER_THROW_RUNTIME_ERROR_STR("Loading files from disk is not supported on the current system.");
#else
    std::ifstream ifs{std::string{filename}, std::ios::binary | std::ios::ate};
    if (!ifs.is_open())
    {
        CER_THROW_RUNTIME_ERROR("Failed to open file '{}' for reading.", filename);
    }

    const size_t file_size = ifs.tellg();
    ifs.seekg(0, std::ios::beg);

    std::vector<std::byte> data;
    data.resize(file_size);
    ifs.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(file_size));

    return data;
#endif
}

auto cer::filesystem::write_text_to_file_on_disk(std::string_view filename,
                                                 std::string_view contents) -> void
{
    std::ofstream ofs{std::string(filename)};

    if (!ofs)
    {
        CER_THROW_RUNTIME_ERROR("Failed to open file '{}' for writing.", filename);
    }

    ofs << contents;
}

#ifdef CERLIB_ENABLE_TESTS
std::vector<std::byte> cer::filesystem::decode_image_data_from_file_on_disk(
    std::string_view filename)
{
    std::vector<std::byte> file_data = load_file_data_from_disk(filename);
    const std::string      filename_str{filename};

    int      width    = 0;
    int      height   = 0;
    int      channels = 0;
    stbi_uc* data     = stbi_load(filename_str.c_str(), &width, &height, &channels, 4);

    if (data == nullptr)
    {
        CER_THROW_RUNTIME_ERROR_STR("Failed to load the image file.");
    }

    const auto _ = gsl::finally([data] { stbi_image_free(data); });

    const std::span src_span{reinterpret_cast<const std::byte*>(data),
                             gsl::narrow<size_t>(width) * gsl::narrow<size_t>(height) *
                                 gsl::narrow<size_t>(channels)};

    std::vector<std::byte> result{src_span.size_bytes()};

    std::ranges::copy(src_span, result.begin());

    return result;
}

auto cer::filesystem::encode_image_data_to_file_on_disk(std::string_view           filename,
                                                        std::span<const std::byte> raw_image_data,
                                                        uint32_t                   width,
                                                        uint32_t                   height) -> void
{
    const std::string filename_str{filename};

    if (const int result = stbi_write_png(filename_str.c_str(),
                                          gsl::narrow<int>(width),
                                          gsl::narrow<int>(height),
                                          4,
                                          raw_image_data.data(),
                                          gsl::narrow<int>(width * 4));
        result == 0)
    {
        CER_THROW_RUNTIME_ERROR_STR("Failed to write the image data to disk.");
    }
}
#endif
