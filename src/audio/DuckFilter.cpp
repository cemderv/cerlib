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
#include "audio/Bus.hpp"
#include "audio/Filter.hpp"

namespace cer
{
DuckFilterInstance::DuckFilterInstance(const DuckFilter* parent)
{
    FilterInstance::init_params(4);
    m_params[DuckFilter::ONRAMP]  = parent->mOnRamp;
    m_params[DuckFilter::OFFRAMP] = parent->mOffRamp;
    m_params[DuckFilter::LEVEL]   = parent->mLevel;
    m_listen_to                   = parent->mListenTo;
    m_engine                      = parent->mEngine;
    m_current_level               = 1;
}

void DuckFilterInstance::filter(const FilterArgs& args)
{
    update_params(args.time);

    auto onramp_step = 1.0f;
    if (m_params[DuckFilter::ONRAMP] > 0.01f)
    {
        onramp_step = (1.0f - m_params[DuckFilter::LEVEL]) /
                      (m_params[DuckFilter::ONRAMP] * args.sample_rate);
    }

    auto offramp_step = 1.0f;
    if (m_params[DuckFilter::OFFRAMP] > 0.01f)
    {
        offramp_step = (1.0f - m_params[DuckFilter::LEVEL]) /
                       (m_params[DuckFilter::OFFRAMP] * args.sample_rate);
    }

    auto sound_on = false;

    if (m_engine != nullptr)
    {
        const auto voice_num = m_engine->get_voice_from_handle_internal(m_listen_to);
        if (voice_num != -1)
        {
            const auto bi = std::static_pointer_cast<BusInstance>(m_engine->m_voice[voice_num]);

            auto v = 0.0f;
            for (size_t i = 0; i < bi->channel_count; ++i)
            {
                v += bi->m_visualization_channel_volume[i];
            }

            if (v > 0.01f)
            {
                sound_on = true;
            }
        }
    }

    auto level = m_current_level;
    for (size_t j = 0; j < args.channels; ++j)
    {
        level             = m_current_level;
        const auto bchofs = j * args.buffer_size;

        for (size_t i = 0; i < args.samples; ++i)
        {
            if (sound_on && level > m_params[DuckFilter::LEVEL])
            {
                level -= onramp_step;
            }

            if (!sound_on && level < 1)
            {
                level += offramp_step;
            }

            if (level < m_params[DuckFilter::LEVEL])
            {
                level = m_params[DuckFilter::LEVEL];
            }

            if (level > 1)
            {
                level = 1;
            }

            args.buffer[i + bchofs] +=
                (-args.buffer[i + bchofs] + args.buffer[i + bchofs] * level) *
                m_params[DuckFilter::WET];
        }
    }

    m_current_level = level;
}

auto DuckFilter::create_instance() -> std::shared_ptr<FilterInstance>
{
    return std::make_shared<DuckFilterInstance>(this);
}
} // namespace cer
