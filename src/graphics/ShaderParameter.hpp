// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "cerlib/Image.hpp"

#include <any>
#include <cerlib/String.hpp>

namespace cer::details
{
class CBufferPacker;

enum class ShaderParameterType : uint8_t
{
    Float,
    Int,
    Bool,
    Vector2,
    Vector3,
    Vector4,
    Matrix,
    Image,
    FloatArray,
    IntArray,
    BoolArray,
    Vector2Array,
    Vector3Array,
    Vector4Array,
    MatrixArray,
};

class ShaderParameter final
{
    friend CBufferPacker;

  public:
    static constexpr auto array_element_base_alignment = 16u;

    String              name;
    ShaderParameterType type{};
    uint16_t            offset{};
    uint16_t            size_in_bytes{};
    uint16_t            array_size{};
    bool                is_image{};
    Image               image;
    std::any            default_value;
};

static auto operator==(const ShaderParameter& lhs, const ShaderParameter& rhs) -> bool
{
    return lhs.name == rhs.name;
}

static auto operator==(const ShaderParameter& lhs, std::string_view rhs) -> bool
{
    return lhs.name == rhs;
}

static auto operator<(std::string_view name, const ShaderParameter& parameter) -> bool
{
    return parameter.name > name;
}

static auto operator<(const ShaderParameter& parameter, std::string_view name) -> bool
{
    return parameter.name < name;
}
} // namespace cer::details
