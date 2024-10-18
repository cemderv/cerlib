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

#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>

#if !defined(DISABLE_SIMD)
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386) || defined(_M_IX86)
#define SOLOUD_SSE_INTRINSICS
#endif
#endif

// Typedefs have to be made before the includes, as the
// includes depend on them.
namespace cer
{
class AudioDevice;

// Maximum number of filters per stream
static constexpr size_t filters_per_stream = 8;

// Number of samples to process on one go
static constexpr size_t sample_granularity = 512;

// Maximum number of concurrent voices (hard limit is 4095)
static constexpr size_t voice_count = 1024;

// 1)mono, 2)stereo 4)quad 6)5.1 8)7.1
static constexpr size_t max_channels = 8;

using mutexCallFunction    = void (*)(void*);
using soloudCallFunction   = void (*)(AudioDevice*);
using soloudResultFunction = bool (*)(AudioDevice*);
using SoundHandle          = size_t;

enum class Waveform
{
    Square = 0,
    Saw,
    Sin,
    Triangle,
    Bounce,
    Jaws,
    Humps,
    FSquare,
    FSaw
};

enum class Resampler
{
    Point,
    Linear,
    CatmullRom
};

enum class AttenuationModel
{
    // No attenuation
    NoAttenuation = 0,
    // Inverse distance attenuation model
    InverseDistance = 1,
    // Linear distance attenuation model
    LinearDistance = 2,
    // Exponential distance attenuation model
    ExponentialDistance = 3
};

struct EngineFlags
{
    bool clip_roundoff : 1          = true;
    bool enable_visualization : 1   = false;
    bool no_fpu_register_change : 1 = false;
};

// Default resampler for both main and bus mixers
static constexpr auto default_resampler = Resampler::Linear;
}; // namespace cer
