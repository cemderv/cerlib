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

#include "soloud_filter.hpp"
#include <cmath>

namespace cer
{
LofiFilterInstance::LofiFilterInstance(LofiFilter* aParent)
{
    mParent = aParent;
    FilterInstance::initParams(3);
    mParam[SAMPLERATE] = aParent->mSampleRate;
    mParam[BITDEPTH]   = aParent->mBitdepth;
}

void LofiFilterInstance::filterChannel(float* aBuffer,
                                       size_t aSamples,
                                       float  aSamplerate,
                                       double aTime,
                                       size_t aChannel,
                                       size_t /*aChannels*/)
{
    updateParams(aTime);

    size_t i;
    for (i = 0; i < aSamples; ++i)
    {
        if (mChannelData[aChannel].mSamplesToSkip <= 0)
        {
            mChannelData[aChannel].mSamplesToSkip += (aSamplerate / mParam[SAMPLERATE]) - 1;

            const auto q = pow(2.0f, mParam[BITDEPTH]);

            mChannelData[aChannel].mSample = floor(q * aBuffer[i]) / q;
        }
        else
        {
            mChannelData[aChannel].mSamplesToSkip--;
        }
        aBuffer[i] += (mChannelData[aChannel].mSample - aBuffer[i]) * mParam[WET];
    }
}

std::shared_ptr<FilterInstance> LofiFilter::createInstance()
{
    return std::make_shared<LofiFilterInstance>(this);
}
} // namespace cer
