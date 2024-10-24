// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <cerlib/String.hpp>
#include <cstdint>

namespace cer::shadercompiler
{
class SourceLocation final
{
  public:
    static const SourceLocation std;

    constexpr SourceLocation()
        : SourceLocation(std::string_view(), 0, 0, 0)
    {
    }

    constexpr SourceLocation(std::string_view filename,
                             uint16_t         line,
                             uint16_t         column,
                             uint16_t         start_index)
        : filename(filename)
        , line(line)
        , column(column)
        , start_index(start_index)
    {
    }

    static auto from_to(const SourceLocation& start, const SourceLocation& end) -> SourceLocation;

    auto operator==(const SourceLocation&) const -> bool = default;

    auto operator!=(const SourceLocation&) const -> bool = default;

    std::string_view filename;
    uint16_t         line;
    uint16_t         column;
    uint16_t         start_index;
};
} // namespace cer::shadercompiler
