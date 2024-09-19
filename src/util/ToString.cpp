// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "cerlib/ToString.hpp"
#include "cerlib/Formatters.hpp"

std::string std::to_string(const cer::Color& value)
{
    return std::format("{}", value);
}

std::string std::to_string(const cer::Vector2& value)
{
    return std::format("{}", value);
}

std::string std::to_string(const cer::Vector3& value)
{
    return std::format("{}", value);
}

std::string std::to_string(const cer::Vector4& value)
{
    return std::format("{}", value);
}

std::string std::to_string(const cer::Matrix& value)
{
    return std::format("{}", value);
}

std::string std::to_string(cer::ImageFormat value)
{
    return std::format("{}", value);
}

std::string std::to_string(const cer::Image& value)
{
    return std::format("{}", value);
}

std::string std::to_string(const cer::Rectangle& value)
{
    return std::format("{}", value);
}
