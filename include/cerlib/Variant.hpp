// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <variant>

namespace cer
{
template <typename... Types>
using Variant = std::variant<Types...>;
}
