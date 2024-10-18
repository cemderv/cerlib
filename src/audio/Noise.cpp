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

#include "audio/Noise.hpp"

namespace cer
{
NoiseInstance::NoiseInstance(const Noise* aParent)
{
    for (int i = 0; i < 10; ++i)
    {
        m_octave_scale[i] = aParent->octave_scale[i];
    }
    m_prg.srand(0xfade);
}

size_t NoiseInstance::audio(float* aBuffer, size_t aSamplesToRead, size_t /*aBufferSize*/)
{
    int   octavestep[10];
    float octavevalue[10];
    float totalscale = 0;
    for (int j = 0; j < 10; ++j)
    {
        octavevalue[j] = 0;
        octavestep[j]  = 1 << j;
        totalscale += m_octave_scale[j];
    }

    for (size_t i = 0; i < aSamplesToRead; ++i)
    {
        aBuffer[i] = m_prg.rand_float() - 0.5f;
        for (int j = 0; j < 10; ++j)
        {
            octavestep[j]++;
            if (octavestep[j] > (1 << (j + 1)))
            {
                octavestep[j]  = 0;
                octavevalue[j] = m_prg.rand_float() - 0.5f;
            }
            aBuffer[i] += octavevalue[j] * m_octave_scale[j];
        }
        aBuffer[i] *= 1.0f / totalscale;
    }

    return aSamplesToRead;
}

bool NoiseInstance::has_ended()
{
    return false;
}

void Noise::setOctaveScale(float aOct0,
                           float aOct1,
                           float aOct2,
                           float aOct3,
                           float aOct4,
                           float aOct5,
                           float aOct6,
                           float aOct7,
                           float aOct8,
                           float aOct9)
{
    octave_scale[0] = aOct0;
    octave_scale[1] = aOct1;
    octave_scale[2] = aOct2;
    octave_scale[3] = aOct3;
    octave_scale[4] = aOct4;
    octave_scale[5] = aOct5;
    octave_scale[6] = aOct6;
    octave_scale[7] = aOct7;
    octave_scale[8] = aOct8;
    octave_scale[9] = aOct9;
}

void Noise::setType(int aType)
{
    switch (aType)
    {
        default:
        case White: setOctaveScale(1, 0, 0, 0, 0, 0, 0, 0, 0, 0); break;
        case Pink: setOctaveScale(1, 1, 1, 1, 1, 1, 1, 1, 1, 1); break;
        case Brownish: setOctaveScale(1, 2, 3, 4, 5, 6, 7, 8, 9, 10); break;
        case Blueish: setOctaveScale(10, 9, 8, 7, 6, 5, 4, 3, 2, 1); break;
    }
}

Noise::Noise()
{
    base_sample_rate = 44100;
    setType(White);
}

Noise::~Noise()
{
    stop();
}

std::shared_ptr<AudioSourceInstance> Noise::create_instance()
{
    return std::make_shared<NoiseInstance>(this);
}
}; // namespace cer