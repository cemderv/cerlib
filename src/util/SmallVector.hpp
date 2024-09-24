// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

template <typename T, size_t Capacity = 0>
using SmallVector = std::vector<T>;
