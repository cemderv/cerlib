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
#include <cstring>

namespace cer
{
FlangerFilterInstance::FlangerFilterInstance(FlangerFilter* aParent)
{
    m_parent      = aParent;
    m_buffer      = 0;
    m_buffer_size = 0;
    m_offset      = 0;
    m_index       = 0;
    FilterInstance::init_params(3);
    m_params[FlangerFilter::WET]   = 1;
    m_params[FlangerFilter::FREQ]  = m_parent->m_freq;
    m_params[FlangerFilter::DELAY] = m_parent->m_delay;
}

void FlangerFilterInstance::filter(const FilterArgs& args)
{
    update_params(args.time);

    if (m_buffer_size < m_params[FlangerFilter::DELAY] * args.sample_rate)
    {
        m_buffer_size = int(ceil(m_params[FlangerFilter::DELAY] * args.sample_rate));
        m_buffer      = std::make_unique<float[]>(m_buffer_size * args.channels);
    }

    const int    maxsamples = int(ceil(m_params[FlangerFilter::DELAY] * args.sample_rate));
    const double inc        = m_params[FlangerFilter::FREQ] * M_PI * 2 / args.sample_rate;
    for (size_t i = 0; i < args.channels; ++i)
    {
        const auto mbofs = i * m_buffer_size;
        auto       abofs = i * args.buffer_size;
        for (size_t j = 0; j < args.samples; ++j, ++abofs)
        {
            const auto delay = int(floor(maxsamples * (1 + cos(m_index)))) / 2;
            m_index += inc;
            m_buffer[mbofs + m_offset % m_buffer_size] = args.buffer[abofs];
            const auto n =
                0.5f * (args.buffer[abofs] +
                        m_buffer[mbofs + (m_buffer_size - delay + m_offset) % m_buffer_size]);
            m_offset++;
            args.buffer[abofs] += (n - args.buffer[abofs]) * m_params[FlangerFilter::WET];
        }
        m_offset -= args.samples;
    }
    m_offset += args.samples;
    m_offset %= m_buffer_size;
}

FlangerFilter::FlangerFilter()
{
    m_delay = 0.005f;
    m_freq  = 10;
}

std::shared_ptr<FilterInstance> FlangerFilter::create_instance()
{
    return std::make_shared<FlangerFilterInstance>(this);
}
} // namespace cer
