// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <string>
#include <string_view>

namespace cer::details
{
struct MultiStringHash
{
    using hash_type      = std::hash<std::string_view>;
    using is_transparent = void;

    size_t operator()(const std::string& str) const
    {
        return hash_type()(str);
    }

    size_t operator()(const char* str) const
    {
        return hash_type()(str);
    }

    size_t operator()(std::string_view str) const
    {
        return hash_type()(str);
    }
};
} // namespace cer::details
