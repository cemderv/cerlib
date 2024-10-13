// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <cerlib/BlendState.hpp>
#include <cerlib/Image.hpp>
#include <cerlib/Interval.hpp>
#include <cerlib/Math.hpp>
#include <cerlib/Particle.hpp>
#include <cerlib/ParticleEmitterShape.hpp>
#include <cerlib/ParticleModifier.hpp>
#include <cerlib/Vector2.hpp>
#include <chrono>
#include <memory>
#include <span>
#include <vector>

namespace cer
{
/**
 * Represents emission properties of a ParticleEmitter.
 *
 * These properties determine the value ranges and behaviors
 * of how particles are emitted. They do *not* alter the behavior
 * of a particle that has already been emitted.
 *
 * For modifiers that modify already emitted particles, see
 * the ParticleModifier type and the `modifiers` field of ParticleEmitter.
 */
struct ParticleEmissionParams
{
    /** The number of particles to spawn with each emission. */
    UIntInterval quantity = {1u, 1u};

    /** The speed of the particle to be emitted. */
    FloatInterval speed = {-100.0f, 100.0f};

    /** The color of the particle to be emitted. */
    ColorInterval color = {black, white};

    /** The duration of the particle to be emitted, in fractional seconds. */
    FloatInterval duration = {1.0f, 1.0f};

    /** The scale factor of the particle to be emitted. */
    FloatInterval scale = {1.0f, 10.0f};

    /** The rotation of the particle to be emitted, in radians. */
    FloatInterval rotation = {-pi, pi};

    /** The mass of the particle to be emitted. */
    FloatInterval mass = {1.0f, 1.0f};
};

/**
 * Represents the description of a particle emitter.
 *
 * Particle emitters do not emit particles themselves.
 * Instead, they are part of a ParticleSystem that is responsible
 * for managing them.
 *
 * Particles are managed and emitted using a ParticleSystem.
 */
struct ParticleEmitter
{
    /** The duration of this emitter's particles. */
    std::chrono::seconds duration = std::chrono::seconds{1};

    /** The shape (form) of this emitter. */
    ParticleEmitterShape shape = ParticlePointShape{};

    /** A list of all modifiers that affect this emitter. */
    std::vector<ParticleModifier> modifiers;

    /** Emission parameters of this emitter. */
    ParticleEmissionParams emission;

    /** The blend state that is used for this emitter's particles. */
    BlendState blend_state = additive;

    /** The image that is used for this emitter's particles. */
    Image image;
};
} // namespace cer
