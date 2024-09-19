// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "cerlib/OStreamCompat.hpp"
#include "cerlib/ToString.hpp"
#include <ostream>

std::ostream& cer::operator<<(std::ostream& os, const Color& value)
{
    return os << std::to_string(value);
}

std::ostream& cer::operator<<(std::ostream& os, const Vector2& value)
{
    return os << std::to_string(value);
}

std::ostream& cer::operator<<(std::ostream& os, const Vector3& value)
{
    return os << std::to_string(value);
}

std::ostream& cer::operator<<(std::ostream& os, const Vector4& value)
{
    return os << std::to_string(value);
}

std::ostream& cer::operator<<(std::ostream& os, const Matrix& value)
{
    return os << std::to_string(value);
}

std::ostream& cer::operator<<(std::ostream& os, ImageFormat value)
{
    return os << std::to_string(value);
}

std::ostream& cer::operator<<(std::ostream& os, const Image& value)
{
    return os << std::to_string(value);
}

std::ostream& cer::operator<<(std::ostream& os, const Rectangle& value)
{
    return os << std::to_string(value);
}
