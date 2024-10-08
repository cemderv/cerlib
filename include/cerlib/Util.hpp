// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <cerlib/Export.hpp>
#include <span>
#include <string>

namespace cer::util
{
CERLIB_API void trim_string(std::string& str, std::span<const char> chars = {{' '}});

CERLIB_API std::string string_trimmed(std::string_view str, std::span<const char> chars = {{' '}});
} // namespace cer::util
