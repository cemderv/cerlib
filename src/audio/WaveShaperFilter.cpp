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
WaveShaperFilterInstance::WaveShaperFilterInstance(WaveShaperFilter* parent)
    : mParent(parent)
{
    FilterInstance::init_params(2);
    m_params[WaveShaperFilter::AMOUNT] = mParent->mAmount;
}

void WaveShaperFilterInstance::filter_channel(const FilterChannelArgs& args)
{
    update_params(args.time);

    auto k = 0.0f;

    if (m_params[1] == 1)
    {
        k = 2 * m_params[WaveShaperFilter::AMOUNT] / 0.01f;
    }
    else
    {
        k = 2 * m_params[WaveShaperFilter::AMOUNT] / (1 - m_params[1]);
    }

    for (size_t i = 0; i < args.samples; ++i)
    {
        const auto dry = args.buffer[i];
        const auto wet = (1 + k) * args.buffer[i] / (1 + k * std::abs(args.buffer[i]));

        args.buffer[i] += (wet - dry) * m_params[WaveShaperFilter::WET];
    }
}

SharedPtr<FilterInstance> WaveShaperFilter::create_instance()
{
    return std::make_shared<WaveShaperFilterInstance>(this);
}
} // namespace cer
