// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "cerlib/Color.hpp"
#include "cerlib/Vector2.hpp"
#include <cstdint>
#include <span>

namespace cer::details
{
enum class VertexElement;

struct TessellatedVertex
{
    Vector2 position;
    Vector2 normal;
    float   stroke_width{};
    Color   color;

    static std::span<const VertexElement> elements();
};

constexpr uint32_t draw_line_vertex_count()
{
    return 6;
}

void tessellate_draw_line(std::span<TessellatedVertex> dst,
                          const Vector2&               start,
                          const Vector2&               end,
                          const Color&                 color,
                          float                        stroke_width);
} // namespace cer::details
