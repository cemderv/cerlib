// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "MultiStringHash.hpp"

#include <unordered_map>

namespace cer::details
{
template <typename T>
using StringUnorderedMap = std::unordered_map<std::string, T, MultiStringHash, std::equal_to<>>;
}
