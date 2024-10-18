/*
SoLoud audio engine
Copyright (c) 2013-2021 Jari Komppa

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

#include "audio/AudioDevice.hpp"
#include "audio/soloud_bus.hpp"
#include "soloud_filter.hpp"

namespace cer
{
DuckFilterInstance::DuckFilterInstance(DuckFilter* aParent)
{
    initParams(4);
    mParam[DuckFilter::ONRAMP]  = aParent->mOnRamp;
    mParam[DuckFilter::OFFRAMP] = aParent->mOffRamp;
    mParam[DuckFilter::LEVEL]   = aParent->mLevel;
    mListenTo                   = aParent->mListenTo;
    mEngine                     = aParent->mEngine;
    mCurrentLevel               = 1;
}

void DuckFilterInstance::filter(float* aBuffer,
                                size_t aSamples,
                                size_t aBufferSize,
                                size_t aChannels,
                                float  aSamplerate,
                                double aTime)
{
    updateParams(aTime);

    auto onramp_step = 1.0f;
    if (mParam[DuckFilter::ONRAMP] > 0.01f)
    {
        onramp_step =
            (1.0f - mParam[DuckFilter::LEVEL]) / (mParam[DuckFilter::ONRAMP] * aSamplerate);
    }

    auto offramp_step = 1.0f;
    if (mParam[DuckFilter::OFFRAMP] > 0.01f)
    {
        offramp_step =
            (1.0f - mParam[DuckFilter::LEVEL]) / (mParam[DuckFilter::OFFRAMP] * aSamplerate);
    }

    auto soundOn = false;
    if (mEngine)
    {
        const auto voice_num = mEngine->getVoiceFromHandle_internal(mListenTo);
        if (voice_num != -1)
        {
            const auto bi = std::static_pointer_cast<BusInstance>(mEngine->m_voice[voice_num]);

            auto v = 0.0f;
            for (size_t i = 0; i < bi->mChannels; ++i)
            {
                v += bi->mVisualizationChannelVolume[i];
            }

            if (v > 0.01f)
            {
                soundOn = true;
            }
        }
    }

    auto level = mCurrentLevel;
    for (size_t j = 0; j < aChannels; ++j)
    {
        level             = mCurrentLevel;
        const auto bchofs = j * aBufferSize;

        for (size_t i = 0; i < aSamples; ++i)
        {
            if (soundOn && level > mParam[DuckFilter::LEVEL])
            {
                level -= onramp_step;
            }

            if (!soundOn && level < 1)
            {
                level += offramp_step;
            }

            if (level < mParam[DuckFilter::LEVEL])
            {
                level = mParam[DuckFilter::LEVEL];
            }

            if (level > 1)
            {
                level = 1;
            }

            aBuffer[i + bchofs] +=
                (-aBuffer[i + bchofs] + aBuffer[i + bchofs] * level) * mParam[DuckFilter::WET];
        }
    }

    mCurrentLevel = level;
}

std::shared_ptr<FilterInstance> DuckFilter::createInstance()
{
    return std::make_shared<DuckFilterInstance>(this);
}
} // namespace cer
