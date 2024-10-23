// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <cerlib/Color.hpp>
#include <variant>

namespace cer
{
/**
 * @ingroup Graphics
 */
struct ParticleColorLerpMod
{
    Color initial_color = white;
    Color final_color   = transparent;
};

/**
 * @ingroup Graphics
 */
struct ParticleContainerMod
{
    Vector2 position;
    float   width                   = 1.0f;
    float   height                  = 1.0f;
    float   restitution_coefficient = 0.0f;
};

/**
 * @ingroup Graphics
 */
struct ParticleDragMod
{
    float drag_coefficient = 0.47f;
    float density          = 0.5f;
};

/**
 * @ingroup Graphics
 */
struct ParticleLinearGravityMod
{
    Vector2 direction;
    float   strength = 0.0f;
};

/**
 * @ingroup Graphics
 */
struct ParticleFastFadeMod
{
    // Nothing to define.
};

/**
 * @ingroup Graphics
 */
struct ParticleOpacityMod
{
    /** */
    float initial_opacity = 1.0f;

    /** */
    float final_opacity = 0.0f;
};

/**
 * @ingroup Graphics
 */
struct ParticleRotationMod
{
    /** */
    float rotation_rate = half_pi;
};

/**
 * @ingroup Graphics
 */
struct ParticleScaleLerpMod
{
    /** */
    float initial_scale = 0.0f;

    /** */
    float final_scale = 1.0f;
};

/**
 * @ingroup Graphics
 */
struct ParticleVelocityColorMod
{
    /** */
    Color stationary_color = white;

    /** */
    Color velocity_color = red;

    /** */
    float velocity_threshold = 0.1f;
};

/**
 * @ingroup Graphics
 */
struct ParticleVortexMod
{
    /** */
    Vector2 position;

    /** */
    float mass = 1.0f;

    /** */
    float max_speed = 1.0f;
};

/**
 * @ingroup Graphics
 */
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
} // namespace cer
