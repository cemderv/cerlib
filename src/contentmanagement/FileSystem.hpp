// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "cerlib/Content.hpp"
#include "util/InternalExport.hpp"

#include <string>
#include <string_view>
#include <vector>

auto set_android_asset_manager(void* asset_manager) -> void;

namespace cer
{
enum class ImageFileFormat;
}

namespace cer::filesystem
{
/**
 *  Sets a string that is prepended to every path when loading resources from
 * files.
 *
 * @param prefix The string to prepend. By default, no string is prepended to the path.
 */
void set_file_loading_root_directory(std::string_view prefix);

AssetData load_asset_data(std::string_view filename);

std::string filename_extension(std::string_view filename);

std::string filename_without_extension(std::string_view filename);

std::string parent_directory(std::string_view filename);

std::string combine_paths(std::string_view path1, std::string_view path2);

/**
 * Loads a binary file from disk, meaning the disk on desktop platforms.
 * On non-desktop platforms, calling this will throw an exception.
 */
std::vector<std::byte> load_file_data_from_disk(std::string_view filename);

void write_text_to_file_on_disk(std::string_view filename, std::string_view contents);

#ifdef CERLIB_ENABLE_TESTS
CERLIB_API_INTERNAL std::vector<std::byte> decode_image_data_from_file_on_disk(
    std::string_view filename);

CERLIB_API_INTERNAL void encode_image_data_to_file_on_disk(
    std::string_view           filename,
    std::span<const std::byte> raw_image_data,
    uint32_t                   width,
    uint32_t                   height);
#endif
} // namespace cer::filesystem
