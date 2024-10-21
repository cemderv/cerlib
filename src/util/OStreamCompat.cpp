// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "cerlib/OStreamCompat.hpp"
#include "cerlib/ToString.hpp"
#include <ostream>

auto cer::operator<<(std::ostream& os, const Color& value) -> std::ostream&
{
    return os << std::to_string(value);
}

auto cer::operator<<(std::ostream& os, const Vector2& value) -> std::ostream&
{
    return os << std::to_string(value);
}

auto cer::operator<<(std::ostream& os, const Vector3& value) -> std::ostream&
{
    return os << std::to_string(value);
}

auto cer::operator<<(std::ostream& os, const Vector4& value) -> std::ostream&
{
    return os << std::to_string(value);
}

auto cer::operator<<(std::ostream& os, const Matrix& value) -> std::ostream&
{
    return os << std::to_string(value);
}

auto cer::operator<<(std::ostream& os, ImageFormat value) -> std::ostream&
{
    return os << std::to_string(value);
}

auto cer::operator<<(std::ostream& os, const Image& value) -> std::ostream&
{
    return os << std::to_string(value);
}

auto cer::operator<<(std::ostream& os, const Rectangle& value) -> std::ostream&
{
    return os << std::to_string(value);
}
