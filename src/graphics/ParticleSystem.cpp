// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "cerlib/ParticleSystem.hpp"

#include <numeric>

#include "cerlib/Logging.hpp"

namespace cer
{
static constexpr auto default_particles_buffer_capacity  = 300u;
static constexpr auto default_particle_reclaim_frequency = 1.0f / 60.0f;

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

void ParticleSystem::reclaim_expired_particles(EmitterData& data)
{
    auto expired_particle_count = uint32_t(0);

    auto& emitter = data.emitter;

    const auto time       = data.timer;
    const auto count      = data.active_particle_count;
    const auto duration_f = float(std::chrono::duration<double>{emitter.duration}.count());

    for (uint32_t i = 0; i < count; ++i)
    {
        const auto& particle = data.particle_buffer[i];

        if ((time - particle.inception) < duration_f)
        {
            break;
        }

        ++expired_particle_count;
    }

    data.active_particle_count -= expired_particle_count;

    assert(expired_particle_count < data.particle_buffer.size());

    std::copy_n(data.particle_buffer.cbegin() + expired_particle_count,
                data.active_particle_count,
                data.particle_buffer.begin());
}

void ParticleSystem::update_emitter(EmitterData& data, float elapsed_time)
{
    auto& emitter = data.emitter;

    data.timer += elapsed_time;
    data.time_since_last_reclaim += elapsed_time;

    if (data.active_particle_count == 0)
    {
        return;
    }

    if (data.time_since_last_reclaim > default_particle_reclaim_frequency)
    {
        reclaim_expired_particles(data);
        data.time_since_last_reclaim -= default_particle_reclaim_frequency;
    }

    const auto duration_f = float(std::chrono::duration<double>{emitter.duration}.count());

    if (data.active_particle_count > 0)
    {
        for (auto& particle : data.particle_buffer)
        {
            particle.age = (data.timer - particle.inception) / duration_f;
            particle.position += particle.velocity * elapsed_time;
        }

        for (const auto& modifier : emitter.modifiers)
        {
            execute_modifier(modifier,
                             elapsed_time,
                             std::span{data.particle_buffer.data(), data.active_particle_count});
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

void ParticleSystem::emit(EmitterData& data, Vector2 position, uint32_t count)
{
    auto& emitter = data.emitter;

    const auto previous_active_particle_count = data.active_particle_count;
    const auto new_active_particle_count      = previous_active_particle_count + count;

    // Ensure that the particle buffer is large enough.
    const auto particles_cap = data.particle_buffer.capacity();
    if (new_active_particle_count > particles_cap)
    {
        if (particles_cap == 0)
        {
            data.particle_buffer.resize(default_particles_buffer_capacity);
        }
        else
        {
            const auto new_capacity = size_t(double(particles_cap) * 1.5);

            data.particle_buffer.resize(size_t(max(new_capacity, new_active_particle_count)));
        }
    }

    for (uint32_t i = 0; i < count; ++i)
    {
        auto& particle = data.particle_buffer[previous_active_particle_count + i];

        particle.inception = data.timer;
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

    data.active_particle_count = new_active_particle_count;
}

void ParticleSystem::trigger_emitter_at(EmitterData& data, Vector2 position)
{
    emit(data, position, fastrand_uint(data.emitter.emission.quantity));
}

void ParticleSystem::trigger_emitter_from_to(EmitterData& data, Vector2 from, Vector2 to)
{
    const auto count     = fastrand_uint(data.emitter.emission.quantity);
    const auto direction = to - from;

    for (uint32_t i = 0; i < count; ++i)
    {
        const auto offset = direction * fastrand_float_zero_to_one();
        emit(data, from + offset, 1);
    }
}

ParticleSystem::ParticleSystem() = default;

ParticleSystem::ParticleSystem(std::vector<ParticleEmitter> emitters)
{
    m_emitters.reserve(emitters.size());

    for (auto&& emitter : emitters)
    {
        m_emitters.push_back(EmitterData{
            .emitter = std::move(emitter),
        });
    }
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
                           [](size_t count, const EmitterData& data) {
                               return count + data.active_particle_count;
                           });
}

auto ParticleSystem::active_particle_count(size_t index) const -> size_t
{
    return m_emitters.at(index).active_particle_count;
}

auto ParticleSystem::emitter_count() const -> size_t
{
    return m_emitters.size();
}

auto ParticleSystem::emitter_at(size_t index) -> ParticleEmitter&
{
    return m_emitters.at(index).emitter;
}

auto ParticleSystem::emitter_at(size_t index) const -> const ParticleEmitter&
{
    return m_emitters.at(index).emitter;
}
} // namespace cer
