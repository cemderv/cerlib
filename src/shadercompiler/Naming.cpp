// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "shadercompiler/Naming.hpp"

#include <cerlib/String.hpp>

namespace cer::shadercompiler
{
auto naming::is_identifier_forbidden(std::string_view identifier) -> bool
{
    return identifier.starts_with(forbidden_identifier_prefix);
}
} // namespace cer::shadercompiler
