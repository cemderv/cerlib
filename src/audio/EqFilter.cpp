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

#include "audio/Common.hpp"
#include "audio/Filter.hpp"
#include <algorithm>
#include <cstring>

namespace cer
{
EqFilterInstance::EqFilterInstance(EqFilter* aParent)
{
    mParent = aParent;
    FilterInstance::initParams(9);
    mParam[BAND1] = aParent->mVolume[BAND1 - BAND1];
    mParam[BAND2] = aParent->mVolume[BAND2 - BAND1];
    mParam[BAND3] = aParent->mVolume[BAND3 - BAND1];
    mParam[BAND4] = aParent->mVolume[BAND4 - BAND1];
    mParam[BAND5] = aParent->mVolume[BAND5 - BAND1];
    mParam[BAND6] = aParent->mVolume[BAND6 - BAND1];
    mParam[BAND7] = aParent->mVolume[BAND7 - BAND1];
    mParam[BAND8] = aParent->mVolume[BAND8 - BAND1];
}

static float catmullrom(float t, float p0, float p1, float p2, float p3)
{
    return 0.5f * ((2 * p1) + (-p0 + p2) * t + (2 * p0 - 5 * p1 + 4 * p2 - p3) * t * t +
                   (-p0 + 3 * p1 - 3 * p2 + p3) * t * t * t);
}

void EqFilterInstance::fftFilterChannel(float* aFFTBuffer,
                                        size_t aSamples,
                                        float /*aSamplerate*/,
                                        time_t /*aTime*/,
                                        size_t /*aChannel*/,
                                        size_t /*aChannels*/)
{
    comp2MagPhase(aFFTBuffer, aSamples / 2);

    for (size_t p = 0; p < aSamples / 2; p++)
    {
        int i  = int(floor(sqrt(p / float(aSamples / 2)) * (aSamples / 2)));
        int p2 = (i / (aSamples / 16));
        int p1 = p2 - 1;
        int p0 = p1 - 1;
        int p3 = p2 + 1;

        if (p1 < 0)
            p1 = 0;
        if (p0 < 0)
            p0 = 0;
        if (p3 > 7)
            p3 = 7;

        const auto v = float(i % (aSamples / 16)) / float(aSamples / 16);

        aFFTBuffer[p * 2] *=
            catmullrom(v, mParam[p0 + 1], mParam[p1 + 1], mParam[p2 + 1], mParam[p3 + 1]);
    }

    memset(aFFTBuffer + aSamples, 0, sizeof(float) * aSamples);
    magPhase2Comp(aFFTBuffer, aSamples / 2);
}

EqFilter::EqFilter()
{
    std::ranges::fill(mVolume, 1.0f);
}

std::shared_ptr<FilterInstance> EqFilter::createInstance()
{
    return std::make_shared<EqFilterInstance>(this);
}
} // namespace cer
