/*
SoLoud audio engine
Copyright (c) 2013-2020 Jari Komppa

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

--

Based on "Using the Biquad Resonant Filter",
Phil Burk, Game Programming Gems 3, p. 606
*/

#include "audio/Filter.hpp"
#include <cmath>

namespace cer
{
void BiquadResonantFilterInstance::calc_bqr_params()
{
    m_dirty = 0;

    const auto omega     = float(2.0f * cer::pi * m_params[Frequency] / m_sample_rate);
    const auto sin_omega = sin(omega);
    const auto cos_omega = cos(omega);
    const auto alpha     = sin_omega / (2.0f * m_params[Resonance]);
    const auto scalar    = 1.0f / (1.0f + alpha);

    switch (BiquadResonantFilter::FilterType(int(m_params[Type])))
    {
        case BiquadResonantFilter::FilterType::LowPass: {
            m_a0 = 0.5f * (1.0f - cos_omega) * scalar;
            m_a1 = (1.0f - cos_omega) * scalar;
            m_a2 = m_a0;
            m_b1 = -2.0f * cos_omega * scalar;
            m_b2 = (1.0f - alpha) * scalar;
            break;
        }
        case BiquadResonantFilter::FilterType::HighPass: {
            m_a0 = 0.5f * (1.0f + cos_omega) * scalar;
            m_a1 = -(1.0f + cos_omega) * scalar;
            m_a2 = m_a0;
            m_b1 = -2.0f * cos_omega * scalar;
            m_b2 = (1.0f - alpha) * scalar;
            break;
        }
        case BiquadResonantFilter::FilterType::BandPass: {
            m_a0 = alpha * scalar;
            m_a1 = 0;
            m_a2 = -m_a0;
            m_b1 = -2.0f * cos_omega * scalar;
            m_b2 = (1.0f - alpha) * scalar;
            break;
        }
        default: break;
    }
}


BiquadResonantFilterInstance::BiquadResonantFilterInstance(BiquadResonantFilter* aParent)
    : m_parent(aParent)
{
    FilterInstance::init_params(4);

    m_params[Resonance] = aParent->resonance;
    m_params[Frequency] = aParent->frequency;
    m_params[Type]      = float(aParent->filter_type);

    calc_bqr_params();
}

void BiquadResonantFilterInstance::filter_channel(const FilterChannelArgs& args)
{
    const auto osamples = args.samples;

    if (args.channel == 0)
    {
        update_params(args.time);

        if (((m_params_changed & ((1 << Frequency) | (1 << Resonance) | (1 << Type))) != 0u) ||
            args.sample_rate != m_sample_rate)
        {
            m_sample_rate = args.sample_rate;
            calc_bqr_params();
        }

        m_params_changed = 0;
    }

    auto& s = m_state[args.channel];

    // make sure we access pairs of samples (one sample may be skipped)
    const auto new_sample_count = size_t(int64_t(args.samples) & ~1);

    int c = 0;

    for (size_t i = 0; i < new_sample_count; i += 2, ++c)
    {
        // Generate outputs by filtering inputs.
        const float x = args.buffer[c];
        s.mY2 = (m_a0 * x) + (m_a1 * s.mX1) + (m_a2 * s.mX2) - (m_b1 * s.mY1) - (m_b2 * s.mY2);
        args.buffer[c] += (s.mY2 - args.buffer[c]) * m_params[Wet];

        ++c;

        // Permute filter operations to reduce data movement.
        // Just substitute variables instead of doing mX1=x, etc.
        s.mX2 = args.buffer[c];
        s.mY1 = (m_a0 * s.mX2) + (m_a1 * x) + (m_a2 * s.mX1) - (m_b1 * s.mY2) - (m_b2 * s.mY1);
        args.buffer[c] += (s.mY1 - args.buffer[c]) * m_params[Wet];

        // Only move a little data.
        s.mX1 = s.mX2;
        s.mX2 = x;
    }

    // If we skipped a sample earlier, patch it by just copying the previous.
    if (osamples != args.samples)
    {
        args.buffer[c] = args.buffer[c - 1];
    }
}

auto BiquadResonantFilter::create_instance() -> std::shared_ptr<FilterInstance>
{
    return std::make_shared<BiquadResonantFilterInstance>(this);
}
} // namespace cer
