// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <cerlib/Color.hpp>
#include <variant>

namespace cer
{
struct ParticleColorLerpMod
{
    Color initial_color = white;
    Color final_color   = transparent;
};

struct ParticleContainerMod
{
    Vector2 position;
    float   width                   = 1.0f;
    float   height                  = 1.0f;
    float   restitution_coefficient = 0.0f;
};

struct ParticleDragMod
{
    float drag_coefficient = 0.47f;
    float density          = 0.5f;
};

struct ParticleLinearGravityMod
{
    Vector2 direction;
    float   strength = 0.0f;
};

struct ParticleFastFadeMod
{
    // Nothing to define.
};

struct ParticleOpacityMod
{
    float initial_opacity = 1.0f;
    float final_opacity   = 0.0f;
};

struct ParticleRotationMod
{
    float rotation_rate = half_pi;
};

struct ParticleScaleLerpMod
{
    float initial_scale = 0.0f;
    float final_scale   = 1.0f;
};

struct ParticleVelocityColorMod
{
    Color stationary_color   = white;
    Color velocity_color     = red;
    float velocity_threshold = 0.1f;
};

struct ParticleVortexMod
{
    Vector2 position;
    float   mass      = 1.0f;
    float   max_speed = 1.0f;
};

using ParticleModifier = std::variant<ParticleColorLerpMod,
                                      ParticleContainerMod,
                                      ParticleDragMod,
                                      ParticleLinearGravityMod,
                                      ParticleFastFadeMod,
                                      ParticleOpacityMod,
                                      ParticleRotationMod,
                                      ParticleScaleLerpMod,
                                      ParticleVelocityColorMod,
                                      ParticleVortexMod>;

//     details::ParticleModifierData m_data;
//     float                         m_frequency                    = 60.0f;
//     float                         m_cycle_time                   = 0.0f;
//     int                           m_particles_updated_this_cycle = 0;
} // namespace cer
