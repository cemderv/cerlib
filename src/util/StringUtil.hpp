// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <string>
#include <string_view>

namespace cer::details
{
[[nodiscard]] auto to_lower_case(std::string_view str) -> std::string;

[[nodiscard]] auto to_upper_case(std::string_view str) -> std::string;
} // namespace cer::details
