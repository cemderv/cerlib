// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "cerlib/Image.hpp"
#include <cerlib/List.hpp>
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
    std::span<const std::byte> data_span;
};

struct DDSFace
{
    List<DDSMipmap, 8> mipmaps;
};

struct DDSImage
{
    uint32_t         width{};
    uint32_t         height{};
    uint32_t         depth{};
    ImageFormat      format{};
    List<DDSFace, 2> faces{};
};

auto load(std::span<const std::byte> memory) -> std::optional<DDSImage>;
} // namespace cer::dds
