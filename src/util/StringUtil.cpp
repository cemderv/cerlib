// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include <cctype>
#include <cerlib/StringUtil.hpp>

auto cer::details::to_lower_case(std::string_view str) -> std::string
{
    auto result = std::string{str};

    for (auto& ch : result)
    {
        ch = char(std::tolower(ch));
    }

    return result;
}

auto cer::details::to_upper_case(std::string_view str) -> std::string
{
    auto result = std::string{str};

    for (auto& ch : result)
    {
        ch = char(std::toupper(ch));
    }

    return result;
}
