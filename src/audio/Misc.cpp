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

#include "audio/Misc.hpp"

namespace cer
{
Prg::Prg()
{
    srand(0);
}

void Prg::srand(int seed)
{
    for (size_t i = 0; i < 16; ++i)
    {
        m_state[i] = seed + i * seed + i;
    }
}

// WELL512 implementation, public domain by Chris Lomont
auto Prg::rand() -> size_t
{
    auto         a = m_state[m_index];
    auto         c = m_state[(m_index + 13) & 15];
    const size_t b = a ^ c ^ (a << 16) ^ (c << 15);
    c              = m_state[(m_index + 9) & 15];
    c ^= (c >> 11);
    a = m_state[m_index] = b ^ c;
    const size_t d       = a ^ ((a << 5) & 0xDA442D24UL);
    m_index              = (m_index + 15) & 15;
    a                    = m_state[m_index];
    m_state[m_index]     = a ^ b ^ d ^ (a << 2) ^ (b << 18) ^ (c << 28);

    return m_state[m_index];
}

auto Prg::rand_float() -> float
{
    return float(rand()) * 2.3283064365386963e-10f;
}
}; // namespace cer
