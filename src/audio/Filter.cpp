/*
SoLoud audio engine
Copyright (c) 2013-2014 Jari Komppa

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
#include "audio/Fader.hpp"

namespace cer
{
void FilterInstance::initParams(int aNumParams)
{
    mNumParams  = aNumParams;
    mParam      = std::make_unique<float[]>(mNumParams);
    mParamFader = std::make_unique<Fader[]>(mNumParams);

    mParam[0] = 1; // set 'wet' to 1
}

void FilterInstance::updateParams(double aTime)
{
    for (size_t i = 0; i < mNumParams; ++i)
    {
        if (mParamFader[i].mActive > 0)
        {
            mParamChanged |= 1 << i;
            mParam[i] = mParamFader[i].get(aTime);
        }
    }
}

void FilterInstance::setFilterParameter(size_t aAttributeId, float aValue)
{
    if (aAttributeId >= mNumParams)
        return;

    mParamFader[aAttributeId].mActive = 0;
    mParam[aAttributeId]              = aValue;
    mParamChanged |= 1 << aAttributeId;
}

void FilterInstance::fadeFilterParameter(size_t aAttributeId,
                                         float  aTo,
                                         double aTime,
                                         double aStartTime)
{
    if (aAttributeId >= mNumParams || aTime <= 0 || aTo == mParam[aAttributeId])
        return;

    mParamFader[aAttributeId].set(mParam[aAttributeId], aTo, aTime, aStartTime);
}

void FilterInstance::oscillateFilterParameter(
    size_t aAttributeId, float aFrom, float aTo, double aTime, double aStartTime)
{
    if (aAttributeId >= mNumParams || aTime <= 0 || aFrom == aTo)
        return;

    mParamFader[aAttributeId].setLFO(aFrom, aTo, aTime, aStartTime);
}

float FilterInstance::getFilterParameter(size_t aAttributeId)
{
    if (aAttributeId >= mNumParams)
        return 0;

    return mParam[aAttributeId];
}

void FilterInstance::filter(float* aBuffer,
                            size_t aSamples,
                            size_t aBufferSize,
                            size_t aChannels,
                            float  aSamplerate,
                            double aTime)
{
    for (size_t i = 0; i < aChannels; ++i)
    {
        filterChannel(aBuffer + i * aBufferSize, aSamples, aSamplerate, aTime, i, aChannels);
    }
}

void FilterInstance::filterChannel(float* /*aBuffer*/,
                                   size_t /*aSamples*/,
                                   float /*aSamplerate*/,
                                   double /*aTime*/,
                                   size_t /*aChannel*/,
                                   size_t /*aChannels*/)
{
}

}; // namespace cer
