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

#include <algorithm>

#include "audio/Filter.hpp"

namespace cer
{
EchoFilterInstance::EchoFilterInstance(const EchoFilter* parent)
{
    FilterInstance::init_params(4);

    m_params[EchoFilter::DELAY]  = parent->delay;
    m_params[EchoFilter::DECAY]  = parent->decay;
    m_params[EchoFilter::FILTER] = parent->filter;
}

void EchoFilterInstance::filter(const FilterArgs& args)
{
    update_params(args.time);
    if (m_buffer == nullptr)
    {
        // We only know channels and sample rate at this point.. not really optimal
        m_buffer_max_size = int(ceil(m_params[EchoFilter::DELAY] * args.sample_rate));
        m_buffer          = std::make_unique<float[]>(m_buffer_max_size * args.channels);
    }

    m_buffer_size = int(ceil(m_params[EchoFilter::DELAY] * args.sample_rate));
    m_buffer_size = std::min(m_buffer_size, m_buffer_max_size);

    auto prevofs = (m_offset + m_buffer_size - 1) % m_buffer_size;

    for (size_t i = 0; i < args.samples; ++i)
    {
        for (size_t j = 0; j < args.channels; ++j)
        {
            const auto chofs  = j * m_buffer_size;
            const auto bchofs = j * args.buffer_size;

            m_buffer[m_offset + chofs] =
                m_params[EchoFilter::FILTER] * m_buffer[prevofs + chofs] +
                (1 - m_params[EchoFilter::FILTER]) * m_buffer[m_offset + chofs];

            const auto n =
                args.buffer[i + bchofs] + m_buffer[m_offset + chofs] * m_params[EchoFilter::DECAY];

            m_buffer[m_offset + chofs] = n;

            args.buffer[i + bchofs] += (n - args.buffer[i + bchofs]) * m_params[EchoFilter::WET];
        }

        prevofs  = m_offset;
        m_offset = (m_offset + 1) % m_buffer_size;
    }
}

auto EchoFilter::create_instance() -> SharedPtr<FilterInstance>
{
    return std::make_shared<EchoFilterInstance>(this);
}
} // namespace cer
