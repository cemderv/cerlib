// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "cerlib/Image.hpp"
#include "util/SmallVector.hpp"
#include <cstdint>
#include <optional>
#include <span>

namespace cer
{
enum class ImageFormat;
}

namespace cer::dds
{
struct DDSMipmap
{
    std::span<const std::byte> data;
};

struct DDSFace
{
    SmallVector<DDSMipmap, 8> mipmaps;
};

struct DDSImage
{
    uint32_t                width{};
    uint32_t                height{};
    uint32_t                depth{};
    ImageFormat             format{0};
    SmallVector<DDSFace, 2> faces{};
};

const void* dds_image_data_upload(const DDSImage& dds_image, uint32_t array_index, uint32_t mipmap);

std::optional<DDSImage> load(std::span<const std::byte> memory);
} // namespace cer::dds
