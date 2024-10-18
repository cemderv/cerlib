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
void BiquadResonantFilterInstance::calcBQRParams()
{
    mDirty = 0;

    const auto omega     = float(2.0f * M_PI * m_params[Frequency] / mSamplerate);
    const auto sin_omega = sin(omega);
    const auto cos_omega = cos(omega);
    const auto alpha     = sin_omega / (2.0f * m_params[Resonance]);
    const auto scalar    = 1.0f / (1.0f + alpha);

    switch (int(m_params[Type]))
    {
        case BiquadResonantFilter::LOWPASS: {
            mA0 = 0.5f * (1.0f - cos_omega) * scalar;
            mA1 = (1.0f - cos_omega) * scalar;
            mA2 = mA0;
            mB1 = -2.0f * cos_omega * scalar;
            mB2 = (1.0f - alpha) * scalar;
            break;
        }
        case BiquadResonantFilter::HIGHPASS: {
            mA0 = 0.5f * (1.0f + cos_omega) * scalar;
            mA1 = -(1.0f + cos_omega) * scalar;
            mA2 = mA0;
            mB1 = -2.0f * cos_omega * scalar;
            mB2 = (1.0f - alpha) * scalar;
            break;
        }
        case BiquadResonantFilter::BANDPASS: {
            mA0 = alpha * scalar;
            mA1 = 0;
            mA2 = -mA0;
            mB1 = -2.0f * cos_omega * scalar;
            mB2 = (1.0f - alpha) * scalar;
            break;
        }
        default: break;
    }
}


BiquadResonantFilterInstance::BiquadResonantFilterInstance(BiquadResonantFilter* aParent)
    : mParent(aParent)
{
    FilterInstance::init_params(4);

    m_params[Resonance] = aParent->mResonance;
    m_params[Frequency] = aParent->mFrequency;
    m_params[Type]      = float(aParent->mFilterType);

    calcBQRParams();
}

void BiquadResonantFilterInstance::filter_channel(const FilterChannelArgs& args)
{
    const auto osamples = args.samples;

    if (args.channel == 0)
    {
        update_params(args.time);

        if (((m_params_changed & ((1 << Frequency) | (1 << Resonance) | (1 << Type))) != 0u) ||
            args.sample_rate != mSamplerate)
        {
            mSamplerate = args.sample_rate;
            calcBQRParams();
        }

        m_params_changed = 0;
    }

    auto& s = mState[args.channel];

    // make sure we access pairs of samples (one sample may be skipped)
    const auto new_sample_count = size_t(int64_t(args.samples) & ~1);

    int c = 0;

    for (size_t i = 0; i < new_sample_count; i += 2, ++c)
    {
        // Generate outputs by filtering inputs.
        const float x = args.buffer[c];
        s.mY2         = (mA0 * x) + (mA1 * s.mX1) + (mA2 * s.mX2) - (mB1 * s.mY1) - (mB2 * s.mY2);
        args.buffer[c] += (s.mY2 - args.buffer[c]) * m_params[Wet];

        ++c;

        // Permute filter operations to reduce data movement.
        // Just substitute variables instead of doing mX1=x, etc.
        s.mX2 = args.buffer[c];
        s.mY1 = (mA0 * s.mX2) + (mA1 * x) + (mA2 * s.mX1) - (mB1 * s.mY2) - (mB2 * s.mY1);
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
