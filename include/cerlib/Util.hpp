// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <cerlib/String.hpp>
#include <cerlib/details/ObjectMacros.hpp>
#include <span>

namespace cer::util
{
void trim_string(String& str, std::span<const char> chars = {{' '}});

[[nodiscard]] auto string_trimmed(std::string_view str, std::span<const char> chars = {{' '}})
    -> String;

[[nodiscard]] auto to_lower_case(std::string_view str) -> String;

[[nodiscard]] auto to_upper_case(std::string_view str) -> String;

template <typename Iterator, typename T>
static Iterator binary_find(Iterator begin, Iterator end, T value)
{
    const auto it = std::lower_bound(begin, end, value);

    if (it != end && !(value < *it))
    {
        end = it;
    }

    return end;
}

template <typename T>
void remove_duplicates_but_keep_order(T& container)
{
    auto i = size_t(0);

    while (i != container.size())
    {
        auto j = i;
        ++j;

        while (j != container.size())
        {
            if (container.at(j) == container.at(i))
            {
                container.erase(container.begin() + j);
            }
            else
            {
                ++j;
            }
        }

        ++i;
    }
}

template <typename... T>
struct VariantSwitch : T...
{
    using T::operator()...;
};

template <typename... T>
VariantSwitch(T...) -> VariantSwitch<T...>;
} // namespace cer::util
