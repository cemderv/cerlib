/*
SoLoud audio engine
Copyright (c) 2020 Jari Komppa

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
   distribution.
*/

#include "audio/Noise.hpp"

namespace cer
{
NoiseInstance::NoiseInstance(const Noise* aParent)
{
    for (int i = 0; i < 10; ++i)
    {
        m_octave_scale[i] = aParent->octave_scale[i];
    }
    m_prg.srand(0xfade);
}

auto NoiseInstance::audio(float*                  buffer,
                          size_t                  samples_to_read,
                          [[maybe_unused]] size_t buffer_size) -> size_t
{
    struct Octave
    {
        int   step  = 0;
        float value = 0.0f;
    };

    constexpr size_t octave_count = 10;

    auto octaves     = std::array<Octave, octave_count>{};
    auto total_scale = 0.0f;

    for (size_t j = 0; j < octave_count; ++j)
    {
        octaves[j] = {
            .step  = 0,
            .value = float(1 << j),
        };

        total_scale += m_octave_scale[j];
    }

    for (size_t i = 0; i < samples_to_read; ++i)
    {
        buffer[i] = m_prg.rand_float() - 0.5f;

        for (size_t j = 0; j < octave_count; ++j)
        {
            auto& octave = octaves[j];
            ++octave.step;

            if (octave.step > (1 << (j + 1)))
            {
                octave.step  = 0;
                octave.value = m_prg.rand_float() - 0.5f;
            }

            buffer[i] += octave.value * m_octave_scale[j];
        }

        buffer[i] *= 1.0f / total_scale;
    }

    return samples_to_read;
}

auto NoiseInstance::has_ended() -> bool
{
    return false;
}

Noise::Noise()
{
    set_type(NoiseType::White);
}

Noise::~Noise()
{
    stop();
}

auto Noise::create_instance() -> std::shared_ptr<AudioSourceInstance>
{
    return std::make_shared<NoiseInstance>(this);
}

void Noise::set_type(NoiseType type)
{
    switch (type)
    {
        default:
        case NoiseType::White: octave_scale = {1, 0, 0, 0, 0, 0, 0, 0, 0, 0}; break;
        case NoiseType::Pink: octave_scale = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1}; break;
        case NoiseType::Brownish: octave_scale = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}; break;
        case NoiseType::Blueish: octave_scale = {10, 9, 8, 7, 6, 5, 4, 3, 2, 1}; break;
    }
}
}; // namespace cer