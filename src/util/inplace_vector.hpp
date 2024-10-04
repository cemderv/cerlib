// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

// This is a placeholder until std::inplace_vector is widely supported.
template <typename T, size_t Capacity = 0>
using inplace_vector = std::vector<T>;
