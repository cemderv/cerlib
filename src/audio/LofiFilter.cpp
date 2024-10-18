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
*/

#include "audio/Filter.hpp"
#include <cmath>

namespace cer
{
LofiFilterInstance::LofiFilterInstance(LofiFilter* aParent)
{
    mParent = aParent;
    FilterInstance::init_params(3);
    m_params[SAMPLERATE] = aParent->mSampleRate;
    m_params[BITDEPTH]   = aParent->mBitdepth;
}

void LofiFilterInstance::filter_channel(const FilterChannelArgs& args)
{
    update_params(args.time);

    for (size_t i = 0; i < args.samples; ++i)
    {
        if (mChannelData[args.channel].mSamplesToSkip <= 0)
        {
            mChannelData[args.channel].mSamplesToSkip +=
                (args.sample_rate / m_params[SAMPLERATE]) - 1;

            const auto q = pow(2.0f, m_params[BITDEPTH]);

            mChannelData[args.channel].mSample = floor(q * args.buffer[i]) / q;
        }
        else
        {
            mChannelData[args.channel].mSamplesToSkip--;
        }

        args.buffer[i] += (mChannelData[args.channel].mSample - args.buffer[i]) * m_params[WET];
    }
}

std::shared_ptr<FilterInstance> LofiFilter::create_instance()
{
    return std::make_shared<LofiFilterInstance>(this);
}
} // namespace cer
