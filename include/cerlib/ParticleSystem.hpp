// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <cerlib/ParticleEmitter.hpp>
#include <span>
#include <vector>

namespace cer
{
class ParticleSystem
{
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

    auto active_particle_count() const -> size_t;

    auto emitters() -> std::span<ParticleEmitter>;

    auto emitters() const -> std::span<const ParticleEmitter>;

  private:
    std::vector<ParticleEmitter> m_emitters;
};
} // namespace cer
