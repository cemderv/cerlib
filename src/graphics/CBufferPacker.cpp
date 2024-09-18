// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "CBufferPacker.hpp"

#include "ShaderImpl.hpp"
#include "cerlib/Math.hpp"
#include "util/InternalError.hpp"

#include <cassert>
#include <gsl/util>

namespace cer::details
{
static uint16_t get_base_alignment(const ShaderParameter& parameter)
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
            CER_THROW_INTERNAL_ERROR_STR("Image parameter cannot have a base alignment.");
    }

    CER_THROW_INTERNAL_ERROR_STR("Invalid shader parameter");
}

CBufferPacker::Result CBufferPacker::pack_parameters(ShaderImpl::ParameterList& parameters,
                                                     uint32_t                   cbuffer_alignment,
                                                     bool take_max_of_alignment_and_size)
{
    uint16_t current_offset{};

    for (ShaderParameter& param : parameters)
    {
        if (param.is_image)
        {
            continue;
        }

        uint16_t       param_size_in_bytes = param.size_in_bytes;
        const uint16_t base_alignment      = get_base_alignment(param);
        const uint16_t offset =
            gsl::narrow_cast<uint16_t>(next_aligned_number(current_offset, base_alignment));

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

    const uint32_t cbuffer_size =
        static_cast<uint32_t>(next_aligned_number(current_offset, cbuffer_alignment));

    // Image parameters
    {
        uint16_t i{};
        for (ShaderParameter& param : parameters)
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
