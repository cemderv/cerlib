// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "cerlib/Util.hpp"

#include <cctype>
#include <ranges>

void cer::util::trim_string(String& str, std::span<const char> chars)
{
    const auto should_remove = [&chars](char ch) {
        return std::ranges::find(chars, ch) != chars.end();
    };

    while (!str.empty() && should_remove(str.back()))
    {
        str.pop_back();
    }

    while (!str.empty() && should_remove(str.front()))
    {
        str.erase(str.begin());
    }
}

auto cer::util::string_trimmed(std::string_view str, std::span<const char> chars) -> String
{
    auto result = String{str};
    trim_string(result, chars);

    return result;
}

auto cer::util::to_lower_case(std::string_view str) -> String
{
    auto result = String{str};

    for (auto& ch : result)
    {
        ch = char(std::tolower(ch));
    }

    return result;
}

auto cer::util::to_upper_case(std::string_view str) -> String
{
    auto result = String{str};

    for (auto& ch : result)
    {
        ch = char(std::toupper(ch));
    }

    return result;
}
