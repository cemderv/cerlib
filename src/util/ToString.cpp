// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "cerlib/ToString.hpp"
#include "cerlib/Formatters.hpp"

auto std::to_string(const cer::Color& value) -> std::string
{
    return cer_fmt::format("{}", value);
}

auto std::to_string(const cer::Vector2& value) -> std::string
{
    return cer_fmt::format("{}", value);
}

auto std::to_string(const cer::Vector3& value) -> std::string
{
    return cer_fmt::format("{}", value);
}

auto std::to_string(const cer::Vector4& value) -> std::string
{
    return cer_fmt::format("{}", value);
}

auto std::to_string(const cer::Matrix& value) -> std::string
{
    return cer_fmt::format("{}", value);
}

auto std::to_string(cer::ImageFormat value) -> std::string
{
    return cer_fmt::format("{}", value);
}

auto std::to_string(const cer::Image& value) -> std::string
{
    return cer_fmt::format("{}", value);
}

auto std::to_string(const cer::Rectangle& value) -> std::string
{
    return cer_fmt::format("{}", value);
}
