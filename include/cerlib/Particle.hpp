// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <cerlib/Color.hpp>
#include <cerlib/Vector2.hpp>

namespace cer
{
struct Particle
{
    float   inception = 0.0f;
    float   age       = 0.0f;
    Vector2 position;
    Vector2 velocity;
    Color   color;
    float   scale    = 0.0f;
    float   rotation = 0.0f;
    float   mass     = 0.0f;
};
} // namespace cer
