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

#include "soloud_fft.hpp"
#include "soloud_internal.hpp"
#include "soloud_thread.hpp"
#include <cfloat> // _controlfp
#include <cmath> // sin
#include <cstring>


#ifdef SOLOUD_SSE_INTRINSICS
#include <xmmintrin.h>
#ifdef _M_IX86
#include <emmintrin.h>
#endif
#endif

namespace cer
{
AlignedFloatBuffer::AlignedFloatBuffer(size_t aFloats)
    : mFloats(aFloats)
{
#ifndef SOLOUD_SSE_INTRINSICS
    mBasePtr = std::make_unique<unsigned char[]>(mFloats * sizeof(float));
    mData    = reinterpret_cast<float*>(mBasePtr.get());
#else
    mBasePtr = std::make_unique<unsigned char[]>(aFloats * sizeof(float) + 16);
    mData    = (float*)(((size_t)mBasePtr.get() + 15) & ~15);
#endif
}

void AlignedFloatBuffer::clear()
{
    std::fill_n(mData, mFloats, 0.0f);
}

TinyAlignedFloatBuffer::TinyAlignedFloatBuffer()
{
    unsigned char* basePtr = &mActualData[0];
    mData                  = reinterpret_cast<float*>(size_t(basePtr) + 15 & ~15);
}

Engine::Engine(EngineFlags           flags,
               std::optional<size_t> aSamplerate,
               std::optional<size_t> aBufferSize,
               size_t                aChannels)
    : mFlags(flags)
{
    assert(aChannels != 3 && aChannels != 5 && aChannels != 7);
    assert(aChannels <= MAX_CHANNELS);

    mAudioThreadMutex = Thread::createMutex();

    int samplerate = aSamplerate.value_or(44100);
    int buffersize = aBufferSize.value_or(2048);

#if defined(WITH_SDL2_STATIC)
    {
        if (!aBufferSize.has_value())
            buffersize = 2048;

        sdl2static_init(this, flags, samplerate, buffersize, aChannels);
    }
#endif

#if defined(WITH_XAUDIO2)
    {
        if (!aBufferSize.has_value())
            buffersize = 4096;

        xaudio2_init(this, flags, samplerate, buffersize, aChannels);
    }
#endif

#if defined(WITH_WINMM)
    {
        if (!aBufferSize.has_value())
            buffersize = 4096;

        winmm_init(this, flags, samplerate, buffersize, aChannels);
    }
#endif

#if defined(WITH_WASAPI)
    {
        if (!aBufferSize.has_value())
            buffersize = 4096;

        if (!aSamplerate.has_value())
            samplerate = 48000;

        wasapi_init(this, flags, samplerate, buffersize, aChannels);
    }
#endif

#if defined(WITH_ALSA)
    {
        if (!aBufferSize.has_value())
            buffersize = 2048;

        alsa_init(this, flags, samplerate, buffersize, aChannels);
    }
#endif

#if defined(WITH_COREAUDIO)
    {
        if (!aBufferSize.has_value())
            buffersize = 2048;

        coreaudio_init(this, flags, samplerate, buffersize, aChannels);
    }
#endif

#if defined(WITH_OPENSLES)
    {
        if (!aBufferSize.has_value())
            buffersize = 4096;

        opensles_init(this, flags, samplerate, buffersize, aChannels);
    }
#endif
}

Engine::~Engine() noexcept
{
    // let's stop all sounds before deinit, so we don't mess up our mutexes
    stopAll();

    // Make sure no audio operation is currently pending
    lockAudioMutex_internal();
    unlockAudioMutex_internal();
    assert(!mInsideAudioThreadMutex);
    stopAll();
    if (mBackendCleanupFunc)
        mBackendCleanupFunc(this);
    mBackendCleanupFunc = 0;
    if (mAudioThreadMutex)
        Thread::destroyMutex(mAudioThreadMutex);
    mAudioThreadMutex = nullptr;

    for (size_t i = 0; i < FILTERS_PER_STREAM; ++i)
    {
        mFilterInstance[i].reset();
    }
}

void Engine::pause()
{
    if (mBackendPauseFunc)
        mBackendPauseFunc(this);
}

void Engine::resume()
{
    if (mBackendResumeFunc)
        mBackendResumeFunc(this);
}


void Engine::postinit_internal(size_t      aSamplerate,
                               size_t      aBufferSize,
                               EngineFlags flags,
                               size_t      aChannels)
{
    mGlobalVolume = 1;
    mChannels     = aChannels;
    mSamplerate   = aSamplerate;
    mBufferSize   = aBufferSize;
    mScratchSize  = (aBufferSize + 15) & (~0xf); // round to the next div by 16

    if (mScratchSize < SAMPLE_GRANULARITY * 2)
        mScratchSize = SAMPLE_GRANULARITY * 2;

    if (mScratchSize < 4096)
        mScratchSize = 4096;

    mScratch       = AlignedFloatBuffer{mScratchSize * MAX_CHANNELS};
    mOutputScratch = AlignedFloatBuffer{mScratchSize * MAX_CHANNELS};

    mResampleData.resize(mMaxActiveVoices * 2);
    mResampleDataOwner.resize(mMaxActiveVoices);

    mResampleDataBuffer =
        AlignedFloatBuffer{mMaxActiveVoices * 2 * SAMPLE_GRANULARITY * MAX_CHANNELS};

    for (size_t i        = 0; i < mMaxActiveVoices * 2; ++i)
        mResampleData[i] = mResampleDataBuffer.mData + (SAMPLE_GRANULARITY * MAX_CHANNELS * i);

    mFlags          = flags;
    mPostClipScaler = 0.95f;

    switch (mChannels)
    {
        case 1: {
            m3dSpeakerPosition[0] = {0, 0, 1};
            break;
        }
        case 2: {
            m3dSpeakerPosition[0] = {2, 0, 1};
            m3dSpeakerPosition[1] = {-2, 0, 1};
            break;
        }
        case 4: {
            m3dSpeakerPosition[0] = {2, 0, 1};
            m3dSpeakerPosition[1] = {-2, 0, 1};

            // I suppose technically the second pair should be straight left & right,
            // but I prefer moving them a bit back to mirror the front speakers.
            m3dSpeakerPosition[2] = {2, 0, -1};
            m3dSpeakerPosition[3] = {-2, 0, -1};

            break;
        }
        case 6: {
            m3dSpeakerPosition[0] = {2, 0, 1};
            m3dSpeakerPosition[1] = {-2, 0, 1};

            // center and subwoofer.
            m3dSpeakerPosition[2] = {0, 0, 1};

            // Sub should be "mix of everything". We'll handle it as a special case and make it a
            // null vector.
            m3dSpeakerPosition[3] = {0, 0, 0};

            // I suppose technically the second pair should be straight left & right,
            // but I prefer moving them a bit back to mirror the front speakers.
            m3dSpeakerPosition[4] = {2, 0, -1};
            m3dSpeakerPosition[5] = {-2, 0, -1};

            break;
        }
        case 8: {
            m3dSpeakerPosition[0] = {2, 0, 1};
            m3dSpeakerPosition[1] = {-2, 0, 1};

            // center and subwoofer.
            m3dSpeakerPosition[2] = {0, 0, 1};

            // Sub should be "mix of everything". We'll handle it as a special case and make it a
            // null vector.
            m3dSpeakerPosition[3] = {0, 0, 0};

            // side
            m3dSpeakerPosition[4] = {2, 0, 0};
            m3dSpeakerPosition[5] = {-2, 0, 0};

            // back
            m3dSpeakerPosition[6] = {2, 0, -1};
            m3dSpeakerPosition[7] = {-2, 0, -1};

            break;
        }
        default: break;
    }
}

float* Engine::getWave()
{
    lockAudioMutex_internal();

    for (int i = 0; i < 256; ++i)
    {
        mWaveData[i] = mVisualizationWaveData[i];
    }

    unlockAudioMutex_internal();
    return mWaveData.data();
}

float Engine::getApproximateVolume(size_t aChannel)
{
    if (aChannel > mChannels)
    {
        return 0;
    }
    float vol = 0;

    lockAudioMutex_internal();
    vol = mVisualizationChannelVolume[aChannel];
    unlockAudioMutex_internal();

    return vol;
}


float* Engine::calcFFT()
{
    lockAudioMutex_internal();
    float temp[1024];
    for (int i = 0; i < 256; ++i)
    {
        temp[i * 2]     = mVisualizationWaveData[i];
        temp[i * 2 + 1] = 0;
        temp[i + 512]   = 0;
        temp[i + 768]   = 0;
    }
    unlockAudioMutex_internal();

    FFT::fft1024(temp);

    for (int i = 0; i < 256; ++i)
    {
        const float real = temp[i * 2];
        const float imag = temp[i * 2 + 1];

        mFFTData[i] = std::sqrt(real * real + imag * imag);
    }

    return mFFTData.data();
}

#if defined(SOLOUD_SSE_INTRINSICS)
void Engine::clip_internal(const AlignedFloatBuffer& aBuffer,
                           const AlignedFloatBuffer& aDestBuffer,
                           size_t                    aSamples,
                           float                     aVolume0,
                           float                     aVolume1)
{
    float  vd = (aVolume1 - aVolume0) / aSamples;
    float  v  = aVolume0;
    size_t i, j, c, d;
    size_t samplequads = (aSamples + 3) / 4; // rounded up

    // Clip
    if (mFlags.ClipRoundoff)
    {
        float                  nb          = -1.65f;
        __m128                 negbound    = _mm_load_ps1(&nb);
        float                  pb          = 1.65f;
        __m128                 posbound    = _mm_load_ps1(&pb);
        float                  ls          = 0.87f;
        __m128                 linearscale = _mm_load_ps1(&ls);
        float                  cs          = -0.1f;
        __m128                 cubicscale  = _mm_load_ps1(&cs);
        float                  nw          = -0.9862875f;
        __m128                 negwall     = _mm_load_ps1(&nw);
        float                  pw          = 0.9862875f;
        __m128                 poswall     = _mm_load_ps1(&pw);
        __m128                 postscale   = _mm_load_ps1(&mPostClipScaler);
        TinyAlignedFloatBuffer volumes;
        volumes.mData[0] = v;
        volumes.mData[1] = v + vd;
        volumes.mData[2] = v + vd + vd;
        volumes.mData[3] = v + vd + vd + vd;
        vd *= 4;
        __m128 vdelta = _mm_load_ps1(&vd);
        c             = 0;
        d             = 0;
        for (size_t j = 0; j < mChannels; ++j)
        {
            __m128 vol = _mm_load_ps(volumes.mData);

            for (i = 0; i < samplequads; ++i)
            {
                // float f1 = origdata[c] * v;	++c; v += vd;
                __m128 f = _mm_load_ps(&aBuffer.mData[c]);
                c += 4;
                f   = _mm_mul_ps(f, vol);
                vol = _mm_add_ps(vol, vdelta);

                // float u1 = (f1 > -1.65f);
                __m128 u = _mm_cmpgt_ps(f, negbound);

                // float o1 = (f1 < 1.65f);
                __m128 o = _mm_cmplt_ps(f, posbound);

                // f1 = (0.87f * f1 - 0.1f * f1 * f1 * f1) * u1 * o1;
                __m128 lin   = _mm_mul_ps(f, linearscale);
                __m128 cubic = _mm_mul_ps(f, f);
                cubic        = _mm_mul_ps(cubic, f);
                cubic        = _mm_mul_ps(cubic, cubicscale);
                f            = _mm_add_ps(cubic, lin);

                // f1 = f1 * u1 + !u1 * -0.9862875f;
                __m128 lowmask  = _mm_andnot_ps(u, negwall);
                __m128 ilowmask = _mm_and_ps(u, f);
                f               = _mm_add_ps(lowmask, ilowmask);

                // f1 = f1 * o1 + !o1 * 0.9862875f;
                __m128 himask  = _mm_andnot_ps(o, poswall);
                __m128 ihimask = _mm_and_ps(o, f);
                f              = _mm_add_ps(himask, ihimask);

                // outdata[d] = f1 * postclip; d++;
                f = _mm_mul_ps(f, postscale);
                _mm_store_ps(&aDestBuffer.mData[d], f);
                d += 4;
            }
        }
    }
    else
    {
        float                  nb        = -1.0f;
        __m128                 negbound  = _mm_load_ps1(&nb);
        float                  pb        = 1.0f;
        __m128                 posbound  = _mm_load_ps1(&pb);
        __m128                 postscale = _mm_load_ps1(&mPostClipScaler);
        TinyAlignedFloatBuffer volumes;
        volumes.mData[0] = v;
        volumes.mData[1] = v + vd;
        volumes.mData[2] = v + vd + vd;
        volumes.mData[3] = v + vd + vd + vd;
        vd *= 4;
        __m128 vdelta = _mm_load_ps1(&vd);
        c             = 0;
        d             = 0;
        for (size_t j = 0; j < mChannels; ++j)
        {
            __m128 vol = _mm_load_ps(volumes.mData);
            for (i = 0; i < samplequads; ++i)
            {
                // float f1 = aBuffer.mData[c] * v; ++c; v += vd;
                __m128 f = _mm_load_ps(&aBuffer.mData[c]);
                c += 4;
                f   = _mm_mul_ps(f, vol);
                vol = _mm_add_ps(vol, vdelta);

                // f1 = (f1 <= -1) ? -1 : (f1 >= 1) ? 1 : f1;
                f = _mm_max_ps(f, negbound);
                f = _mm_min_ps(f, posbound);

                // aDestBuffer.mData[d] = f1 * mPostClipScaler; d++;
                f = _mm_mul_ps(f, postscale);
                _mm_store_ps(&aDestBuffer.mData[d], f);
                d += 4;
            }
        }
    }
}
#else // fallback code
void Engine::clip_internal(const AlignedFloatBuffer& aBuffer,
                           const AlignedFloatBuffer& aDestBuffer,
                           size_t                    aSamples,
                           float                     aVolume0,
                           float                     aVolume1)
{
    const float vd          = (aVolume1 - aVolume0) / aSamples;
    const auto  samplequads = (aSamples + 3) / 4; // rounded up

    // Clip
    if (mFlags.ClipRoundoff)
    {
        auto c = size_t(0);
        auto d = size_t(0);

        for (size_t j = 0; j < mChannels; ++j)
        {
            auto v = aVolume0;
            for (size_t i = 0; i < samplequads; ++i)
            {
                float f1 = aBuffer.mData[c] * v;
                ++c;
                v += vd;

                float f2 = aBuffer.mData[c] * v;
                ++c;
                v += vd;

                float f3 = aBuffer.mData[c] * v;
                ++c;
                v += vd;

                float f4 = aBuffer.mData[c] * v;
                ++c;
                v += vd;

                f1 = f1 <= -1.65f
                         ? -0.9862875f
                         : f1 >= 1.65f
                         ? 0.9862875f
                         : 0.87f * f1 - 0.1f * f1 * f1 * f1;
                f2 = f2 <= -1.65f
                         ? -0.9862875f
                         : f2 >= 1.65f
                         ? 0.9862875f
                         : 0.87f * f2 - 0.1f * f2 * f2 * f2;
                f3 = f3 <= -1.65f
                         ? -0.9862875f
                         : f3 >= 1.65f
                         ? 0.9862875f
                         : 0.87f * f3 - 0.1f * f3 * f3 * f3;
                f4 = f4 <= -1.65f
                         ? -0.9862875f
                         : f4 >= 1.65f
                         ? 0.9862875f
                         : 0.87f * f4 - 0.1f * f4 * f4 * f4;

                aDestBuffer.mData[d] = f1 * mPostClipScaler;
                d++;

                aDestBuffer.mData[d] = f2 * mPostClipScaler;
                d++;

                aDestBuffer.mData[d] = f3 * mPostClipScaler;
                d++;

                aDestBuffer.mData[d] = f4 * mPostClipScaler;
                d++;
            }
        }
    }
    else
    {
        auto c = size_t(0);
        auto d = size_t(0);

        for (size_t j = 0; j < mChannels; ++j)
        {
            auto v = aVolume0;

            for (size_t i = 0; i < samplequads; ++i)
            {
                float f1 = aBuffer.mData[c] * v;
                ++c;
                v += vd;

                float f2 = aBuffer.mData[c] * v;
                ++c;
                v += vd;

                float f3 = aBuffer.mData[c] * v;
                ++c;
                v += vd;

                float f4 = aBuffer.mData[c] * v;
                ++c;
                v += vd;

                f1 = f1 <= -1 ? -1 : f1 >= 1 ? 1 : f1;
                f2 = f2 <= -1 ? -1 : f2 >= 1 ? 1 : f2;
                f3 = f3 <= -1 ? -1 : f3 >= 1 ? 1 : f3;
                f4 = f4 <= -1 ? -1 : f4 >= 1 ? 1 : f4;

                aDestBuffer.mData[d] = f1 * mPostClipScaler;
                d++;

                aDestBuffer.mData[d] = f2 * mPostClipScaler;
                d++;

                aDestBuffer.mData[d] = f3 * mPostClipScaler;
                d++;

                aDestBuffer.mData[d] = f4 * mPostClipScaler;
                d++;
            }
        }
    }
}
#endif

static constexpr auto FIXPOINT_FRAC_BITS = 20;
static constexpr auto FIXPOINT_FRAC_MUL  = 1 << FIXPOINT_FRAC_BITS;
static constexpr auto FIXPOINT_FRAC_MASK = (1 << FIXPOINT_FRAC_BITS) - 1;

static float catmullrom(float t, float p0, float p1, float p2, float p3)
{
    return 0.5f * (2 * p1 + (-p0 + p2) * t + (2 * p0 - 5 * p1 + 4 * p2 - p3) * t * t +
                   (-p0 + 3 * p1 - 3 * p2 + p3) * t * t * t);
}

static void resample_catmullrom(const float* aSrc,
                                const float* aSrc1,
                                float*       aDst,
                                int          aSrcOffset,
                                int          aDstSampleCount,
                                int          aStepFixed)
{
    int pos = aSrcOffset;

    for (int i = 0; i < aDstSampleCount; ++i, pos += aStepFixed)
    {
        const int p = pos >> FIXPOINT_FRAC_BITS;
        const int f = pos & FIXPOINT_FRAC_MASK;

        auto s3 = 0.0f;

        if (p < 3)
        {
            s3 = aSrc1[512 + p - 3];
        }
        else
        {
            s3 = aSrc[p - 3];
        }

        auto s2 = 0.0f;

        if (p < 2)
        {
            s2 = aSrc1[512 + p - 2];
        }
        else
        {
            s2 = aSrc[p - 2];
        }

        auto s1 = 0.0f;

        if (p < 1)
        {
            s1 = aSrc1[512 + p - 1];
        }
        else
        {
            s1 = aSrc[p - 1];
        }

        const auto s0 = aSrc[p];

        aDst[i] = catmullrom(f / float(FIXPOINT_FRAC_MUL), s3, s2, s1, s0);
    }
}

static void resample_linear(const float* aSrc,
                            const float* aSrc1,
                            float*       aDst,
                            int          aSrcOffset,
                            int          aDstSampleCount,
                            int          aStepFixed)
{
    int pos = aSrcOffset;

    for (int i = 0; i < aDstSampleCount; ++i, pos += aStepFixed)
    {
        const int   p  = pos >> FIXPOINT_FRAC_BITS;
        const int   f  = pos & FIXPOINT_FRAC_MASK;
        float       s1 = aSrc1[SAMPLE_GRANULARITY - 1];
        const float s2 = aSrc[p];
        if (p != 0)
        {
            s1 = aSrc[p - 1];
        }
        aDst[i] = s1 + (s2 - s1) * f * (1 / (float)FIXPOINT_FRAC_MUL);
    }
}

static void resample_point(const float* aSrc,
                           const float* aSrc1,
                           float*       aDst,
                           int          aSrcOffset,
                           int          aDstSampleCount,
                           int          aStepFixed)
{
    int pos = aSrcOffset;

    for (int i = 0; i < aDstSampleCount; ++i, pos += aStepFixed)
    {
        const int p = pos >> FIXPOINT_FRAC_BITS;

        aDst[i] = aSrc[p];
    }
}


void panAndExpand(std::shared_ptr<AudioSourceInstance>& aVoice,
                  float*                                aBuffer,
                  size_t                                aSamplesToRead,
                  size_t                                aBufferSize,
                  float*                                aScratch,
                  size_t                                aChannels)
{
#ifdef SOLOUD_SSE_INTRINSICS
    assert(((size_t)aBuffer & 0xf) == 0);
    assert(((size_t)aScratch & 0xf) == 0);
    assert(((size_t)aBufferSize & 0xf) == 0);
#endif

    float                           pan[MAX_CHANNELS]; // current speaker volume
    std::array<float, MAX_CHANNELS> pand{}; // destination speaker volume
    std::array<float, MAX_CHANNELS> pani{}; // speaker volume increment per sample

    for (size_t k = 0; k < aChannels; k++)
    {
        pan[k]  = aVoice->mCurrentChannelVolume[k];
        pand[k] = aVoice->mChannelVolume[k] * aVoice->mOverallVolume;
        pani[k] =
            (pand[k] - pan[k]) /
            aSamplesToRead; // TODO: this is a bit inconsistent.. but it's a hack to begin with
    }

    switch (aChannels)
    {
        case 1: // Target is mono. Sum everything. (1->1, 2->1, 4->1, 6->1, 8->1)
            for (size_t j = 0, ofs = 0; j < aVoice->mChannels; ++j, ofs += aBufferSize)
            {
                pan[0] = aVoice->mCurrentChannelVolume[0];
                for (size_t k = 0; k < aSamplesToRead; k++)
                {
                    pan[0] += pani[0];
                    aBuffer[k] += aScratch[ofs + k] * pan[0];
                }
            }
            break;
        case 2: switch (aVoice->mChannels)
            {
                case 8: // 8->2, just sum lefties and righties, add a bit of center and sub?
                    for (size_t j = 0; j < aSamplesToRead; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        float s1 = aScratch[j];
                        float s2 = aScratch[aBufferSize + j];
                        float s3 = aScratch[aBufferSize * 2 + j];
                        float s4 = aScratch[aBufferSize * 3 + j];
                        float s5 = aScratch[aBufferSize * 4 + j];
                        float s6 = aScratch[aBufferSize * 5 + j];
                        float s7 = aScratch[aBufferSize * 6 + j];
                        float s8 = aScratch[aBufferSize * 7 + j];
                        aBuffer[j + 0] += 0.2f * (s1 + s3 + s4 + s5 + s7) * pan[0];
                        aBuffer[j + aBufferSize] += 0.2f * (s2 + s3 + s4 + s6 + s8) * pan[1];
                    }
                    break;
                case 6: // 6->2, just sum lefties and righties, add a bit of center and sub?
                    for (size_t j = 0; j < aSamplesToRead; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        float s1 = aScratch[j];
                        float s2 = aScratch[aBufferSize + j];
                        float s3 = aScratch[aBufferSize * 2 + j];
                        float s4 = aScratch[aBufferSize * 3 + j];
                        float s5 = aScratch[aBufferSize * 4 + j];
                        float s6 = aScratch[aBufferSize * 5 + j];
                        aBuffer[j + 0] += 0.3f * (s1 + s3 + s4 + s5) * pan[0];
                        aBuffer[j + aBufferSize] += 0.3f * (s2 + s3 + s4 + s6) * pan[1];
                    }
                    break;
                case 4: // 4->2, just sum lefties and righties
                    for (size_t j = 0; j < aSamplesToRead; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        float s1 = aScratch[j];
                        float s2 = aScratch[aBufferSize + j];
                        float s3 = aScratch[aBufferSize * 2 + j];
                        float s4 = aScratch[aBufferSize * 3 + j];
                        aBuffer[j + 0] += 0.5f * (s1 + s3) * pan[0];
                        aBuffer[j + aBufferSize] += 0.5f * (s2 + s4) * pan[1];
                    }
                    break;
                case 2: // 2->2
#if defined(SOLOUD_SSE_INTRINSICS)
                {
                    int c = 0;
                    // if ((aBufferSize & 3) == 0)
                    {
                        size_t                 samplequads = aSamplesToRead / 4; // rounded down
                        TinyAlignedFloatBuffer pan0;
                        pan0.mData[0] = pan[0] + pani[0];
                        pan0.mData[1] = pan[0] + pani[0] * 2;
                        pan0.mData[2] = pan[0] + pani[0] * 3;
                        pan0.mData[3] = pan[0] + pani[0] * 4;
                        TinyAlignedFloatBuffer pan1;
                        pan1.mData[0] = pan[1] + pani[1];
                        pan1.mData[1] = pan[1] + pani[1] * 2;
                        pan1.mData[2] = pan[1] + pani[1] * 3;
                        pan1.mData[3] = pan[1] + pani[1] * 4;
                        pani[0] *= 4;
                        pani[1] *= 4;
                        __m128 pan0delta = _mm_load_ps1(&pani[0]);
                        __m128 pan1delta = _mm_load_ps1(&pani[1]);
                        __m128 p0        = _mm_load_ps(pan0.mData);
                        __m128 p1        = _mm_load_ps(pan1.mData);

                        for (size_t j = 0; j < samplequads; ++j)
                        {
                            __m128 f0 = _mm_load_ps(aScratch + c);
                            __m128 c0 = _mm_mul_ps(f0, p0);
                            __m128 f1 = _mm_load_ps(aScratch + c + aBufferSize);
                            __m128 c1 = _mm_mul_ps(f1, p1);
                            __m128 o0 = _mm_load_ps(aBuffer + c);
                            __m128 o1 = _mm_load_ps(aBuffer + c + aBufferSize);
                            c0        = _mm_add_ps(c0, o0);
                            c1        = _mm_add_ps(c1, o1);
                            _mm_store_ps(aBuffer + c, c0);
                            _mm_store_ps(aBuffer + c + aBufferSize, c1);
                            p0 = _mm_add_ps(p0, pan0delta);
                            p1 = _mm_add_ps(p1, pan1delta);
                            c += 4;
                        }
                    }

                    // If buffer size or samples to read are not divisible by 4, handle leftovers
                    for (size_t j = c; j < aSamplesToRead; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        float s1 = aScratch[j];
                        float s2 = aScratch[aBufferSize + j];
                        aBuffer[j + 0] += s1 * pan[0];
                        aBuffer[j + aBufferSize] += s2 * pan[1];
                    }
                }
#else // fallback
                    for (size_t j = 0; j < aSamplesToRead; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        float s1 = aScratch[j];
                        float s2 = aScratch[aBufferSize + j];
                        aBuffer[j + 0] += s1 * pan[0];
                        aBuffer[j + aBufferSize] += s2 * pan[1];
                    }
#endif
                    break;
                case 1: // 1->2
#if defined(SOLOUD_SSE_INTRINSICS)
                {
                    int c = 0;
                    // if ((aBufferSize & 3) == 0)
                    {
                        size_t                 samplequads = aSamplesToRead / 4; // rounded down
                        TinyAlignedFloatBuffer pan0;
                        pan0.mData[0] = pan[0] + pani[0];
                        pan0.mData[1] = pan[0] + pani[0] * 2;
                        pan0.mData[2] = pan[0] + pani[0] * 3;
                        pan0.mData[3] = pan[0] + pani[0] * 4;
                        TinyAlignedFloatBuffer pan1;
                        pan1.mData[0] = pan[1] + pani[1];
                        pan1.mData[1] = pan[1] + pani[1] * 2;
                        pan1.mData[2] = pan[1] + pani[1] * 3;
                        pan1.mData[3] = pan[1] + pani[1] * 4;
                        pani[0] *= 4;
                        pani[1] *= 4;
                        __m128 pan0delta = _mm_load_ps1(&pani[0]);
                        __m128 pan1delta = _mm_load_ps1(&pani[1]);
                        __m128 p0        = _mm_load_ps(pan0.mData);
                        __m128 p1        = _mm_load_ps(pan1.mData);

                        for (size_t j = 0; j < samplequads; ++j)
                        {
                            __m128 f  = _mm_load_ps(aScratch + c);
                            __m128 c0 = _mm_mul_ps(f, p0);
                            __m128 c1 = _mm_mul_ps(f, p1);
                            __m128 o0 = _mm_load_ps(aBuffer + c);
                            __m128 o1 = _mm_load_ps(aBuffer + c + aBufferSize);
                            c0        = _mm_add_ps(c0, o0);
                            c1        = _mm_add_ps(c1, o1);
                            _mm_store_ps(aBuffer + c, c0);
                            _mm_store_ps(aBuffer + c + aBufferSize, c1);
                            p0 = _mm_add_ps(p0, pan0delta);
                            p1 = _mm_add_ps(p1, pan1delta);
                            c += 4;
                        }
                    }
                    // If buffer size or samples to read are not divisible by 4, handle leftovers
                    for (size_t j = c; j < aSamplesToRead; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        float s = aScratch[j];
                        aBuffer[j + 0] += s * pan[0];
                        aBuffer[j + aBufferSize] += s * pan[1];
                    }
                }
#else // fallback
                    for (size_t j = 0; j < aSamplesToRead; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        float s = aScratch[j];
                        aBuffer[j + 0] += s * pan[0];
                        aBuffer[j + aBufferSize] += s * pan[1];
                    }
#endif
                    break;
            }
            break;
        case 4: switch (aVoice->mChannels)
            {
                case 8: // 8->4, add a bit of center, sub?
                    for (size_t j = 0; j < aSamplesToRead; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        pan[2] += pani[2];
                        pan[3] += pani[3];
                        float s1 = aScratch[j];
                        float s2 = aScratch[aBufferSize + j];
                        float s3 = aScratch[aBufferSize * 2 + j];
                        float s4 = aScratch[aBufferSize * 3 + j];
                        float s5 = aScratch[aBufferSize * 4 + j];
                        float s6 = aScratch[aBufferSize * 5 + j];
                        float s7 = aScratch[aBufferSize * 6 + j];
                        float s8 = aScratch[aBufferSize * 7 + j];
                        float c  = (s3 + s4) * 0.7f;
                        aBuffer[j + 0] += s1 * pan[0] + c;
                        aBuffer[j + aBufferSize] += s2 * pan[1] + c;
                        aBuffer[j + aBufferSize * 2] += 0.5f * (s5 + s7) * pan[2];
                        aBuffer[j + aBufferSize * 3] += 0.5f * (s6 + s8) * pan[3];
                    }
                    break;
                case 6: // 6->4, add a bit of center, sub?
                    for (size_t j = 0; j < aSamplesToRead; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        pan[2] += pani[2];
                        pan[3] += pani[3];
                        float s1 = aScratch[j];
                        float s2 = aScratch[aBufferSize + j];
                        float s3 = aScratch[aBufferSize * 2 + j];
                        float s4 = aScratch[aBufferSize * 3 + j];
                        float s5 = aScratch[aBufferSize * 4 + j];
                        float s6 = aScratch[aBufferSize * 5 + j];
                        float c  = (s3 + s4) * 0.7f;
                        aBuffer[j + 0] += s1 * pan[0] + c;
                        aBuffer[j + aBufferSize] += s2 * pan[1] + c;
                        aBuffer[j + aBufferSize * 2] += s5 * pan[2];
                        aBuffer[j + aBufferSize * 3] += s6 * pan[3];
                    }
                    break;
                case 4: // 4->4
                    for (size_t j = 0; j < aSamplesToRead; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        pan[2] += pani[2];
                        pan[3] += pani[3];
                        float s1 = aScratch[j];
                        float s2 = aScratch[aBufferSize + j];
                        float s3 = aScratch[aBufferSize * 2 + j];
                        float s4 = aScratch[aBufferSize * 3 + j];
                        aBuffer[j + 0] += s1 * pan[0];
                        aBuffer[j + aBufferSize] += s2 * pan[1];
                        aBuffer[j + aBufferSize * 2] += s3 * pan[2];
                        aBuffer[j + aBufferSize * 3] += s4 * pan[3];
                    }
                    break;
                case 2: // 2->4
                    for (size_t j = 0; j < aSamplesToRead; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        pan[2] += pani[2];
                        pan[3] += pani[3];
                        float s1 = aScratch[j];
                        float s2 = aScratch[aBufferSize + j];
                        aBuffer[j + 0] += s1 * pan[0];
                        aBuffer[j + aBufferSize] += s2 * pan[1];
                        aBuffer[j + aBufferSize * 2] += s1 * pan[2];
                        aBuffer[j + aBufferSize * 3] += s2 * pan[3];
                    }
                    break;
                case 1: // 1->4
                    for (size_t j = 0; j < aSamplesToRead; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        pan[2] += pani[2];
                        pan[3] += pani[3];
                        float s = aScratch[j];
                        aBuffer[j + 0] += s * pan[0];
                        aBuffer[j + aBufferSize] += s * pan[1];
                        aBuffer[j + aBufferSize * 2] += s * pan[2];
                        aBuffer[j + aBufferSize * 3] += s * pan[3];
                    }
                    break;
            }
            break;
        case 6: switch (aVoice->mChannels)
            {
                case 8: // 8->6
                    for (size_t j = 0; j < aSamplesToRead; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        pan[2] += pani[2];
                        pan[3] += pani[3];
                        pan[4] += pani[4];
                        pan[5] += pani[5];
                        float s1 = aScratch[j];
                        float s2 = aScratch[aBufferSize + j];
                        float s3 = aScratch[aBufferSize * 2 + j];
                        float s4 = aScratch[aBufferSize * 3 + j];
                        float s5 = aScratch[aBufferSize * 4 + j];
                        float s6 = aScratch[aBufferSize * 5 + j];
                        float s7 = aScratch[aBufferSize * 6 + j];
                        float s8 = aScratch[aBufferSize * 7 + j];
                        aBuffer[j + 0] += s1 * pan[0];
                        aBuffer[j + aBufferSize] += s2 * pan[1];
                        aBuffer[j + aBufferSize * 2] += s3 * pan[2];
                        aBuffer[j + aBufferSize * 3] += s4 * pan[3];
                        aBuffer[j + aBufferSize * 4] += 0.5f * (s5 + s7) * pan[4];
                        aBuffer[j + aBufferSize * 5] += 0.5f * (s6 + s8) * pan[5];
                    }
                    break;
                case 6: // 6->6
                    for (size_t j = 0; j < aSamplesToRead; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        pan[2] += pani[2];
                        pan[3] += pani[3];
                        pan[4] += pani[4];
                        pan[5] += pani[5];
                        float s1 = aScratch[j];
                        float s2 = aScratch[aBufferSize + j];
                        float s3 = aScratch[aBufferSize * 2 + j];
                        float s4 = aScratch[aBufferSize * 3 + j];
                        float s5 = aScratch[aBufferSize * 4 + j];
                        float s6 = aScratch[aBufferSize * 5 + j];
                        aBuffer[j + 0] += s1 * pan[0];
                        aBuffer[j + aBufferSize] += s2 * pan[1];
                        aBuffer[j + aBufferSize * 2] += s3 * pan[2];
                        aBuffer[j + aBufferSize * 3] += s4 * pan[3];
                        aBuffer[j + aBufferSize * 4] += s5 * pan[4];
                        aBuffer[j + aBufferSize * 5] += s6 * pan[5];
                    }
                    break;
                case 4: // 4->6
                    for (size_t j = 0; j < aSamplesToRead; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        pan[2] += pani[2];
                        pan[3] += pani[3];
                        pan[4] += pani[4];
                        pan[5] += pani[5];
                        float s1 = aScratch[j];
                        float s2 = aScratch[aBufferSize + j];
                        float s3 = aScratch[aBufferSize * 2 + j];
                        float s4 = aScratch[aBufferSize * 3 + j];
                        aBuffer[j + 0] += s1 * pan[0];
                        aBuffer[j + aBufferSize] += s2 * pan[1];
                        aBuffer[j + aBufferSize * 2] += 0.5f * (s1 + s2) * pan[2];
                        aBuffer[j + aBufferSize * 3] += 0.25f * (s1 + s2 + s3 + s4) * pan[3];
                        aBuffer[j + aBufferSize * 4] += s3 * pan[4];
                        aBuffer[j + aBufferSize * 5] += s4 * pan[5];
                    }
                    break;
                case 2: // 2->6
                    for (size_t j = 0; j < aSamplesToRead; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        pan[2] += pani[2];
                        pan[3] += pani[3];
                        pan[4] += pani[4];
                        pan[5] += pani[5];
                        float s1 = aScratch[j];
                        float s2 = aScratch[aBufferSize + j];
                        aBuffer[j + 0] += s1 * pan[0];
                        aBuffer[j + aBufferSize] += s2 * pan[1];
                        aBuffer[j + aBufferSize * 2] += 0.5f * (s1 + s2) * pan[2];
                        aBuffer[j + aBufferSize * 3] += 0.5f * (s1 + s2) * pan[3];
                        aBuffer[j + aBufferSize * 4] += s1 * pan[4];
                        aBuffer[j + aBufferSize * 5] += s2 * pan[5];
                    }
                    break;
                case 1: // 1->6
                    for (size_t j = 0; j < aSamplesToRead; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        pan[2] += pani[2];
                        pan[3] += pani[3];
                        pan[4] += pani[4];
                        pan[5] += pani[5];
                        float s = aScratch[j];
                        aBuffer[j + 0] += s * pan[0];
                        aBuffer[j + aBufferSize] += s * pan[1];
                        aBuffer[j + aBufferSize * 2] += s * pan[2];
                        aBuffer[j + aBufferSize * 3] += s * pan[3];
                        aBuffer[j + aBufferSize * 4] += s * pan[4];
                        aBuffer[j + aBufferSize * 5] += s * pan[5];
                    }
                    break;
            }
            break;
        case 8: switch (aVoice->mChannels)
            {
                case 8: // 8->8
                    for (size_t j = 0; j < aSamplesToRead; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        pan[2] += pani[2];
                        pan[3] += pani[3];
                        pan[4] += pani[4];
                        pan[5] += pani[5];
                        pan[6] += pani[6];
                        pan[7] += pani[7];
                        float s1 = aScratch[j];
                        float s2 = aScratch[aBufferSize + j];
                        float s3 = aScratch[aBufferSize * 2 + j];
                        float s4 = aScratch[aBufferSize * 3 + j];
                        float s5 = aScratch[aBufferSize * 4 + j];
                        float s6 = aScratch[aBufferSize * 5 + j];
                        float s7 = aScratch[aBufferSize * 6 + j];
                        float s8 = aScratch[aBufferSize * 7 + j];
                        aBuffer[j + 0] += s1 * pan[0];
                        aBuffer[j + aBufferSize] += s2 * pan[1];
                        aBuffer[j + aBufferSize * 2] += s3 * pan[2];
                        aBuffer[j + aBufferSize * 3] += s4 * pan[3];
                        aBuffer[j + aBufferSize * 4] += s5 * pan[4];
                        aBuffer[j + aBufferSize * 5] += s6 * pan[5];
                        aBuffer[j + aBufferSize * 6] += s7 * pan[6];
                        aBuffer[j + aBufferSize * 7] += s8 * pan[7];
                    }
                    break;
                case 6: // 6->8
                    for (size_t j = 0; j < aSamplesToRead; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        pan[2] += pani[2];
                        pan[3] += pani[3];
                        pan[4] += pani[4];
                        pan[5] += pani[5];
                        pan[6] += pani[6];
                        pan[7] += pani[7];
                        float s1 = aScratch[j];
                        float s2 = aScratch[aBufferSize + j];
                        float s3 = aScratch[aBufferSize * 2 + j];
                        float s4 = aScratch[aBufferSize * 3 + j];
                        float s5 = aScratch[aBufferSize * 4 + j];
                        float s6 = aScratch[aBufferSize * 5 + j];
                        aBuffer[j + 0] += s1 * pan[0];
                        aBuffer[j + aBufferSize] += s2 * pan[1];
                        aBuffer[j + aBufferSize * 2] += s3 * pan[2];
                        aBuffer[j + aBufferSize * 3] += s4 * pan[3];
                        aBuffer[j + aBufferSize * 4] += 0.5f * (s5 + s1) * pan[4];
                        aBuffer[j + aBufferSize * 5] += 0.5f * (s6 + s2) * pan[5];
                        aBuffer[j + aBufferSize * 6] += s5 * pan[6];
                        aBuffer[j + aBufferSize * 7] += s6 * pan[7];
                    }
                    break;
                case 4: // 4->8
                    for (size_t j = 0; j < aSamplesToRead; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        pan[2] += pani[2];
                        pan[3] += pani[3];
                        pan[4] += pani[4];
                        pan[5] += pani[5];
                        pan[6] += pani[6];
                        pan[7] += pani[7];
                        float s1 = aScratch[j];
                        float s2 = aScratch[aBufferSize + j];
                        float s3 = aScratch[aBufferSize * 2 + j];
                        float s4 = aScratch[aBufferSize * 3 + j];
                        aBuffer[j + 0] += s1 * pan[0];
                        aBuffer[j + aBufferSize] += s2 * pan[1];
                        aBuffer[j + aBufferSize * 2] += 0.5f * (s1 + s2) * pan[2];
                        aBuffer[j + aBufferSize * 3] += 0.25f * (s1 + s2 + s3 + s4) * pan[3];
                        aBuffer[j + aBufferSize * 4] += 0.5f * (s1 + s3) * pan[4];
                        aBuffer[j + aBufferSize * 5] += 0.5f * (s2 + s4) * pan[5];
                        aBuffer[j + aBufferSize * 6] += s3 * pan[4];
                        aBuffer[j + aBufferSize * 7] += s4 * pan[5];
                    }
                    break;
                case 2: // 2->8
                    for (size_t j = 0; j < aSamplesToRead; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        pan[2] += pani[2];
                        pan[3] += pani[3];
                        pan[4] += pani[4];
                        pan[5] += pani[5];
                        pan[6] += pani[6];
                        pan[7] += pani[7];
                        float s1 = aScratch[j];
                        float s2 = aScratch[aBufferSize + j];
                        aBuffer[j + 0] += s1 * pan[0];
                        aBuffer[j + aBufferSize] += s2 * pan[1];
                        aBuffer[j + aBufferSize * 2] += 0.5f * (s1 + s2) * pan[2];
                        aBuffer[j + aBufferSize * 3] += 0.5f * (s1 + s2) * pan[3];
                        aBuffer[j + aBufferSize * 4] += s1 * pan[4];
                        aBuffer[j + aBufferSize * 5] += s2 * pan[5];
                        aBuffer[j + aBufferSize * 6] += s1 * pan[6];
                        aBuffer[j + aBufferSize * 7] += s2 * pan[7];
                    }
                    break;
                case 1: // 1->8
                    for (size_t j = 0; j < aSamplesToRead; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        pan[2] += pani[2];
                        pan[3] += pani[3];
                        pan[4] += pani[4];
                        pan[5] += pani[5];
                        pan[6] += pani[6];
                        pan[7] += pani[7];
                        float s = aScratch[j];
                        aBuffer[j + 0] += s * pan[0];
                        aBuffer[j + aBufferSize] += s * pan[1];
                        aBuffer[j + aBufferSize * 2] += s * pan[2];
                        aBuffer[j + aBufferSize * 3] += s * pan[3];
                        aBuffer[j + aBufferSize * 4] += s * pan[4];
                        aBuffer[j + aBufferSize * 5] += s * pan[5];
                        aBuffer[j + aBufferSize * 6] += s * pan[6];
                        aBuffer[j + aBufferSize * 7] += s * pan[7];
                    }
                    break;
            }
            break;
    }

    for (size_t k = 0; k < aChannels; k++)
    {
        aVoice->mCurrentChannelVolume[k] = pand[k];
    }
}

void Engine::mixBus_internal(float*    aBuffer,
                             size_t    aSamplesToRead,
                             size_t    aBufferSize,
                             float*    aScratch,
                             size_t    aBus,
                             float     aSamplerate,
                             size_t    aChannels,
                             Resampler aResampler)
{
    // Clear accumulation buffer
    for (size_t i = 0; i < aSamplesToRead; ++i)
    {
        for (size_t j = 0; j < aChannels; ++j)
        {
            aBuffer[i + j * aBufferSize] = 0;
        }
    }

    // Accumulate sound sources
    for (size_t i = 0; i < mActiveVoiceCount; ++i)
    {
        auto& voice = mVoice[mActiveVoice[i]];

        if (voice != nullptr && voice->mBusHandle == aBus && !voice->mFlags.Paused &&
            !voice->mFlags.Inaudible)
        {
            float step = voice->mSamplerate / aSamplerate;

            // avoid step overflow
            if (step > (1 << (32 - FIXPOINT_FRAC_BITS)))
            {
                step = 0;
            }

            size_t step_fixed = (int)floor(step * FIXPOINT_FRAC_MUL);
            size_t outofs     = 0;

            if (voice->mDelaySamples)
            {
                if (voice->mDelaySamples > aSamplesToRead)
                {
                    outofs = aSamplesToRead;
                    voice->mDelaySamples -= aSamplesToRead;
                }
                else
                {
                    outofs               = voice->mDelaySamples;
                    voice->mDelaySamples = 0;
                }

                // Clear scratch where we're skipping
                for (size_t k = 0; k < voice->mChannels; k++)
                {
                    memset(aScratch + k * aBufferSize, 0, sizeof(float) * outofs);
                }
            }

            while (step_fixed != 0 && outofs < aSamplesToRead)
            {
                if (voice->mLeftoverSamples == 0)
                {
                    // Swap resample buffers (ping-pong)
                    float* t                = voice->mResampleData[0];
                    voice->mResampleData[0] = voice->mResampleData[1];
                    voice->mResampleData[1] = t;

                    // Get a block of source data

                    size_t readcount = 0;
                    if (!voice->hasEnded() || voice->mFlags.Looping)
                    {
                        readcount = voice->getAudio(voice->mResampleData[0],
                                                    SAMPLE_GRANULARITY,
                                                    SAMPLE_GRANULARITY);
                        if (readcount < SAMPLE_GRANULARITY)
                        {
                            if (voice->mFlags.Looping)
                            {
                                while (readcount < SAMPLE_GRANULARITY &&
                                       voice->seek(voice->mLoopPoint, mScratch.mData, mScratchSize))
                                {
                                    voice->mLoopCount++;

                                    const int inc =
                                        voice->getAudio(voice->mResampleData[0] + readcount,
                                                        SAMPLE_GRANULARITY - readcount,
                                                        SAMPLE_GRANULARITY);

                                    readcount += inc;
                                    if (inc == 0)
                                        break;
                                }
                            }
                        }
                    }

                    // Clear remaining of the resample data if the full scratch wasn't used
                    if (readcount < SAMPLE_GRANULARITY)
                    {
                        for (size_t k = 0; k < voice->mChannels; k++)
                        {
                            memset(voice->mResampleData[0] + readcount + SAMPLE_GRANULARITY * k,
                                   0,
                                   sizeof(float) * (SAMPLE_GRANULARITY - readcount));
                        }
                    }

                    // If we go past zero, crop to zero (a bit of a kludge)
                    if (voice->mSrcOffset < SAMPLE_GRANULARITY * FIXPOINT_FRAC_MUL)
                    {
                        voice->mSrcOffset = 0;
                    }
                    else
                    {
                        // We have new block of data, move pointer backwards
                        voice->mSrcOffset -= SAMPLE_GRANULARITY * FIXPOINT_FRAC_MUL;
                    }


                    // Run the per-stream filters to get our source data

                    for (size_t j = 0; j < FILTERS_PER_STREAM; ++j)
                    {
                        if (voice->mFilter[j])
                        {
                            voice->mFilter[j]->filter(voice->mResampleData[0],
                                                      SAMPLE_GRANULARITY,
                                                      SAMPLE_GRANULARITY,
                                                      voice->mChannels,
                                                      voice->mSamplerate,
                                                      mStreamTime);
                        }
                    }
                }
                else
                {
                    voice->mLeftoverSamples = 0;
                }

                // Figure out how many samples we can generate from this source data.
                // The value may be zero.

                size_t writesamples = 0;

                if (voice->mSrcOffset < SAMPLE_GRANULARITY * FIXPOINT_FRAC_MUL)
                {
                    writesamples = ((SAMPLE_GRANULARITY * FIXPOINT_FRAC_MUL) - voice->mSrcOffset) /
                                   step_fixed +
                                   1;

                    // avoid reading past the current buffer..
                    if (((writesamples * step_fixed + voice->mSrcOffset) >> FIXPOINT_FRAC_BITS) >=
                        SAMPLE_GRANULARITY)
                        writesamples--;
                }


                // If this is too much for our output buffer, don't write that many:
                if (writesamples + outofs > aSamplesToRead)
                {
                    voice->mLeftoverSamples = (writesamples + outofs) - aSamplesToRead;
                    writesamples            = aSamplesToRead - outofs;
                }

                // Call resampler to generate the samples, once per channel
                if (writesamples)
                {
                    for (size_t j = 0; j < voice->mChannels; ++j)
                    {
                        switch (aResampler)
                        {
                            case Resampler::Point: resample_point(
                                    voice->mResampleData[0] + SAMPLE_GRANULARITY * j,
                                    voice->mResampleData[1] + SAMPLE_GRANULARITY * j,
                                    aScratch + aBufferSize * j + outofs,
                                    voice->mSrcOffset,
                                    writesamples,
                                    /*voice->mSamplerate,
                                    aSamplerate,*/
                                    step_fixed);
                                break;
                            case Resampler::CatmullRom: resample_catmullrom(
                                    voice->mResampleData[0] + SAMPLE_GRANULARITY * j,
                                    voice->mResampleData[1] + SAMPLE_GRANULARITY * j,
                                    aScratch + aBufferSize * j + outofs,
                                    voice->mSrcOffset,
                                    writesamples,
                                    /*voice->mSamplerate,
                                    aSamplerate,*/
                                    step_fixed);
                                break;
                            default:
                                // case RESAMPLER_LINEAR:
                                resample_linear(voice->mResampleData[0] + SAMPLE_GRANULARITY * j,
                                                voice->mResampleData[1] + SAMPLE_GRANULARITY * j,
                                                aScratch + aBufferSize * j + outofs,
                                                voice->mSrcOffset,
                                                writesamples,
                                                /*voice->mSamplerate,
                                                aSamplerate,*/
                                                step_fixed);
                                break;
                        }
                    }
                }

                // Keep track of how many samples we've written so far
                outofs += writesamples;

                // Move source pointer onwards (writesamples may be zero)
                voice->mSrcOffset += writesamples * step_fixed;
            }

            // Handle panning and channel expansion (and/or shrinking)
            panAndExpand(voice, aBuffer, aSamplesToRead, aBufferSize, aScratch, aChannels);

            // clear voice if the sound is over
            // TODO: check this condition some day
            if (!voice->mFlags.Looping && !voice->mFlags.DisableAutostop && voice->hasEnded())
            {
                stopVoice_internal(mActiveVoice[i]);
            }
        }
        else if (voice && voice->mBusHandle == aBus && !voice->mFlags.Paused &&
                 voice->mFlags.Inaudible && voice->mFlags.InaudibleTick)
        {
            // Inaudible but needs ticking. Do minimal work (keep counters up to date and ask
            // audiosource for data)
            auto step       = voice->mSamplerate / aSamplerate;
            auto step_fixed = int(floor(step * FIXPOINT_FRAC_MUL));
            auto outofs     = size_t(0);

            if (voice->mDelaySamples)
            {
                if (voice->mDelaySamples > aSamplesToRead)
                {
                    outofs = aSamplesToRead;
                    voice->mDelaySamples -= aSamplesToRead;
                }
                else
                {
                    outofs               = voice->mDelaySamples;
                    voice->mDelaySamples = 0;
                }
            }

            while (step_fixed != 0 && outofs < aSamplesToRead)
            {
                if (voice->mLeftoverSamples == 0)
                {
                    // Swap resample buffers (ping-pong)
                    float* t                = voice->mResampleData[0];
                    voice->mResampleData[0] = voice->mResampleData[1];
                    voice->mResampleData[1] = t;

                    // Get a block of source data

                    if (!voice->hasEnded() || voice->mFlags.Looping)
                    {
                        auto readcount = voice->getAudio(voice->mResampleData[0],
                                                         SAMPLE_GRANULARITY,
                                                         SAMPLE_GRANULARITY);
                        if (readcount < SAMPLE_GRANULARITY)
                        {
                            if (voice->mFlags.Looping)
                            {
                                while (readcount < SAMPLE_GRANULARITY &&
                                       voice->seek(voice->mLoopPoint, mScratch.mData, mScratchSize))
                                {
                                    voice->mLoopCount++;
                                    readcount +=
                                        voice->getAudio(voice->mResampleData[0] + readcount,
                                                        SAMPLE_GRANULARITY - readcount,
                                                        SAMPLE_GRANULARITY);
                                }
                            }
                        }
                    }

                    // If we go past zero, crop to zero (a bit of a kludge)
                    if (voice->mSrcOffset < SAMPLE_GRANULARITY * FIXPOINT_FRAC_MUL)
                    {
                        voice->mSrcOffset = 0;
                    }
                    else
                    {
                        // We have new block of data, move pointer backwards
                        voice->mSrcOffset -= SAMPLE_GRANULARITY * FIXPOINT_FRAC_MUL;
                    }

                    // Skip filters
                }
                else
                {
                    voice->mLeftoverSamples = 0;
                }

                // Figure out how many samples we can generate from this source data.
                // The value may be zero.

                auto writesamples = size_t(0);

                if (voice->mSrcOffset < SAMPLE_GRANULARITY * FIXPOINT_FRAC_MUL)
                {
                    writesamples = ((SAMPLE_GRANULARITY * FIXPOINT_FRAC_MUL) - voice->mSrcOffset) /
                                   step_fixed +
                                   1;

                    // avoid reading past the current buffer..
                    if (((writesamples * step_fixed + voice->mSrcOffset) >> FIXPOINT_FRAC_BITS) >=
                        SAMPLE_GRANULARITY)
                        writesamples--;
                }


                // If this is too much for our output buffer, don't write that many:
                if (writesamples + outofs > aSamplesToRead)
                {
                    voice->mLeftoverSamples = (writesamples + outofs) - aSamplesToRead;
                    writesamples            = aSamplesToRead - outofs;
                }

                // Skip resampler

                // Keep track of how many samples we've written so far
                outofs += writesamples;

                // Move source pointer onwards (writesamples may be zero)
                voice->mSrcOffset += writesamples * step_fixed;
            }

            // clear voice if the sound is over
            // TODO: check this condition some day
            if (!voice->mFlags.Looping && !voice->mFlags.DisableAutostop && voice->hasEnded())
            {
                stopVoice_internal(mActiveVoice[i]);
            }
        }
    }
}

void Engine::mapResampleBuffers_internal()
{
    assert(mMaxActiveVoices < 256);
    auto live = std::array<char, 256>{};

    for (size_t i = 0; i < mMaxActiveVoices; ++i)
    {
        for (size_t j = 0; j < mMaxActiveVoices; ++j)
        {
            if (mResampleDataOwner[i] &&
                mResampleDataOwner[i].get() == mVoice[mActiveVoice[j]].get())
            {
                live[i] |= 1; // Live channel
                live[j] |= 2; // Live voice
            }
        }
    }

    for (size_t i = 0; i < mMaxActiveVoices; ++i)
    {
        if (!(live[i] & 1) && mResampleDataOwner[i]) // For all dead channels with owners..
        {
            mResampleDataOwner[i]->mResampleData[0] = nullptr;
            mResampleDataOwner[i]->mResampleData[1] = nullptr;
            mResampleDataOwner[i]                   = nullptr;
        }
    }

    auto latestfree = 0;

    for (size_t i = 0; i < mActiveVoiceCount; ++i)
    {
        if (!(live[i] & 2) && mVoice[mActiveVoice[i]]) // For all live voices with no channel..
        {
            int found = -1;

            for (size_t j = latestfree; found == -1 && j < mMaxActiveVoices; ++j)
            {
                if (mResampleDataOwner[j] == nullptr)
                {
                    found = j;
                }
            }

            assert(found != -1);
            mResampleDataOwner[found]                   = mVoice[mActiveVoice[i]];
            mResampleDataOwner[found]->mResampleData[0] = mResampleData[found * 2 + 0];
            mResampleDataOwner[found]->mResampleData[1] = mResampleData[found * 2 + 1];

            memset(mResampleDataOwner[found]->mResampleData[0],
                   0,
                   sizeof(float) * SAMPLE_GRANULARITY * MAX_CHANNELS);

            memset(mResampleDataOwner[found]->mResampleData[1],
                   0,
                   sizeof(float) * SAMPLE_GRANULARITY * MAX_CHANNELS);

            latestfree = found + 1;
        }
    }
}

void Engine::calcActiveVoices_internal()
{
    // TODO: consider whether we need to re-evaluate the active voices all the time.
    // It is a must when new voices are started, but otherwise we could get away
    // with postponing it sometimes..

    mActiveVoiceDirty = false;

    // Populate
    size_t candidates = 0;
    size_t mustlive   = 0;

    for (size_t i = 0; i < mHighestVoice; ++i)
    {
        const auto voice = mVoice[i];
        if (voice == nullptr)
        {
            continue;
        }

        // TODO: check this some day
        if ((!voice->mFlags.Inaudible && !voice->mFlags.Paused) || voice->mFlags.InaudibleTick)
        {
            mActiveVoice[candidates] = i;
            candidates++;
            if (mVoice[i]->mFlags.InaudibleTick)
            {
                mActiveVoice[candidates - 1] = mActiveVoice[mustlive];
                mActiveVoice[mustlive]       = i;
                mustlive++;
            }
        }
    }

    // Check for early out
    if (candidates <= mMaxActiveVoices)
    {
        // everything is audible, early out
        mActiveVoiceCount = candidates;
        mapResampleBuffers_internal();
        return;
    }

    mActiveVoiceCount = mMaxActiveVoices;

    if (mustlive >= mMaxActiveVoices)
    {
        // Oopsie. Well, nothing to sort, since the "must live" voices already
        // ate all our active voice slots.
        // This is a potentially an error situation, but we have no way to report
        // error from here. And asserting could be bad, too.
        return;
    }

    // If we get this far, there's nothing to it: we'll have to sort the voices to find the most
    // audible.

    // Iterative partial quicksort:
    int       left = 0, stack[24], pos = 0;
    int       len  = candidates - mustlive;
    size_t*   data = mActiveVoice.data() + mustlive;
    const int k    = mActiveVoiceCount;
    while (true)
    {
        for (; left + 1 < len; len++)
        {
            if (pos == 24)
            {
                len = stack[pos = 0];
            }

            const int   pivot    = data[left];
            const float pivotvol = mVoice[pivot]->mOverallVolume;
            stack[pos++]         = len;

            for (int right = left - 1;;)
            {
                do
                {
                    ++right;
                } while (mVoice[data[right]]->mOverallVolume > pivotvol);

                do
                {
                    --len;
                } while (pivotvol > mVoice[data[len]]->mOverallVolume);

                if (right >= len)
                {
                    break;
                }

                std::swap(data[left], data[right]);
            }
        }

        if (pos == 0)
        {
            break;
        }

        if (left >= k)
        {
            break;
        }

        left = len;
        len  = stack[--pos];
    }

    // TODO: should the rest of the voices be flagged INAUDIBLE?
    mapResampleBuffers_internal();
}

void Engine::mix_internal(size_t aSamples, size_t aStride)
{
#ifdef __arm__
    // flush to zero (FTZ) for ARM
    {
        static bool once = false;
        if (!once)
        {
            once = true;
            asm("vmsr fpscr,%0" ::"r"(1 << 24));
        }
    }
#endif

#ifdef _MCW_DN
    {
        static bool once = false;
        if (!once)
        {
            once = true;
            if (!mFlags.NoFpuRegisterChange)
            {
                _controlfp(_DN_FLUSH, _MCW_DN);
            }
        }
    }
#endif

#ifdef SOLOUD_SSE_INTRINSICS
    {
        static bool once = false;
        if (!once)
        {
            once = true;
            // Set denorm clear to zero (CTZ) and denorms are zero (DAZ) flags on.
            // This causes all math to consider really tiny values as zero, which
            // helps performance. I'd rather use constants from the sse headers,
            // but for some reason the DAZ value is not defined there(!)
            if (!mFlags.NoFpuRegisterChange)
            {
                _mm_setcsr(_mm_getcsr() | 0x8040);
            }
        }
    }
#endif

    const auto buffertime   = aSamples / float(mSamplerate);
    auto       globalVolume = std::array<float, 2>{};

    mStreamTime += buffertime;
    mLastClockedTime = 0;

    globalVolume[0] = mGlobalVolume;

    if (mGlobalVolumeFader.mActive)
    {
        mGlobalVolume = mGlobalVolumeFader.get(mStreamTime);
    }

    globalVolume[1] = mGlobalVolume;

    lockAudioMutex_internal();

    // Process faders. May change scratch size.
    for (size_t i = 0; i < mHighestVoice; ++i)
    {
        if (mVoice[i] != nullptr && !mVoice[i]->mFlags.Paused)
        {
            auto volume = std::array<float, 2>{};

            mVoice[i]->mActiveFader = 0;

            if (mGlobalVolumeFader.mActive > 0)
            {
                mVoice[i]->mActiveFader = 1;
            }

            mVoice[i]->mStreamTime += buffertime;
            mVoice[i]->mStreamPosition +=
                double(buffertime) * double(mVoice[i]->mOverallRelativePlaySpeed);

            // TODO: this is actually unstable, because mStreamTime depends on the relative play
            // speed.
            if (mVoice[i]->mRelativePlaySpeedFader.mActive > 0)
            {
                float speed = mVoice[i]->mRelativePlaySpeedFader.get(mVoice[i]->mStreamTime);
                setVoiceRelativePlaySpeed_internal(i, speed);
            }

            volume[0] = mVoice[i]->mOverallVolume;

            if (mVoice[i]->mVolumeFader.mActive > 0)
            {
                mVoice[i]->mSetVolume   = mVoice[i]->mVolumeFader.get(mVoice[i]->mStreamTime);
                mVoice[i]->mActiveFader = 1;
                updateVoiceVolume_internal(i);
                mActiveVoiceDirty = true;
            }

            volume[1] = mVoice[i]->mOverallVolume;

            if (mVoice[i]->mPanFader.mActive > 0)
            {
                float pan = mVoice[i]->mPanFader.get(mVoice[i]->mStreamTime);
                setVoicePan_internal(i, pan);
                mVoice[i]->mActiveFader = 1;
            }

            if (mVoice[i]->mPauseScheduler.mActive)
            {
                mVoice[i]->mPauseScheduler.get(mVoice[i]->mStreamTime);
                if (mVoice[i]->mPauseScheduler.mActive == -1)
                {
                    mVoice[i]->mPauseScheduler.mActive = 0;
                    setVoicePause_internal(i, 1);
                }
            }

            if (mVoice[i]->mStopScheduler.mActive)
            {
                mVoice[i]->mStopScheduler.get(mVoice[i]->mStreamTime);
                if (mVoice[i]->mStopScheduler.mActive == -1)
                {
                    mVoice[i]->mStopScheduler.mActive = 0;
                    stopVoice_internal(i);
                }
            }
        }
    }

    if (mActiveVoiceDirty)
    {
        calcActiveVoices_internal();
    }

    mixBus_internal(mOutputScratch.mData,
                    aSamples,
                    aStride,
                    mScratch.mData,
                    0,
                    float(mSamplerate),
                    mChannels,
                    mResampler);

    for (size_t i = 0; i < FILTERS_PER_STREAM; ++i)
    {
        if (mFilterInstance[i])
        {
            mFilterInstance[i]->filter(mOutputScratch.mData,
                                       aSamples,
                                       aStride,
                                       mChannels,
                                       float(mSamplerate),
                                       mStreamTime);
        }
    }

    unlockAudioMutex_internal();

    // Note: clipping channels*aStride, not channels*aSamples, so we're possibly clipping some
    // unused data. The buffers should be large enough for it, we just may do a few bytes of
    // unneccessary work.
    clip_internal(mOutputScratch, mScratch, aStride, globalVolume[0], globalVolume[1]);

    if (mFlags.EnableVisualization)
    {
        for (size_t i = 0; i < MAX_CHANNELS; ++i)
        {
            mVisualizationChannelVolume[i] = 0;
        }

        if (aSamples > 255)
        {
            for (size_t i = 0; i < 256; ++i)
            {
                mVisualizationWaveData[i] = 0;
                for (size_t j = 0; j < mChannels; ++j)
                {
                    const auto sample = mScratch.mData[i + j * aStride];
                    const auto absvol = fabs(sample);

                    if (mVisualizationChannelVolume[j] < absvol)
                    {
                        mVisualizationChannelVolume[j] = absvol;
                    }

                    mVisualizationWaveData[i] += sample;
                }
            }
        }
        else
        {
            // Very unlikely failsafe branch
            for (size_t i = 0; i < 256; ++i)
            {
                mVisualizationWaveData[i] = 0;

                for (size_t j = 0; j < mChannels; ++j)
                {
                    const auto sample = mScratch.mData[(i % aSamples) + j * aStride];
                    const auto absvol = fabs(sample);

                    if (mVisualizationChannelVolume[j] < absvol)
                    {
                        mVisualizationChannelVolume[j] = absvol;
                    }

                    mVisualizationWaveData[i] += sample;
                }
            }
        }
    }
}

void Engine::mix(float* aBuffer, size_t aSamples)
{
    size_t stride = (aSamples + 15) & ~0xf;
    mix_internal(aSamples, stride);
    interlace_samples_float(mScratch.mData, aBuffer, aSamples, mChannels, stride);
}

void Engine::mixSigned16(short* aBuffer, size_t aSamples)
{
    size_t stride = (aSamples + 15) & ~0xf;
    mix_internal(aSamples, stride);
    interlace_samples_s16(mScratch.mData, aBuffer, aSamples, mChannels, stride);
}

void interlace_samples_float(const float* aSourceBuffer,
                             float*       aDestBuffer,
                             size_t       aSamples,
                             size_t       aChannels,
                             size_t       aStride)
{
    // 111222 -> 121212
    for (size_t j = 0; j < aChannels; ++j)
    {
        auto c = j * aStride;
        for (auto i = j; i < aSamples * aChannels; i += aChannels)
        {
            aDestBuffer[i] = aSourceBuffer[c];
            ++c;
        }
    }
}

void interlace_samples_s16(const float* aSourceBuffer,
                           short*       aDestBuffer,
                           size_t       aSamples,
                           size_t       aChannels,
                           size_t       aStride)
{
    // 111222 -> 121212
    for (size_t j = 0; j < aChannels; ++j)
    {
        size_t c = j * aStride;

        for (size_t i = j; i < aSamples * aChannels; i += aChannels)
        {
            aDestBuffer[i] = short(aSourceBuffer[c] * 0x7fff);
            ++c;
        }
    }
}

void Engine::lockAudioMutex_internal()
{
    if (mAudioThreadMutex)
    {
        Thread::lockMutex(mAudioThreadMutex);
    }
    assert(!mInsideAudioThreadMutex);
    mInsideAudioThreadMutex = true;
}

void Engine::unlockAudioMutex_internal()
{
    assert(mInsideAudioThreadMutex);
    mInsideAudioThreadMutex = false;
    if (mAudioThreadMutex)
    {
        Thread::unlockMutex(mAudioThreadMutex);
    }
}
}; // namespace cer
