// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "Tessellation2D.hpp"

#include "VertexElement.hpp"

#include <array>

namespace cer::details
{
auto TessellatedVertex::elements() -> std::span<const VertexElement>
{
    static constexpr auto s_elements_instance = std::array{
        VertexElement::Vector2,
        VertexElement::Vector2,
        VertexElement::Float,
        VertexElement::Vector4,
    };

    return s_elements_instance;
}

void tessellate_draw_line(std::span<TessellatedVertex> dst,
                          const Vector2&               start,
                          const Vector2&               end,
                          const Color&                 color,
                          float                        stroke_width)
{
    const auto normal  = line_normal(start, end);
    const auto offset1 = -normal;
    const auto offset2 = normal;

    const auto v0 = TessellatedVertex{
        .position     = start,
        .normal       = offset1,
        .stroke_width = stroke_width,
        .color        = color,
    };

    const auto v1 = TessellatedVertex{
        .position     = start,
        .normal       = offset2,
        .stroke_width = stroke_width,
        .color        = color,
    };

    const auto v2 = TessellatedVertex{
        .position     = end,
        .normal       = offset1,
        .stroke_width = stroke_width,
        .color        = color,
    };

    const auto v3 = TessellatedVertex{
        .position     = end,
        .normal       = offset2,
        .stroke_width = stroke_width,
        .color        = color,
    };

    dst[0] = v0;
    dst[1] = v0;
    dst[2] = v1;
    dst[3] = v2;
    dst[4] = v3;
    dst[5] = v3;
}
} // namespace cer::details
