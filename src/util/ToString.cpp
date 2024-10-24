// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "cerlib/ToString.hpp"
#include "cerlib/Formatters.hpp"

auto std::to_string(const cer::Color& value) -> cer::String
{
    return cer_fmt::format("{}", value);
}

auto std::to_string(const cer::Vector2& value) -> cer::String
{
    return cer_fmt::format("{}", value);
}

auto std::to_string(const cer::Vector3& value) -> cer::String
{
    return cer_fmt::format("{}", value);
}

auto std::to_string(const cer::Vector4& value) -> cer::String
{
    return cer_fmt::format("{}", value);
}

auto std::to_string(const cer::Matrix& value) -> cer::String
{
    return cer_fmt::format("{}", value);
}

auto std::to_string(cer::ImageFormat value) -> cer::String
{
    return cer_fmt::format("{}", value);
}

auto std::to_string(const cer::Image& value) -> cer::String
{
    return cer_fmt::format("{}", value);
}

auto std::to_string(const cer::Rectangle& value) -> cer::String
{
    return cer_fmt::format("{}", value);
}
