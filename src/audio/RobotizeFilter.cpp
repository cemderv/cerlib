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
RobotizeFilterInstance::RobotizeFilterInstance(RobotizeFilter* parent)
    : mParent(parent)
{
    FilterInstance::init_params(3);
    m_params[FREQ] = parent->mFreq;
    m_params[WAVE] = float(parent->mWave);
}

void RobotizeFilterInstance::filter_channel(const FilterChannelArgs& args)
{
    const auto period = int(args.sample_rate / m_params[FREQ]);
    const auto start  = int(args.time * args.sample_rate) % period;

    for (size_t i = 0; i < args.samples; ++i)
    {
        float       s    = args.buffer[i];
        const float wpos = ((start + i) % period) / float(period);

        s *= generate_waveform(Waveform(int(m_params[WAVE])), wpos) + 0.5f;
        args.buffer[i] += (s - args.buffer[i]) * m_params[WET];
    }
}

auto RobotizeFilter::create_instance() -> std::shared_ptr<FilterInstance>
{
    return std::make_shared<RobotizeFilterInstance>(this);
}
} // namespace cer
