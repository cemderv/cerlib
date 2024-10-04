// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <cerlib/details/ObjectMacros.hpp>
#include <span>
#include <string>

namespace cer::util
{
void trim_string(std::string& str, std::span<const char> chars = {{' '}});

auto string_trimmed(std::string_view str, std::span<const char> chars = {{' '}}) -> std::string;
} // namespace cer::util
