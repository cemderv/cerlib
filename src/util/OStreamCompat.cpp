// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "cerlib/OStreamCompat.hpp"
#include "cerlib/ToString.hpp"

#include <ostream>

auto cer::operator<<(std::ostream& os, const cer::Color& value) -> std::ostream&
{
    return os << std::to_string(value);
}

auto cer::operator<<(std::ostream& os, const cer::Vector2& value) -> std::ostream&
{
    return os << std::to_string(value);
}

auto cer::operator<<(std::ostream& os, const cer::Vector3& value) -> std::ostream&
{
    return os << std::to_string(value);
}

auto cer::operator<<(std::ostream& os, const cer::Vector4& value) -> std::ostream&
{
    return os << std::to_string(value);
}

auto cer::operator<<(std::ostream& os, const cer::Matrix& value) -> std::ostream&
{
    return os << std::to_string(value);
}

auto cer::operator<<(std::ostream& os, cer::ImageFormat value) -> std::ostream&
{
    return os << std::to_string(value);
}

auto cer::operator<<(std::ostream& os, const cer::Image& value) -> std::ostream&
{
    return os << std::to_string(value);
}

auto cer::operator<<(std::ostream& os, const cer::Rectangle& value) -> std::ostream&
{
    return os << std::to_string(value);
}
