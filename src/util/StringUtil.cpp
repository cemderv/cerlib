// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "StringUtil.hpp"

#include <cctype>

namespace cer::details
{
auto to_lower_case(std::string_view str) -> std::string
{
    auto result = std::string(str);

    for (auto& ch : result)
    {
        ch = static_cast<char>(std::tolower(ch));
    }

    return result;
}

auto to_upper_case(std::string_view str) -> std::string
{
    auto result = std::string(str);

    for (auto& ch : result)
    {
        ch = static_cast<char>(std::toupper(ch));
    }

    return result;
}
} // namespace cer::details
