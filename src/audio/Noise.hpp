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

#pragma once

#include "audio/AudioSource.hpp"
#include "audio/Misc.hpp"

namespace cer
{
class Noise;

class NoiseInstance final : public AudioSourceInstance
{
  public:
    explicit NoiseInstance(const Noise* aParent);

    size_t getAudio(float* aBuffer, size_t aSamplesToRead, size_t aBufferSize) override;
    bool   hasEnded() override;

    float mOctaveScale[10];
    Prg   mPrg;
};

class Noise final : public AudioSource
{
  public:
    enum NOISETYPES
    {
        WHITE = 0,
        PINK,
        BROWNISH,
        BLUEISH
    };

    Noise();

    void setOctaveScale(float aOct0,
                        float aOct1,
                        float aOct2,
                        float aOct3,
                        float aOct4,
                        float aOct5,
                        float aOct6,
                        float aOct7,
                        float aOct8,
                        float aOct9);
    void setType(int aType);

    ~Noise() override;

  public:
    std::shared_ptr<AudioSourceInstance> createInstance() override;
    float                                mOctaveScale[10];
};
}; // namespace cer
