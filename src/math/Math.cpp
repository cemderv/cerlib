// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include <cerlib/Math.hpp>

#include <random>

namespace cer
{
static auto random_device() -> std::mt19937&
{
    thread_local auto s_generator = std::mt19937{};
    return s_generator;
}
} // namespace cer

auto cer::random_int(int32_t min, int32_t max) -> int32_t
{
    return std::uniform_int_distribution{min, max}(random_device());
}

auto cer::random_uint(uint32_t min, uint32_t max) -> uint32_t
{
    return std::uniform_int_distribution{min, max}(random_device());
}

auto cer::random_float(float min, float max) -> float
{
    return std::uniform_real_distribution{min, max}(random_device());
}

auto cer::random_double(double min, double max) -> double
{
    return std::uniform_real_distribution{min, max}(random_device());
}

auto cer::next_aligned_number(int64_t number, int64_t alignment) -> int64_t
{
    return number - 1 + (alignment & -alignment);
}
