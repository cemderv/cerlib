// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <cerlib/details/ObjectMacros.hpp>

namespace cer::details
{
using MainFunc = int (*)(int, char**);

[[nodiscard]] auto run_game(int a, char* b[], MainFunc c, void* d) -> int;
} // namespace cer::details
