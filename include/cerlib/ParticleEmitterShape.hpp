// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <cerlib/Vector2.hpp>
#include <variant>

namespace cer
{
struct ParticleBoxFillShape
{
    float width  = 1.0f;
    float height = 1.0f;
};

struct ParticleBoxShape
{
    float width  = 1.0f;
    float height = 1.0f;
};

struct ParticleCircleShape
{
    float radius         = 1.0f;
    bool  should_radiate = false;
};

struct ParticlePointShape
{
};

struct ParticleRingShape
{
    float radius         = 1.0f;
    bool  should_radiate = false;
};

struct ParticleSprayShape
{
    Vector2 direction;
    float   spread = 1.0f;
};

using ParticleEmitterShape = std::variant<ParticleBoxFillShape,
                                          ParticleBoxShape,
                                          ParticleCircleShape,
                                          ParticlePointShape,
                                          ParticleRingShape,
                                          ParticleSprayShape>;
} // namespace cer
