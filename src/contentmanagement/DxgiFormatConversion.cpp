// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "DxgiFormatConversion.hpp"

#include "cerlib/Image.hpp"

namespace cer
{
std::optional<ImageFormat> from_dxgi_format(DXGI_FORMAT dxgi_format)
{
    switch (dxgi_format)
    {
        case DXGI_FORMAT_R8G8B8A8_UNORM: return ImageFormat::R8G8B8A8_UNorm;
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: return ImageFormat::R8G8B8A8_Srgb;
        default: return {};
    };
}
} // namespace cer
