// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "cerlib/ParticleSystem.hpp"

#include <numeric>

#include "cerlib/Logging.hpp"

namespace cer
{
struct ParticleModifierVisitor
{
    std::span<Particle> particles;
    float               elapsed_time = 0.0f;

    void operator()(const ParticleColorLerpMod& mod) const
    {
        const auto initial_color = mod.initial_color;
        const auto final_color   = mod.final_color;

        const auto delta = Color{
            final_color.r - initial_color.r,
            final_color.g - initial_color.g,
            final_color.b - initial_color.b,
            final_color.a - initial_color.a,
        };

        for (auto& particle : particles)
        {
            particle.color.r = initial_color.r + (delta.r * particle.age);
            particle.color.g = initial_color.g + (delta.g * particle.age);
            particle.color.b = initial_color.b + (delta.b * particle.age);
            particle.color.a = initial_color.a + (delta.a * particle.age);
        }
    }

    void operator()(const ParticleContainerMod& mod) const
    {
        const auto left   = mod.width * -0.5f;
        const auto right  = mod.width * 0.5f;
        const auto top    = mod.height * -0.5f;
        const auto bottom = mod.height * 0.5f;

        for (auto& particle : particles)
        {
            auto& pos = particle.position;
            auto& vel = particle.velocity;

            if (pos.x < left)
            {
                pos.x = left + (left - pos.x);
                vel.x = -vel.x * mod.restitution_coefficient;
            }
            else if (particle.position.x > right)
            {
                pos.x = right - (pos.x - right);
                vel.x = -vel.x * mod.restitution_coefficient;
            }

            if (pos.y < top)
            {
                pos.y = top + (top - pos.y);
                vel.y = -vel.y * mod.restitution_coefficient;
            }
            else if (pos.y > bottom)
            {
                pos.y = bottom - (pos.y - bottom);
                vel.y = -vel.y * mod.restitution_coefficient;
            }
        }
    }

    void operator()(const ParticleDragMod& mod) const
    {
        for (auto& particle : particles)
        {
            const auto drag = -mod.drag_coefficient * mod.density * particle.mass * elapsed_time;
            particle.velocity += particle.velocity * drag;
        }
    }

    void operator()(const ParticleLinearGravityMod& mod) const
    {
        const auto vector = mod.direction * mod.strength * elapsed_time;

        for (auto& particle : particles)
        {
            particle.velocity += vector * particle.mass;
        }
    }

    void operator()(const ParticleFastFadeMod&) const
    {
        for (auto& particle : particles)
        {
            particle.color.a = 1.0f - particle.age;
        }
    }

    void operator()(const ParticleOpacityMod& mod) const
    {
        const auto delta = mod.final_opacity - mod.initial_opacity;

        for (auto& particle : particles)
        {
            particle.color.a = (delta * particle.age) + mod.initial_opacity;
        }
    }

    void operator()(const ParticleRotationMod& mod) const
    {
        const auto rotation_rate_delta = mod.rotation_rate * elapsed_time;

        for (auto& particle : particles)
        {
            particle.rotation += rotation_rate_delta;
        }
    }

    void operator()(const ParticleScaleLerpMod& mod) const
    {
        const auto delta = mod.final_scale - mod.initial_scale;

        for (auto& particle : particles)
        {
            particle.scale = (delta * particle.age) + mod.initial_scale;
        }
    }

    void operator()(const ParticleVelocityColorMod& mod) const
    {
        const auto velocity_threshold2 = squared(mod.velocity_threshold);

        for (auto& particle : particles)
        {
            const auto velocity_length_squared = length_squared(particle.velocity);
            const auto delta_color             = mod.velocity_color - mod.stationary_color;

            if (velocity_length_squared >= velocity_threshold2)
            {
                particle.color = mod.velocity_color;
            }
            else
            {
                const auto t = sqrt(velocity_length_squared) / velocity_threshold2;

                particle.color.r = (delta_color.r * t) + mod.stationary_color.r;
                particle.color.g = (delta_color.g * t) + mod.stationary_color.g;
                particle.color.b = (delta_color.b * t) + mod.stationary_color.b;
                particle.color.a = (delta_color.a * t) + mod.stationary_color.a;
            }
        }
    }

