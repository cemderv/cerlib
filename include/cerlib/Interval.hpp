// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <cstddef>
#include <cstdint>

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

namespace cer::details
{
template <typename T>
struct IntervalType
{
    constexpr IntervalType() = default;

    constexpr IntervalType(const T& value);

    constexpr IntervalType(const T& min, const T& max);

    constexpr auto contains(const T& value) const -> bool;

    auto operator==(const IntervalType& other) const -> bool = default;

    auto operator!=(const IntervalType& other) const -> bool = default;

    T min = T(0);
    T max = T(1);
};
} // namespace cer::details

#include <cassert>

template <typename T>
constexpr cer::details::IntervalType<T>::IntervalType(const T& value)
    : min(value)
    , max(value)
{
}

template <typename T>
constexpr cer::details::IntervalType<T>::IntervalType(const T& min, const T& max)
    : min(min)
    , max(max)
{
    // assert(min <= max);
}

template <typename T>
constexpr auto cer::details::IntervalType<T>::contains(const T& value) const -> bool
{
    return value >= min && value <= max;
}

namespace cer
{
using FloatInterval = details::IntervalType<float>;

using IntInterval = details::IntervalType<int32_t>;

using UIntInterval = details::IntervalType<uint32_t>;
} // namespace cer