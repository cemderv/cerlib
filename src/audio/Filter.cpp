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
void FilterInstance::init_params(int param_count)
{
    m_param_count  = param_count;
    m_params       = std::make_unique<float[]>(m_param_count);
    m_param_faders = std::make_unique<Fader[]>(m_param_count);

    m_params[0] = 1; // set 'wet' to 1
}

void FilterInstance::update_params(double time)
{
    for (size_t i = 0; i < m_param_count; ++i)
    {
        if (m_param_faders[i].mActive > 0)
        {
            m_params_changed |= size_t(1) << i;
            m_params[i] = m_param_faders[i].get(time);
        }
    }
}

void FilterInstance::set_filter_parameter(size_t attribute_id, float value)
{
    if (attribute_id >= m_param_count)
    {
        return;
    }

    m_param_faders[attribute_id].mActive = 0;
    m_params[attribute_id]               = value;
    m_params_changed |= size_t(1) << attribute_id;
}

void FilterInstance::fade_filter_parameter(size_t attribute_id,
                                           float  to,
                                           double time,
                                           double start_time)
{
    if (attribute_id >= m_param_count || time <= 0 || to == m_params[attribute_id])
    {
        return;
    }

    m_param_faders[attribute_id].set(m_params[attribute_id], to, time, start_time);
}

void FilterInstance::oscillate_filter_parameter(
    size_t attribute_id, float from, float to, double time, double start_time)
{
    if (attribute_id >= m_param_count || time <= 0 || from == to)
    {
        return;
    }

    m_param_faders[attribute_id].setLFO(from, to, time, start_time);
}

auto FilterInstance::filter_parameter(size_t attribute_id) -> float
{
    if (attribute_id >= m_param_count)
    {
        return 0;
    }

    return m_params[attribute_id];
}

void FilterInstance::filter(const FilterArgs& args)
{
    for (size_t i = 0; i < args.channels; ++i)
    {
        filter_channel(FilterChannelArgs{
            .buffer        = args.buffer + (i * args.buffer_size),
            .samples       = args.samples,
            .sample_rate   = args.sample_rate,
            .time          = args.time,
            .channel       = i,
            .channel_count = args.channels,
        });
    }
}

void FilterInstance::filter_channel([[maybe_unused]] const FilterChannelArgs& args)
{
}
}; // namespace cer
