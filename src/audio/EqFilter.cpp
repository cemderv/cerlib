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
    FilterInstance::init_params(9);
    m_params[BAND1] = aParent->mVolume[BAND1 - BAND1];
    m_params[BAND2] = aParent->mVolume[BAND2 - BAND1];
    m_params[BAND3] = aParent->mVolume[BAND3 - BAND1];
    m_params[BAND4] = aParent->mVolume[BAND4 - BAND1];
    m_params[BAND5] = aParent->mVolume[BAND5 - BAND1];
    m_params[BAND6] = aParent->mVolume[BAND6 - BAND1];
    m_params[BAND7] = aParent->mVolume[BAND7 - BAND1];
    m_params[BAND8] = aParent->mVolume[BAND8 - BAND1];
}

static auto catmull_rom(float t, float p0, float p1, float p2, float p3) -> float
{
    return 0.5f * ((2 * p1) + (-p0 + p2) * t + (2 * p0 - 5 * p1 + 4 * p2 - p3) * t * t +
                   (-p0 + 3 * p1 - 3 * p2 + p3) * t * t * t);
}

void EqFilterInstance::fft_filter_channel(const FilterChannelArgs& args)
{
    comp2MagPhase(args.buffer, args.samples / 2);

    for (size_t p = 0; p < args.samples / 2; p++)
    {
        const auto i  = int(floor(sqrt(p / float(args.samples / 2)) * (args.samples / 2)));
        const auto p2 = int(i / (args.samples / 16));

        auto p1 = p2 - 1;
        auto p0 = p1 - 1;
        auto p3 = p2 + 1;

        p1 = std::max(p1, 0);
        p0 = std::max(p0, 0);
        p3 = std::min(p3, 7);

        const auto v = float(i % (args.samples / 16)) / float(args.samples / 16);

        args.buffer[p * 2] *=
            catmull_rom(v, m_params[p0 + 1], m_params[p1 + 1], m_params[p2 + 1], m_params[p3 + 1]);
    }

    memset(args.buffer + args.samples, 0, sizeof(float) * args.samples);
    magPhase2Comp(args.buffer, args.samples / 2);
}

EqFilter::EqFilter()
{
    std::ranges::fill(mVolume, 1.0f);
}

std::shared_ptr<FilterInstance> EqFilter::create_instance()
{
    return std::make_shared<EqFilterInstance>(this);
}
} // namespace cer
