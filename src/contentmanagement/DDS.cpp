// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "DDS.hpp"

#include "DxgiFormatConversion.hpp"
#include "cerlib/Math.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>

namespace cer::dds
{
static constexpr uint32_t D3D11_REQ_TEXTURE1D_ARRAY_AXIS_DIMENSION = 2048; // NOLINT
static constexpr uint32_t D3D11_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION = 2048; // NOLINT
static constexpr uint32_t D3D11_REQ_TEXTURE1D_U_DIMENSION          = 16384; // NOLINT

static constexpr uint32_t D3D11_RESOURCE_DIMENSION_TEXTURE1D = 2; // NOLINT
static constexpr uint32_t D3D11_RESOURCE_DIMENSION_TEXTURE2D = 3; // NOLINT
static constexpr uint32_t D3D11_RESOURCE_DIMENSION_TEXTURE3D = 4; // NOLINT

static constexpr uint32_t D3D11_RESOURCE_MISC_TEXTURECUBE        = 0x4L; // NOLINT
static constexpr uint32_t D3D11_REQ_MIP_LEVELS                   = 15; // NOLINT
static constexpr uint32_t D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION   = 16384; // NOLINT
static constexpr uint32_t D3D11_REQ_TEXTURE3D_U_V_OR_W_DIMENSION = 2048; // NOLINT

static constexpr auto make_fourcc(char ch0, char ch1, char ch2, char ch3) -> uint32_t
{
    return uint32_t(uint8_t(ch0)) | uint32_t(uint8_t(ch1)) << 8 | uint32_t(uint8_t(ch2)) << 16 |
           uint32_t(uint8_t(ch3)) << 24;
}

// #pragma pack(push, 1)

static constexpr int32_t dds_magic = 0x20534444; // NOLINT

struct DdsPixelformat
{
    uint32_t size{};
    uint32_t flags{};
    uint32_t four_cc{};
    uint32_t rgb_bit_count{};
    uint32_t r_bit_mask{};
    uint32_t g_bit_mask{};
    uint32_t b_bit_mask{};
    uint32_t a_bit_mask{};
};

static constexpr uint32_t dds_fourcc              = 0x00000004; // NOLINT
static constexpr uint32_t dds_rgb                 = 0x00000040; // NOLINT
static constexpr uint32_t dds_luminance           = 0x00020000; // NOLINT
static constexpr uint32_t dds_alpha               = 0x00000002; // NOLINT
static constexpr uint32_t dds_header_flags_volume = 0x00800000; // NOLINT
static constexpr uint32_t dds_height              = 0x00000002; // NOLINT
static constexpr uint32_t dds_cubemap             = 0x00000200; // NOLINT

struct DdsHeader
{
    uint32_t                 size{};
    uint32_t                 flags{};
    uint32_t                 height{};
    uint32_t                 width{};
    uint32_t                 pitch_or_linear_size{};
    uint32_t                 depth{};
    uint32_t                 mip_map_count{};
    std::array<uint32_t, 11> reserved1{};
    DdsPixelformat           ddspf{};
    uint32_t                 caps{};
    uint32_t                 caps2{};
    uint32_t                 caps3{};
    uint32_t                 caps4{};
    uint32_t                 reserved2{};
};

struct DdsHeaderDxT10
{
    DXGI_FORMAT dxgi_format{DXGI_FORMAT_UNKNOWN};
    uint32_t    resource_dimension{};
    uint32_t    misc_flag{};
    uint32_t    array_size{};
    uint32_t    reserved{};
};

// #pragma pack(pop)

static auto bits_per_pixel(DXGI_FORMAT fmt) -> size_t
{
    switch (fmt)
    {
        case DXGI_FORMAT_R32G32B32A32_TYPELESS:
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
        case DXGI_FORMAT_R32G32B32A32_UINT:
        case DXGI_FORMAT_R32G32B32A32_SINT: return 128;

        case DXGI_FORMAT_R32G32B32_TYPELESS:
        case DXGI_FORMAT_R32G32B32_FLOAT:
        case DXGI_FORMAT_R32G32B32_UINT:
        case DXGI_FORMAT_R32G32B32_SINT: return 96;

        case DXGI_FORMAT_R16G16B16A16_TYPELESS:
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
        case DXGI_FORMAT_R16G16B16A16_UNORM:
        case DXGI_FORMAT_R16G16B16A16_UINT:
        case DXGI_FORMAT_R16G16B16A16_SNORM:
        case DXGI_FORMAT_R16G16B16A16_SINT:
        case DXGI_FORMAT_R32G32_TYPELESS:
        case DXGI_FORMAT_R32G32_FLOAT:
        case DXGI_FORMAT_R32G32_UINT:
        case DXGI_FORMAT_R32G32_SINT:
        case DXGI_FORMAT_R32G8X24_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
        case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT: return 64;

        case DXGI_FORMAT_R10G10B10A2_TYPELESS:
        case DXGI_FORMAT_R10G10B10A2_UNORM:
        case DXGI_FORMAT_R10G10B10A2_UINT:
        case DXGI_FORMAT_R11G11B10_FLOAT:
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_R8G8B8A8_UINT:
        case DXGI_FORMAT_R8G8B8A8_SNORM:
        case DXGI_FORMAT_R8G8B8A8_SINT:
        case DXGI_FORMAT_R16G16_TYPELESS:
        case DXGI_FORMAT_R16G16_FLOAT:
        case DXGI_FORMAT_R16G16_UNORM:
        case DXGI_FORMAT_R16G16_UINT:
        case DXGI_FORMAT_R16G16_SNORM:
        case DXGI_FORMAT_R16G16_SINT:
        case DXGI_FORMAT_R32_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT:
        case DXGI_FORMAT_R32_FLOAT:
        case DXGI_FORMAT_R32_UINT:
        case DXGI_FORMAT_R32_SINT:
        case DXGI_FORMAT_R24G8_TYPELESS:
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
        case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
        case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
        case DXGI_FORMAT_R8G8_B8G8_UNORM:
        case DXGI_FORMAT_G8R8_G8B8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8X8_UNORM:
        case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8X8_TYPELESS:
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB: return 32;

        case DXGI_FORMAT_R8G8_TYPELESS:
        case DXGI_FORMAT_R8G8_UNORM:
        case DXGI_FORMAT_R8G8_UINT:
        case DXGI_FORMAT_R8G8_SNORM:
        case DXGI_FORMAT_R8G8_SINT:
        case DXGI_FORMAT_R16_TYPELESS:
        case DXGI_FORMAT_R16_FLOAT:
        case DXGI_FORMAT_D16_UNORM:
        case DXGI_FORMAT_R16_UNORM:
        case DXGI_FORMAT_R16_UINT:
        case DXGI_FORMAT_R16_SNORM:
        case DXGI_FORMAT_R16_SINT:
        case DXGI_FORMAT_B5G6R5_UNORM:
        case DXGI_FORMAT_B5G5R5A1_UNORM:
        case DXGI_FORMAT_B4G4R4A4_UNORM: return 16;

        case DXGI_FORMAT_R8_TYPELESS:
        case DXGI_FORMAT_R8_UNORM:
        case DXGI_FORMAT_R8_UINT:
        case DXGI_FORMAT_R8_SNORM:
        case DXGI_FORMAT_R8_SINT:
        case DXGI_FORMAT_A8_UNORM: return 8;

        case DXGI_FORMAT_R1_UNORM: return 1;

        case DXGI_FORMAT_BC1_TYPELESS:
        case DXGI_FORMAT_BC1_UNORM:
        case DXGI_FORMAT_BC1_UNORM_SRGB:
        case DXGI_FORMAT_BC4_TYPELESS:
        case DXGI_FORMAT_BC4_UNORM:
        case DXGI_FORMAT_BC4_SNORM: return 4;

        case DXGI_FORMAT_BC2_TYPELESS:
        case DXGI_FORMAT_BC2_UNORM:
        case DXGI_FORMAT_BC2_UNORM_SRGB:
        case DXGI_FORMAT_BC3_TYPELESS:
        case DXGI_FORMAT_BC3_UNORM:
        case DXGI_FORMAT_BC3_UNORM_SRGB:
        case DXGI_FORMAT_BC5_TYPELESS:
        case DXGI_FORMAT_BC5_UNORM:
        case DXGI_FORMAT_BC5_SNORM:
        case DXGI_FORMAT_BC6H_TYPELESS:
        case DXGI_FORMAT_BC6H_UF16:
        case DXGI_FORMAT_BC6H_SF16:
        case DXGI_FORMAT_BC7_TYPELESS:
        case DXGI_FORMAT_BC7_UNORM:
        case DXGI_FORMAT_BC7_UNORM_SRGB: return 8;

        default: return 0;
    }
}

static void get_surface_info(size_t      width,
                             size_t      height,
                             DXGI_FORMAT fmt,
                             size_t*     out_num_bytes,
                             size_t*     out_row_bytes,
                             size_t*     out_num_rows)
{
    size_t num_bytes{};
    size_t row_bytes{};
    size_t num_rows{};

    bool   bc{};
    bool   packed{};
    size_t bcnum_bytes_per_block{};

    switch (fmt)
    {
        case DXGI_FORMAT_BC1_TYPELESS:
        case DXGI_FORMAT_BC1_UNORM:
        case DXGI_FORMAT_BC1_UNORM_SRGB:
        case DXGI_FORMAT_BC4_TYPELESS:
        case DXGI_FORMAT_BC4_UNORM:
        case DXGI_FORMAT_BC4_SNORM:
            bc                    = true;
            bcnum_bytes_per_block = 8;
            break;

        case DXGI_FORMAT_BC2_TYPELESS:
        case DXGI_FORMAT_BC2_UNORM:
        case DXGI_FORMAT_BC2_UNORM_SRGB:
        case DXGI_FORMAT_BC3_TYPELESS:
        case DXGI_FORMAT_BC3_UNORM:
        case DXGI_FORMAT_BC3_UNORM_SRGB:
        case DXGI_FORMAT_BC5_TYPELESS:
        case DXGI_FORMAT_BC5_UNORM:
        case DXGI_FORMAT_BC5_SNORM:
        case DXGI_FORMAT_BC6H_TYPELESS:
        case DXGI_FORMAT_BC6H_UF16:
        case DXGI_FORMAT_BC6H_SF16:
        case DXGI_FORMAT_BC7_TYPELESS:
        case DXGI_FORMAT_BC7_UNORM:
        case DXGI_FORMAT_BC7_UNORM_SRGB:
            bc                    = true;
            bcnum_bytes_per_block = 16;
            break;

        case DXGI_FORMAT_R8G8_B8G8_UNORM:
        case DXGI_FORMAT_G8R8_G8B8_UNORM: packed = true; break;

        default: break;
    }

    if (bc)
    {
        size_t num_blocks_wide = 0;
        if (width > 0)
        {
            num_blocks_wide = std::max<size_t>(1, (width + 3) / 4);
        }

        size_t num_blocks_high = 0;
        if (height > 0)
        {
            num_blocks_high = std::max<size_t>(1, (height + 3) / 4);
        }

        row_bytes = num_blocks_wide * bcnum_bytes_per_block;
        num_rows  = num_blocks_high;
    }
    else if (packed)
    {
        row_bytes = ((width + 1) >> 1) * 4;
        num_rows  = height;
    }
    else
    {
        size_t bpp = bits_per_pixel(fmt);
        row_bytes  = (width * bpp + 7) / 8; // Round up to the nearest byte.
        num_rows   = height;
    }

    num_bytes = row_bytes * num_rows;

    if (out_num_bytes != nullptr)
    {
        *out_num_bytes = num_bytes;
    }

    if (out_row_bytes != nullptr)
    {
        *out_row_bytes = row_bytes;
    }

    if (out_num_rows != nullptr)
    {
        *out_num_rows = num_rows;
    }
}

static constexpr auto is_bitmask(
    const DdsPixelformat& ddpf, uint32_t r, uint32_t g, uint32_t b, uint32_t a) -> bool
{
    return ddpf.r_bit_mask == r && ddpf.g_bit_mask == g && ddpf.b_bit_mask == b &&
           ddpf.a_bit_mask == a;
}

static auto get_dxgi_format(const DdsPixelformat& ddpf) -> DXGI_FORMAT
{
    if ((ddpf.flags & dds_rgb) != 0u)
    {
        switch (ddpf.rgb_bit_count)
        {
            case 32: {
                if (is_bitmask(ddpf, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000))
                {
                    return DXGI_FORMAT_R8G8B8A8_UNORM;
                }

                if (is_bitmask(ddpf, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000))
                {
                    return DXGI_FORMAT_B8G8R8A8_UNORM;
                }

                if (is_bitmask(ddpf, 0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000))
                {
                    return DXGI_FORMAT_B8G8R8X8_UNORM;
                }

                if (is_bitmask(ddpf, 0x3ff00000, 0x000ffc00, 0x000003ff, 0xc0000000))
                {
                    return DXGI_FORMAT_R10G10B10A2_UNORM;
                }

                if (is_bitmask(ddpf, 0x0000ffff, 0xffff0000, 0x00000000, 0x00000000))
                {
                    return DXGI_FORMAT_R16G16_UNORM;
                }

                if (is_bitmask(ddpf, 0xffffffff, 0x00000000, 0x00000000, 0x00000000))
                {
                    return DXGI_FORMAT_R32_FLOAT;
                }

                break;
            }

            case 24: break;

            case 16: {
                if (is_bitmask(ddpf, 0x7c00, 0x03e0, 0x001f, 0x8000))
                {
                    return DXGI_FORMAT_B5G5R5A1_UNORM;
                }

                if (is_bitmask(ddpf, 0xf800, 0x07e0, 0x001f, 0x0000))
                {
                    return DXGI_FORMAT_B5G6R5_UNORM;
                }

                if (is_bitmask(ddpf, 0x0f00, 0x00f0, 0x000f, 0xf000))
                {
                    return DXGI_FORMAT_B4G4R4A4_UNORM;
                }

                break;
            }
            default: break;
        }
    }
    else if ((ddpf.flags & dds_luminance) != 0u)
    {
        if (8 == ddpf.rgb_bit_count)
        {
            if (is_bitmask(ddpf, 0x000000ff, 0x00000000, 0x00000000, 0x00000000))
            {
                return DXGI_FORMAT_R8_UNORM;
            }
        }

        if (16 == ddpf.rgb_bit_count)
        {
            if (is_bitmask(ddpf, 0x0000ffff, 0x00000000, 0x00000000, 0x00000000))
            {
                return DXGI_FORMAT_R16_UNORM;
            }

            if (is_bitmask(ddpf, 0x000000ff, 0x00000000, 0x00000000, 0x0000ff00))
            {
                return DXGI_FORMAT_R8G8_UNORM;
            }
        }
    }
    else if ((ddpf.flags & dds_alpha) != 0u)
    {
        if (8 == ddpf.rgb_bit_count)
        {
            return DXGI_FORMAT_A8_UNORM;
        }
    }
    else if ((ddpf.flags & dds_fourcc) != 0u)
    {
        if (make_fourcc('D', 'X', 'T', '1') == ddpf.four_cc)
        {
            return DXGI_FORMAT_BC1_UNORM;
        }
        if (make_fourcc('D', 'X', 'T', '3') == ddpf.four_cc)
        {
            return DXGI_FORMAT_BC2_UNORM;
        }
        if (make_fourcc('D', 'X', 'T', '5') == ddpf.four_cc)
        {
            return DXGI_FORMAT_BC3_UNORM;
        }
        if (make_fourcc('D', 'X', 'T', '2') == ddpf.four_cc)
        {
            return DXGI_FORMAT_BC2_UNORM;
        }
        if (make_fourcc('D', 'X', 'T', '4') == ddpf.four_cc)
        {
            return DXGI_FORMAT_BC3_UNORM;
        }
        if (make_fourcc('A', 'T', 'I', '1') == ddpf.four_cc)
        {
            return DXGI_FORMAT_BC4_UNORM;
        }
        if (make_fourcc('B', 'C', '4', 'U') == ddpf.four_cc)
        {
            return DXGI_FORMAT_BC4_UNORM;
        }
        if (make_fourcc('B', 'C', '4', 'S') == ddpf.four_cc)
        {
            return DXGI_FORMAT_BC4_SNORM;
        }
        if (make_fourcc('A', 'T', 'I', '2') == ddpf.four_cc)
        {
            return DXGI_FORMAT_BC5_UNORM;
        }
        if (make_fourcc('B', 'C', '5', 'U') == ddpf.four_cc)
        {
            return DXGI_FORMAT_BC5_UNORM;
        }
        if (make_fourcc('B', 'C', '5', 'S') == ddpf.four_cc)
        {
            return DXGI_FORMAT_BC5_SNORM;
        }
        if (make_fourcc('R', 'G', 'B', 'G') == ddpf.four_cc)
        {
            return DXGI_FORMAT_R8G8_B8G8_UNORM;
        }
        if (make_fourcc('G', 'R', 'G', 'B') == ddpf.four_cc)
        {
            return DXGI_FORMAT_G8R8_G8B8_UNORM;
        }

        switch (ddpf.four_cc)
        {
            case 36: return DXGI_FORMAT_R16G16B16A16_UNORM;
            case 110: return DXGI_FORMAT_R16G16B16A16_SNORM;
            case 111: return DXGI_FORMAT_R16_FLOAT;
            case 112: return DXGI_FORMAT_R16G16_FLOAT;
            case 113: return DXGI_FORMAT_R16G16B16A16_FLOAT;
            case 114: return DXGI_FORMAT_R32_FLOAT;
            case 115: return DXGI_FORMAT_R32G32_FLOAT;
            case 116: return DXGI_FORMAT_R32G32B32A32_FLOAT;
            default: break;
        }
    }

    return DXGI_FORMAT_UNKNOWN;
}

static void fill_init_data(size_t                     width,
                           size_t                     height,
                           size_t                     depth,
                           size_t                     mip_count,
                           size_t                     array_size,
                           DXGI_FORMAT                format,
                           std::span<const std::byte> bit_data,
                           DDSImage&                  init_data)
{
    size_t num_bytes{};
    size_t row_bytes{};
    size_t num_rows{};

    std::span src_bits{bit_data.data(), bit_data.size()};

    for (size_t face_index = 0; face_index < array_size; ++face_index)
    {
        size_t w = width;
        size_t h = height;
        size_t d = depth;

        for (size_t mip_index = 0; mip_index < mip_count; ++mip_index)
        {
            get_surface_info(w, h, format, &num_bytes, &row_bytes, &num_rows);

            auto& face    = init_data.faces.at(face_index);
            auto& mip     = face.mipmaps.at(mip_index);
            mip.data_span = src_bits.subspan(0, num_bytes);

            src_bits = src_bits.subspan(num_bytes * d);

            w = max(w >> 1, size_t(1));
            h = max(h >> 1, size_t(1));
            d = max(d >> 1, size_t(1));
        }
    }
}

static auto create_image_from_dds(const DdsHeader& header, std::span<const std::byte> bit_data)
    -> DDSImage
{
    DDSImage image{};
    image.width  = header.width;
    image.height = header.height;
    image.depth  = header.depth;

    uint32_t    res_dim{};
    size_t      array_size{1};
    DXGI_FORMAT dxgi_format{DXGI_FORMAT_UNKNOWN};

    const size_t mip_count = header.mip_map_count == 0 ? 1 : header.mip_map_count;

    if ((header.ddspf.flags & dds_fourcc) != 0u &&
        make_fourcc('D', 'X', '1', '0') == header.ddspf.four_cc)
    {
        const DdsHeaderDxT10* d3d10ext = reinterpret_cast<const DdsHeaderDxT10*>(
            reinterpret_cast<const char*>(&header) + sizeof(DdsHeader)); // NOLINT

        array_size = d3d10ext->array_size;

        if (array_size == 0)
        {
            throw std::runtime_error{fmt::format("DDS has invalid array size ({})", array_size)};
        }

        if (bits_per_pixel(d3d10ext->dxgi_format) == 0)
        {
            throw std::runtime_error{"DDS has invalid format"};
        }

        dxgi_format = d3d10ext->dxgi_format;

        switch (d3d10ext->resource_dimension)
        {
            case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
                // D3DX writes 1D textures with a fixed Height of 1.
                if ((header.flags & dds_height) != 0u && image.height != 1)
                {
                    throw std::runtime_error{
                        fmt::format("DDS has invalid 1D image height ({})", image.height)};
                }

                image.height = image.depth = 1;
                break;

            case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
                if ((d3d10ext->misc_flag & D3D11_RESOURCE_MISC_TEXTURECUBE) != 0u)
                {
                    throw std::runtime_error{"Cubemaps are not supported"};
                }

                image.depth = 1;
                break;

            case D3D11_RESOURCE_DIMENSION_TEXTURE3D: {
                if ((header.flags & dds_header_flags_volume) == 0u)
                {
                    throw std::runtime_error{"DDS has invalid 3D image flags"};
                }

                if (array_size > 1)
                {
                    throw std::runtime_error{
                        fmt::format("DDS has invalid array size for 3D image ({})", array_size)};
                }

                break;
            }

            default:
                throw std::runtime_error{
                    fmt::format("DDS has invalid image type ({})", d3d10ext->resource_dimension)};
        }

        res_dim = d3d10ext->resource_dimension;
    }
    else
    {
        dxgi_format = get_dxgi_format(header.ddspf);

        if (dxgi_format == DXGI_FORMAT_UNKNOWN)
        {
            throw std::runtime_error{"DDS has invalid image format"};
        }

        if ((header.flags & dds_header_flags_volume) != 0u)
        {
            res_dim = D3D11_RESOURCE_DIMENSION_TEXTURE3D;
        }
        else
        {
            if ((header.caps2 & dds_cubemap) != 0u)
            {
                throw std::runtime_error{"Cubemaps are not supported"};
            }

            image.depth = 1;
            res_dim     = D3D11_RESOURCE_DIMENSION_TEXTURE2D;
        }

        assert(bits_per_pixel(dxgi_format) != 0);
    }

    if (mip_count > D3D11_REQ_MIP_LEVELS)
    {
        throw std::runtime_error{"DDS exceeds number of allowed mipmaps"};
    }

    switch (res_dim)
    {
        case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
            if (array_size > D3D11_REQ_TEXTURE1D_ARRAY_AXIS_DIMENSION ||
                image.width > D3D11_REQ_TEXTURE1D_U_DIMENSION)
            {
                throw std::runtime_error{"DDS has invalid dimensions"};
            }
            break;

        case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
            if (array_size > D3D11_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION ||
                image.width > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION ||
                image.height > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION)
            {
                throw std::runtime_error{"DDS has invalid dimensions"};
            }
            break;

        case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
            if (array_size > 1 || image.width > D3D11_REQ_TEXTURE3D_U_V_OR_W_DIMENSION ||
                image.height > D3D11_REQ_TEXTURE3D_U_V_OR_W_DIMENSION ||
                image.depth > D3D11_REQ_TEXTURE3D_U_V_OR_W_DIMENSION)
            {
                throw std::runtime_error{"DDS has invalid dimensions"};
            }
            break;
        default: break;
    }

    image.faces.resize(array_size);

    for (DDSFace& face : image.faces)
    {
        face.mipmaps.resize(mip_count);
    }

    fill_init_data(image.width,
                   image.height,
                   image.depth,
                   mip_count,
                   array_size,
                   dxgi_format,
                   bit_data,
                   image);

    const Option<ImageFormat> format = from_dxgi_format(dxgi_format);

    if (!format.has_value())
    {
        throw std::runtime_error{"Unsupported format in DDS data."};
    }

    image.format = *format;

    return image;
}

auto load(std::span<const std::byte> memory) -> Option<DDSImage>
{
    // Validate DDS file in memory.
    if (memory.size() < sizeof(uint32_t) + sizeof(DdsHeader))
    {
        return {};
    }

    const uint32_t dw_magic_number = *reinterpret_cast<const uint32_t*>(memory.data());

    if (dw_magic_number != dds_magic)
    {
        return {};
    }

    const DdsHeader* header = reinterpret_cast<const DdsHeader*>(memory.data() + sizeof(uint32_t));

    // Verify the header to validate the DDS file.
    if (header->size != sizeof(DdsHeader) || header->ddspf.size != sizeof(DdsPixelformat))
    {
        throw std::runtime_error{"DDS has invalid header"};
    }

    // Check for the DX10 extension.
    bool b_dxt10_header = false;

    if ((header->ddspf.flags & dds_fourcc) != 0u &&
        make_fourcc('D', 'X', '1', '0') == header->ddspf.four_cc)
    {
        // Must be long enough for both headers and magic value
        if (memory.size() < (sizeof(DdsHeader) + sizeof(uint32_t) + sizeof(DdsHeaderDxT10)))
        {
            throw std::runtime_error{"DDS has invalid header/magic number"};
        }

        b_dxt10_header = true;
    }

    const size_t offset =
        sizeof(uint32_t) + sizeof(DdsHeader) + (b_dxt10_header ? sizeof(DdsHeaderDxT10) : 0);

    return create_image_from_dds(*header, memory.subspan(offset));
}
} // namespace cer::dds
