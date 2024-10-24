// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <unordered_map>

namespace cer
{
template <typename Key,
          typename Value,
          typename Hash  = std::hash<Key>,
          typename Equal = std::equal_to<Key>>
using HashMap = std::unordered_map<Key, Value, Hash, Equal>;
}
