// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "StringUtil.hpp"
#include <cctype>

std::string cer::details::to_lower_case(std::string_view str)
{
    std::string result{str};

    for (char& ch : result)
    {
        ch = static_cast<char>(std::tolower(ch));
    }

    return result;
}

std::string cer::details::to_upper_case(std::string_view str)
{
    std::string result{str};

    for (char& ch : result)
    {
        ch = static_cast<char>(std::toupper(ch));
    }

    return result;
}
