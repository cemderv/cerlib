// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "shadercompiler/SourceLocation.hpp"

#include <cassert>

namespace cer::shadercompiler
{
constexpr SourceLocation SourceLocation::std = {
    "<std>",
    0,
    0,
    0,
};

SourceLocation SourceLocation::from_to(const SourceLocation& start, const SourceLocation& end)
{
    assert(start.filename == end.filename);
    assert(start.start_index < end.start_index);

    return {start.filename, start.line, start.column, start.start_index};
}
} // namespace cer::shadercompiler
