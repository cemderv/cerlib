// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <unordered_set>

namespace cer
{
template <typename Value, typename Hash = std::hash<Value>, typename Equal = std::equal_to<Value>>
using HashSet = std::unordered_set<Value, Hash, Equal>;
}
