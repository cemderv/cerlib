// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <cerlib/String.hpp>

namespace cer::details
{
struct MultiStringHash
{
    using hash_type      = std::hash<std::string_view>;
    using is_transparent = void;

    auto operator()(const String& str) const -> size_t
    {
        return hash_type{}(str);
    }

    auto operator()(const char* str) const -> size_t
    {
        return hash_type{}(str);
    }

    auto operator()(std::string_view str) const -> size_t
    {
        return hash_type{}(str);
    }
};
} // namespace cer::details
