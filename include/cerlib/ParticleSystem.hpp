// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <cerlib/ParticleEmitter.hpp>
#include <span>
#include <vector>

namespace cer
{
namespace details
{
class GraphicsDevice;
}

class ParticleSystem
{
    friend details::GraphicsDevice;

  public:
    ParticleSystem();

    explicit ParticleSystem(std::vector<ParticleEmitter> emitters);

    ParticleSystem(const ParticleSystem&) = delete;

    void operator=(const ParticleSystem&) = delete;

    ParticleSystem(ParticleSystem&&) noexcept;

    auto operator=(ParticleSystem&&) noexcept -> ParticleSystem&;

    void update(float elapsed_time);

    void trigger_at(Vector2 position);

    void trigger_from_to(Vector2 from, Vector2 to);

    auto emitter_count() const -> size_t;

    auto emitter_at(size_t index) -> ParticleEmitter&;

    auto emitter_at(size_t index) const -> const ParticleEmitter&;

    auto active_particle_count() const -> size_t;

    auto active_particle_count(size_t index) const -> size_t;

  private:
    struct EmitterData
    {
        ParticleEmitter       emitter;
        float                 timer = 0.0f;
        std::vector<Particle> particle_buffer;
        size_t                active_particle_count   = 0;
        float                 time_since_last_reclaim = 0.0f;
    } s_;

    void reclaim_expired_particles(EmitterData& emitter);

    void update_emitter(EmitterData& data, float elapsed_time);

    void emit(EmitterData& data, Vector2 position, uint32_t count);

    void trigger_emitter_at(EmitterData& emitter, Vector2 position);

    void trigger_emitter_from_to(EmitterData& emitter, Vector2 from, Vector2 to);

    std::vector<EmitterData> m_emitters;
};
} // namespace cer
