// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include <cerlib/Math.hpp>

#include <random>

namespace cer
{
static std::mt19937& random_device()
{
    thread_local std::mt19937 s_generator;
    return s_generator;
}
} // namespace cer

int32_t cer::random_int(int32_t min, int32_t max)
{
    return std::uniform_int_distribution(min, max)(random_device());
}

uint32_t cer::random_uint(uint32_t min, uint32_t max)
{
    return std::uniform_int_distribution(min, max)(random_device());
}

float cer::random_float(float min, float max)
{
    return std::uniform_real_distribution(min, max)(random_device());
}

double cer::random_double(double min, double max)
{
    return std::uniform_real_distribution(min, max)(random_device());
}

uint32_t cer::mipmap_extent(uint32_t base_extent, uint32_t mipmap)
{
    auto extent = base_extent;

    for (uint32_t i = 0; i < mipmap; ++i)
    {
        extent = max(extent / 2, 1u);
    }

    return extent;
}

uint32_t cer::max_mipmap_count_for_extent(uint32_t base_extent)
{
    uint32_t max_mipmap_count = 0u;

    for (uint32_t m = base_extent; m > 0; m = m >> 1, ++max_mipmap_count)
    {
        // Nothing to do
    }

    return max_mipmap_count;
}

int64_t cer::next_aligned_number(int64_t number, int64_t alignment)
{
    return number - 1 + (alignment & -alignment);
}
