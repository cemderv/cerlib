// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "CBufferPacker.hpp"

#include "ShaderImpl.hpp"
#include "cerlib/Math.hpp"
#include "util/narrow_cast.hpp"
#include <cassert>

namespace cer::details
{
static auto get_base_alignment(const ShaderParameter& parameter) -> uint16_t
{
    assert(!parameter.is_image);

    switch (parameter.type)
    {
        case ShaderParameterType::Float: [[fallthrough]];
        case ShaderParameterType::Int: [[fallthrough]];
        case ShaderParameterType::Bool: return 4;
        case ShaderParameterType::Vector2: return 8;
        case ShaderParameterType::Vector3: [[fallthrough]];
        case ShaderParameterType::Vector4: [[fallthrough]];
        case ShaderParameterType::Matrix: return 16;

        case ShaderParameterType::FloatArray: [[fallthrough]];
        case ShaderParameterType::IntArray: [[fallthrough]];
        case ShaderParameterType::BoolArray: [[fallthrough]];
        case ShaderParameterType::Vector2Array: [[fallthrough]];
        case ShaderParameterType::Vector3Array: [[fallthrough]];
        case ShaderParameterType::Vector4Array: [[fallthrough]];
        case ShaderParameterType::MatrixArray: return ShaderParameter::array_element_base_alignment;

        case ShaderParameterType::Image:
            throw std::runtime_error{"Image parameter cannot have a base alignment."};
    }

    throw std::runtime_error{"Invalid shader parameter"};
}

auto CBufferPacker::pack_parameters(ShaderImpl::ParameterList& parameters,
                                    uint32_t                   cbuffer_alignment,
                                    bool take_max_of_alignment_and_size) -> Result
{
    auto current_offset = uint16_t{};

    for (auto& param : parameters)
    {
        if (param.is_image)
        {
            continue;
        }

        auto       param_size_in_bytes = param.size_in_bytes;
        const auto base_alignment      = get_base_alignment(param);
        const auto offset =
            narrow_cast<uint16_t>(next_aligned_number(current_offset, base_alignment));

        param.offset = offset;

        if (take_max_of_alignment_and_size)
        {
            current_offset = offset + std::max(param_size_in_bytes, base_alignment);
        }
        else
        {
            current_offset = offset + param_size_in_bytes;
        }
    }

    const auto cbuffer_size = uint32_t(next_aligned_number(current_offset, cbuffer_alignment));

    // Image parameters
    {
        auto i = uint16_t{};
        for (auto& param : parameters)
        {
            if (!param.is_image)
            {
                continue;
            }

            param.offset = i;
            ++i;
        }
    }

    return {
        .cbuffer_size = cbuffer_size,
    };
}
} // namespace cer::details
