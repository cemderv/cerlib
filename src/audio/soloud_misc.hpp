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

namespace cer
{
enum class Waveform;

// Class that handles aligned allocations to support vectorized operations
class AlignedFloatBuffer
{
  public:
    AlignedFloatBuffer() = default;

    // Allocate and align buffer
    explicit AlignedFloatBuffer(size_t aFloats);

    AlignedFloatBuffer(const AlignedFloatBuffer& aBuffer)            = delete;
    AlignedFloatBuffer& operator=(const AlignedFloatBuffer& aBuffer) = delete;

    AlignedFloatBuffer(AlignedFloatBuffer&& aBuffer) noexcept            = default;
    AlignedFloatBuffer& operator=(AlignedFloatBuffer&& aBuffer) noexcept = default;

    // Clear data to zero.
    void clear();

    float*                           mData = nullptr; // aligned pointer
    std::unique_ptr<unsigned char[]> mBasePtr;
    size_t                           mFloats = 0; // size of buffer (w/out padding)
};

// Lightweight class that handles small aligned buffer to support vectorized operations
class TinyAlignedFloatBuffer
{
  public:
    float*        mData; // aligned pointer
    unsigned char mActualData[sizeof(float) * 16 + 16];

    // ctor
    TinyAlignedFloatBuffer();
};

// Generate a waveform.
float generateWaveform(Waveform aWaveform, float p);

// WELL512 random
class Prg
{
  public:
    // random generator
    Prg();
    size_t mState[16];
    size_t mIndex;
    size_t rand();
    float  rand_float();
    void   srand(int aSeed);
};
}; // namespace cer
