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

#include "audio/Common.hpp"
#include "audio/Filter.hpp"
#include "audio/Misc.hpp"

namespace cer
{
RobotizeFilterInstance::RobotizeFilterInstance(RobotizeFilter* aParent)
{
    mParent = aParent;
    FilterInstance::initParams(3);
    mParam[FREQ] = aParent->mFreq;
    mParam[WAVE] = float(aParent->mWave);
}

void RobotizeFilterInstance::filterChannel(float* aBuffer,
                                           size_t aSamples,
                                           float  aSamplerate,
                                           time_t aTime,
                                           size_t aChannel,
                                           size_t aChannels)
{
    const auto period = int(aSamplerate / mParam[FREQ]);
    const auto start  = int(aTime * aSamplerate) % period;

    for (size_t i = 0; i < aSamples; ++i)
    {
        float s    = aBuffer[i];
        float wpos = ((start + i) % period) / (float)period;
        s *= generateWaveform(Waveform(int(mParam[WAVE])), wpos) + 0.5f;
        aBuffer[i] += (s - aBuffer[i]) * mParam[WET];
    }
}

std::shared_ptr<FilterInstance> RobotizeFilter::createInstance()
{
    return std::make_shared<RobotizeFilterInstance>(this);
}
} // namespace cer
