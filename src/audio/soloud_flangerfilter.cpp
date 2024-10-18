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
#include <cstring>

namespace cer
{
FlangerFilterInstance::FlangerFilterInstance(FlangerFilter* aParent)
{
    mParent       = aParent;
    mBuffer       = 0;
    mBufferLength = 0;
    mOffset       = 0;
    mIndex        = 0;
    FilterInstance::initParams(3);
    mParam[FlangerFilter::WET]   = 1;
    mParam[FlangerFilter::FREQ]  = mParent->mFreq;
    mParam[FlangerFilter::DELAY] = mParent->mDelay;
}

void FlangerFilterInstance::filter(float* aBuffer,
                                   size_t aSamples,
                                   size_t aBufferSize,
                                   size_t aChannels,
                                   float  aSamplerate,
                                   double aTime)
{
    updateParams(aTime);

    if (mBufferLength < mParam[FlangerFilter::DELAY] * aSamplerate)
    {
        mBufferLength = int(ceil(mParam[FlangerFilter::DELAY] * aSamplerate));
        mBuffer       = std::make_unique<float[]>(mBufferLength * aChannels);
    }

    const int    maxsamples = int(ceil(mParam[FlangerFilter::DELAY] * aSamplerate));
    const double inc        = mParam[FlangerFilter::FREQ] * M_PI * 2 / aSamplerate;
    for (size_t i = 0; i < aChannels; ++i)
    {
        const auto mbofs = i * mBufferLength;
        auto       abofs = i * aBufferSize;
        for (size_t j = 0; j < aSamples; ++j, ++abofs)
        {
            const auto delay = int(floor(maxsamples * (1 + cos(mIndex)))) / 2;
            mIndex += inc;
            mBuffer[mbofs + mOffset % mBufferLength] = aBuffer[abofs];
            const auto n =
                0.5f * (aBuffer[abofs] +
                        mBuffer[mbofs + (mBufferLength - delay + mOffset) % mBufferLength]);
            mOffset++;
            aBuffer[abofs] += (n - aBuffer[abofs]) * mParam[FlangerFilter::WET];
        }
        mOffset -= aSamples;
    }
    mOffset += aSamples;
    mOffset %= mBufferLength;
}

FlangerFilter::FlangerFilter()
{
    mDelay = 0.005f;
    mFreq  = 10;
}

std::shared_ptr<FilterInstance> FlangerFilter::createInstance()
{
    return std::make_shared<FlangerFilterInstance>(this);
}
} // namespace cer