    void operator()(const ParticleVortexMod& mod) const
    {
        for (auto& particle : particles)
        {
            auto       dist      = mod.position - particle.position;
            const auto distance2 = length_squared(dist);
            const auto distance  = sqrt(distance2);

            auto m = (10'000.0f * mod.mass * particle.mass) / distance2;
            m      = max(min(m, mod.max_speed), -mod.max_speed) * elapsed_time;

            particle.velocity += (dist / distance) * m;
        }
    }
};

static void execute_modifier(const ParticleModifier& modifier,
                             float                   elapsed_time,
                             std::span<Particle>     particles)
{
    std::visit(
        ParticleModifierVisitor{
            .particles    = particles,
            .elapsed_time = elapsed_time,
        },
        modifier);
}

static void reclaim_expired_particles(ParticleEmitter& emitter)
{
    auto expired_particle_count = uint32_t(0);

    auto&      state      = emitter.s_;
    const auto time       = state.timer;
    const auto count      = state.active_particle_count;
    const auto duration_f = float(std::chrono::duration<double>{emitter.duration}.count());

    for (uint32_t i = 0; i < count; ++i)
    {
        const auto& particle = state.particles[i];

        if ((time - particle.inception) < duration_f)
        {
            break;
        }

        ++expired_particle_count;
    }

    state.active_particle_count -= expired_particle_count;

    assert(expired_particle_count < state.particles.size());

    std::copy_n(state.particles.cbegin() + expired_particle_count,
                state.active_particle_count,
                state.particles.begin());
}

static void update_emitter(ParticleEmitter& emitter, float elapsed_time)
{
    auto& state = emitter.s_;

    state.timer += elapsed_time;
    state.time_since_last_reclaim += elapsed_time;

    if (state.active_particle_count == 0)
    {
        return;
    }

    if (const auto reclaim_time_reciprocal = 1.0f / state.reclaim_frequency;
        state.time_since_last_reclaim > reclaim_time_reciprocal)
    {
        reclaim_expired_particles(emitter);
        state.time_since_last_reclaim -= reclaim_time_reciprocal;
    }

    const auto duration_f = float(std::chrono::duration<double>{emitter.duration}.count());

    if (state.active_particle_count > 0)
    {
        for (auto& particle : state.particles)
        {
            particle.age = (state.timer - particle.inception) / duration_f;
            particle.position += particle.velocity * elapsed_time;
        }

        // Execute modifiers
        for (const auto& modifier : emitter.modifiers)
        {
            execute_modifier(modifier,
                             elapsed_time,
                             std::span{state.particles.data(), state.active_particle_count});
        }
    }
}

struct ParticleEmitterShapeVisitor
{
    struct Result
    {
        Vector2 offset;
        Vector2 heading;
    };

    auto operator()(const ParticleBoxFillShape& p) const -> Result
    {
        return {
            .offset  = {fastrand_float(p.width * -0.5f, p.width * 0.5f),
                        fastrand_float(p.height * -0.5f, p.height * 0.5f)},
            .heading = fastrand_angle_vector2(),
        };
    }

    auto operator()(const ParticleBoxShape& p) const -> Result
    {
        return {
            .offset  = {fastrand_float(p.width * -0.5f, p.width * 0.5f),
                        fastrand_float(p.height * -0.5f, p.height * 0.5f)},
            .heading = fastrand_angle_vector2(),
        };
    }

    auto operator()(const ParticleCircleShape& p) const -> Result
    {
        const auto dist    = fastrand_float(0.0f, p.radius);
        const auto heading = fastrand_angle_vector2();

        return {
            .offset  = heading * dist,
            .heading = p.should_radiate ? fastrand_angle_vector2() : heading,
        };
    }

    auto operator()(const ParticlePointShape& p) const -> Result
    {
        return {
            .offset  = {},
            .heading = fastrand_angle_vector2(),
        };
    }

    auto operator()(const ParticleRingShape& p) const -> Result
    {
        const auto heading = fastrand_angle_vector2();

        return {
            .offset  = heading * p.radius,
            .heading = p.should_radiate ? fastrand_angle_vector2() : heading,
        };
    }

