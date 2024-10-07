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
struct ParticleEmissionParams
{
    UIntInterval  quantity = {1};
    FloatInterval speed    = {-100.0f, 100.0f};
    ColorInterval color    = {black, white};
    FloatInterval duration = {1.0f, 1.0f};
    FloatInterval scale    = {1.0f, 10.0f};
    FloatInterval rotation = {-pi, pi};
    FloatInterval mass     = {1.0f};
};

struct ParticleEmitter
{
    std::chrono::seconds          duration = std::chrono::seconds{1};
    ParticleEmitterShape          shape    = ParticlePointShape{};
    std::vector<ParticleModifier> modifiers;
    ParticleEmissionParams        emission;
    BlendState                    blend_state = BlendState::additive();
    Image                         image;

    auto particles() const -> std::span<const Particle>
    {
        return {s_.particles.begin(), size_t(s_.active_particle_count)};
    }

    struct State
    {
        float                 timer = 0.0f;
        std::vector<Particle> particles;
        uint32_t              active_particle_count   = 0;
        float                 time_since_last_reclaim = 0.0f;
    } s_;
};
} // namespace cer
