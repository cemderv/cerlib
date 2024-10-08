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

/**
 * Represents a system that manages and emits particles.
 *
 * A particle system consists of particle emitters that define
 * how individual particles look and how they are emitted.
 *
 * A ParticleSystem owns its emitters and associated data and
 * cannot be copied, but moved.
 *
 * The typical lifecycle of a particle system is:
 * - Create it.
 * - Call the `update` method with each game update. This advances the particle simulation.
 * - Call `cer::draw_particles` to draw all of the system's particles.
 */
class ParticleSystem
{
    friend details::GraphicsDevice;

  public:
    ParticleSystem();

    /**
     * Creates a particle system with a specific list of emitters.
     * The particle system will own the emitters and their associated data.
     * After creation, the emitters can be accessed and modified by the user
     * using the `emitter_count` and `emitter_at` methods.
     * However, the list itself cannot be modified, i.e. the number of emitters
     * is immutable.
     *
     * @param emitters The emitters that should make up the system
     */
    explicit ParticleSystem(std::vector<ParticleEmitter> emitters);

    ParticleSystem(const ParticleSystem&) = delete;

    void operator=(const ParticleSystem&) = delete;

    ParticleSystem(ParticleSystem&&) noexcept;

    auto operator=(ParticleSystem&&) noexcept -> ParticleSystem&;

    /**
     * Advances the system's particle simulation.
     *
     * @param elapsed_time The time to advance, which typically is derived from the `elapsed_time`
     * field of GameTime.
     */
    void update(float elapsed_time);

    /**
     * Emits particles at a specific location.
     *
     * @param position The location at which to emit particles.
     */
    void trigger_at(Vector2 position);

    /**
     * Emits particles along a specific line.
     *
     * @param from The start point of the line
     * @param to The end point of the line
     */
    void trigger_from_to(Vector2 from, Vector2 to);

    /**
     * Gets the number of emitters that are in this system.
     */
    auto emitter_count() const -> size_t;

    /**
     * Gets an emitter of this system by index.
     * The number of emitters can be obtained using `emitter_count`.
     *
     * @param index The index of the emitter.
     *
     * @throw std::out_of_range If the index exceeds the emitter list bounds.
     */
    auto emitter_at(size_t index) -> ParticleEmitter&;

    /**
     * Gets an emitter of this system by index.
     * The number of emitters can be obtained using `emitter_count`.
     *
     * @param index The index of the emitter.
     *
     * @throw std::out_of_range If the index exceeds the emitter list bounds.
     */
    auto emitter_at(size_t index) const -> const ParticleEmitter&;

    /**
     * Gets the current total number of particles that have been emitted by this system.
     */
    auto active_particle_count() const -> size_t;

    /**
     * Gets the current number of particles that have been emitted by a specific emitter.
     *
     * @param index The index of the emitter for which to query the number of emitted particles.
     */
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