    auto operator()(const ParticleSprayShape& p) const -> Result
    {
        auto angle = std::atan2(p.direction.y, p.direction.x);
        angle      = fastrand_float(angle - (p.spread * 0.5f), angle + (p.spread * 0.5f));

        return {
            .offset  = {},
            .heading = {cos(angle), sin(angle)},
        };
    }
};

static auto calculate_random_offset_and_heading(const ParticleEmitterShape& shape)
    -> ParticleEmitterShapeVisitor::Result
{
    return std::visit(ParticleEmitterShapeVisitor{}, shape);
}

static void emit(ParticleEmitter& emitter, Vector2 position, uint32_t count)
{
    auto& state = emitter.s_;

    // Ensure that the particle buffer is large enough.
    {
        const auto current_capacity = state.particles.capacity();
        assert(current_capacity >= state.active_particle_count);

        const auto available_count = current_capacity - state.active_particle_count;

        if (available_count == 0)
        {
            if (current_capacity == 0)
            {
                cer::log_info("Resizing initially");
                state.particles.resize(100);
            }
            else
            {
                state.particles.resize(size_t(double(current_capacity) * 1.5));
                cer::log_info("Resized from {} to {}",
                              current_capacity,
                              state.particles.capacity());
            }
        }
    }

    const auto previous_tail = state.active_particle_count;

    state.active_particle_count += count;

    for (uint32_t i = 0; i < count; ++i)
    {
        auto& particle = state.particles[previous_tail + i];

        particle.inception = state.timer;
        particle.age       = 0.0f;

        const auto [offset, heading] = calculate_random_offset_and_heading(emitter.shape);

        particle.position = offset;
        particle.velocity = heading;

        particle.position += position;
        particle.velocity *= fastrand_float(emitter.emission.speed);

        particle.color    = fastrand_color(emitter.emission.color);
        particle.scale    = fastrand_float(emitter.emission.scale);
        particle.rotation = fastrand_float(emitter.emission.rotation);
        particle.mass     = fastrand_float(emitter.emission.mass);
    }
}

static void trigger_emitter_at(ParticleEmitter& emitter, Vector2 position)
{
    emit(emitter, position, fastrand_uint(emitter.emission.quantity));
}

static void trigger_emitter_from_to(ParticleEmitter& emitter, Vector2 from, Vector2 to)
{
    const auto count     = fastrand_uint(emitter.emission.quantity);
    const auto direction = to - from;

    for (uint32_t i = 0; i < count; ++i)
    {
        const auto offset = direction * fastrand_float_zero_to_one();
        emit(emitter, from + offset, 1);
    }
}

ParticleSystem::ParticleSystem() = default;

ParticleSystem::ParticleSystem(std::vector<ParticleEmitter> emitters)
    : m_emitters(std::move(emitters))
{
}

ParticleSystem::ParticleSystem(ParticleSystem&&) noexcept = default;

auto ParticleSystem::operator=(ParticleSystem&&) noexcept -> ParticleSystem& = default;

void ParticleSystem::update(float elapsed_time)
{
    for (auto& emitter : m_emitters)
    {
        update_emitter(emitter, elapsed_time);
    }
}

void ParticleSystem::trigger_at(Vector2 position)
{
    for (auto& emitter : m_emitters)
    {
        trigger_emitter_at(emitter, position);
    }
}

void ParticleSystem::trigger_from_to(Vector2 from, Vector2 to)
{
    for (auto& emitter : m_emitters)
    {
        trigger_emitter_from_to(emitter, from, to);
    }
}

auto ParticleSystem::active_particle_count() const -> size_t
{
    return std::accumulate(m_emitters.cbegin(),
                           m_emitters.cend(),
                           size_t(0),
                           [](size_t count, const auto& emitter) {
                               return count + emitter.particles().size();
                           });
}

auto ParticleSystem::emitters() -> std::span<ParticleEmitter>
{
    return m_emitters;
}

auto ParticleSystem::emitters() const -> std::span<const ParticleEmitter>
{
    return m_emitters;
}

} // namespace cer
