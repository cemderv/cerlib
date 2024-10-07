// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include <cerlib/Math.hpp>

#include <random>

namespace cer::details
{
thread_local int32_t s_fastrand_seed = 1;

static auto random_device() -> std::mt19937&
{
    thread_local auto s_generator = std::mt19937{};
    return s_generator;
}
} // namespace cer::details

auto cer::random_int(int32_t min, int32_t max) -> int32_t
{
    return std::uniform_int_distribution{min, max}(details::random_device());
}

auto cer::random_uint(uint32_t min, uint32_t max) -> uint32_t
{
    return std::uniform_int_distribution{min, max}(details::random_device());
}

auto cer::random_float(float min, float max) -> float
{
    return std::uniform_real_distribution{min, max}(details::random_device());
}

auto cer::random_double(double min, double max) -> double
{
    return std::uniform_real_distribution{min, max}(details::random_device());
}

void cer::seed_fastrand(int32_t value)
{
    details::s_fastrand_seed = value;
}

auto cer::fastrand_int() -> int32_t
{
    details::s_fastrand_seed = 214013 * details::s_fastrand_seed + 2531011;

    return (details::s_fastrand_seed >> 16) & 0x7FFF;
}

auto cer::fastrand_int(int32_t min, int32_t max) -> int32_t
{
    return int32_t(lerp(double(min), double(max), double(fastrand_float_zero_to_one())));
}

auto cer::fastrand_int(const IntInterval& interval) -> int32_t
{
    return fastrand_int(interval.min, interval.max);
}

auto cer::fastrand_uint() -> uint32_t
{
    return uint32_t(fastrand_int());
}

auto cer::fastrand_uint(uint32_t min, uint32_t max) -> uint32_t
{
    return uint32_t(lerp(double(min), double(max), double(fastrand_float_zero_to_one())));
}

auto cer::fastrand_uint(const UIntInterval& interval) -> uint32_t
{
    return fastrand_uint(interval.min, interval.max);
}

auto cer::fastrand_float_zero_to_one() -> float
{
    return float(double(fastrand_int()) / double(std::numeric_limits<int16_t>::max()));
}

auto cer::fastrand_float(float min, float max) -> float
{
    return lerp(min, max, fastrand_float_zero_to_one());
}

auto cer::fastrand_float(const FloatInterval& interval) -> float
{
    return fastrand_float(interval.min, interval.max);
}

auto cer::fastrand_angle() -> float
{
    return fastrand_float(-pi, pi);
}

auto cer::mipmap_extent(uint32_t base_extent, uint32_t mipmap) -> uint32_t
{
    auto extent = base_extent;

    for (uint32_t i = 0; i < mipmap; ++i)
    {
        extent = max(extent / 2, 1u);
    }

    return extent;
}

auto cer::max_mipmap_count_for_extent(uint32_t base_extent) -> uint32_t
{
    uint32_t max_mipmap_count = 0u;

    for (uint32_t m = base_extent; m > 0; m = m >> 1, ++max_mipmap_count)
    {
        // Nothing to do
    }

    return max_mipmap_count;
}

auto cer::next_aligned_number(int64_t number, int64_t alignment) -> int64_t
{
    return number - 1 + (alignment & -alignment);
}
