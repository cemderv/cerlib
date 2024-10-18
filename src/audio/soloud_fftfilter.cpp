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

/*
 * Transformations largely based on smbPitchShift.cpp
 * COPYRIGHT 1999-2015 Stephan M. Bernsee <s.bernsee [AT] zynaptiq [DOT] com>
 * http://blogs.zynaptiq.com/bernsee
 * The Wide Open License (WOL)
 *
 * Permission to use, copy, modify, distribute and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice and this license appear in all source copies.
 * THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT EXPRESS OR IMPLIED WARRANTY OF
 * ANY KIND. See http://www.dspguru.com/wol.htm for more information.
 */

#include "soloud.hpp"
#include "soloud_fft.hpp"
#include "soloud_filter.hpp"
#include <cstring>

namespace cer
{
static constexpr auto STFT_WINDOW_SIZE  = 256; // must be power of two
static constexpr auto STFT_WINDOW_HALF  = STFT_WINDOW_SIZE / 2;
static constexpr auto STFT_WINDOW_TWICE = STFT_WINDOW_SIZE * 2;

// Needed for subclasses
FFTFilterInstance::FFTFilterInstance()
{
    for (size_t i = 0; i < MAX_CHANNELS; ++i)
    {
        mInputOffset[i] = STFT_WINDOW_SIZE;
        mMixOffset[i]   = STFT_WINDOW_HALF;
        mReadOffset[i]  = 0;
    }
}

FFTFilterInstance::FFTFilterInstance(FFTFilter* aParent)
    : FFTFilterInstance()
{
    mParent = aParent;
    FilterInstance::initParams(1);
}

void FFTFilterInstance::filterChannel(float* aBuffer,
                                      size_t aSamples,
                                      float  aSamplerate,
                                      double aTime,
                                      size_t aChannel,
                                      size_t aChannels)
{
    if (aChannel == 0)
    {
        updateParams(aTime);
    }

    // Chicken-egg problem: we don't know channel count before this.
    // Could allocate max_channels but that would potentially waste a lot of memory.
    if (mInputBuffer == 0)
    {
        mInputBuffer = std::make_unique<float[]>(STFT_WINDOW_TWICE * aChannels);
        mMixBuffer   = std::make_unique<float[]>(STFT_WINDOW_TWICE * aChannels);
        mTemp        = std::make_unique<float[]>(STFT_WINDOW_SIZE);
        mLastPhase   = std::make_unique<float[]>(STFT_WINDOW_SIZE * aChannels);
        mSumPhase    = std::make_unique<float[]>(STFT_WINDOW_SIZE * aChannels);
    }

    int    i;
    size_t ofs      = 0;
    size_t chofs    = STFT_WINDOW_TWICE * aChannel;
    size_t inputofs = mInputOffset[aChannel];
    size_t mixofs   = mMixOffset[aChannel];
    size_t readofs  = mReadOffset[aChannel];

    while (ofs < aSamples)
    {
        int samples = STFT_WINDOW_HALF - (inputofs & (STFT_WINDOW_HALF - 1));

        if (ofs + samples > aSamples)
            samples = aSamples - ofs;

        for (i = 0; i < samples; ++i)
        {
            mInputBuffer[chofs + ((inputofs + STFT_WINDOW_HALF) & (STFT_WINDOW_TWICE - 1))] =
                aBuffer[ofs + i];
            mMixBuffer[chofs + ((inputofs + STFT_WINDOW_HALF) & (STFT_WINDOW_TWICE - 1))] = 0;
            inputofs++;
        }

        if ((inputofs & (STFT_WINDOW_HALF - 1)) == 0)
        {
            for (i = 0; i < STFT_WINDOW_SIZE; ++i)
            {
                mTemp[i] =
                    mInputBuffer[chofs + ((inputofs + STFT_WINDOW_TWICE - STFT_WINDOW_HALF + i) &
                                          (STFT_WINDOW_TWICE - 1))];
            }

            FFT::fft(mTemp.get(), STFT_WINDOW_SIZE);

            // do magic
            fftFilterChannel(mTemp.get(),
                             STFT_WINDOW_HALF,
                             aSamplerate,
                             aTime,
                             aChannel,
                             aChannels);

            FFT::ifft(mTemp.get(), STFT_WINDOW_SIZE);

            for (i = 0; i < STFT_WINDOW_SIZE; ++i)
            {
                // mMixBuffer[chofs + (mixofs & (STFT_WINDOW_TWICE - 1))] = mTemp[i];
                mMixBuffer[chofs + (mixofs & (STFT_WINDOW_TWICE - 1))] +=
                    mTemp[i] * ((float)STFT_WINDOW_HALF - abs(STFT_WINDOW_HALF - i)) *
                    (1.0f / (float)STFT_WINDOW_HALF);

                mixofs++;
            }

            mixofs -= STFT_WINDOW_HALF;
        }

        for (i = 0; i < samples; ++i)
        {
            aBuffer[ofs + i] +=
                (mMixBuffer[chofs + (readofs & (STFT_WINDOW_TWICE - 1))] - aBuffer[ofs + i]) *
                mParam[0];
            readofs++;
        }

        ofs += samples;
    }
    mInputOffset[aChannel] = inputofs;
    mReadOffset[aChannel]  = readofs;
    mMixOffset[aChannel]   = mixofs;
}

void FFTFilterInstance::comp2MagPhase(float* aFFTBuffer, size_t aSamples)
{
    for (size_t i = 0; i < aSamples; ++i)
    {
        float re              = aFFTBuffer[i * 2];
        float im              = aFFTBuffer[i * 2 + 1];
        aFFTBuffer[i * 2]     = sqrt(re * re + im * im) * 2;
        aFFTBuffer[i * 2 + 1] = atan2(im, re);
    }
}

void FFTFilterInstance::magPhase2MagFreq(float* aFFTBuffer,
                                         size_t aSamples,
                                         float  aSamplerate,
                                         size_t aChannel)
{
    float stepsize   = aSamples / aSamplerate;
    float expct      = (stepsize / aSamples) * 2.0f * (float)M_PI;
    float freqPerBin = aSamplerate / aSamples;
    for (size_t i = 0; i < aSamples; ++i)
    {
        const float pha = aFFTBuffer[i * 2 + 1];

        /* compute phase difference */
        float freq = pha - mLastPhase[i + aChannel * STFT_WINDOW_SIZE];
        mLastPhase[i + aChannel * STFT_WINDOW_SIZE] = pha;

        /* subtract expected phase difference */
        freq -= float(i) * expct;

        /* map delta phase into +/- Pi interval */
        int qpd = int(floor(freq / M_PI));
        if (qpd >= 0)
            qpd += qpd & 1;
        else
            qpd -= qpd & 1;
        freq -= float(M_PI) * float(qpd);

        /* get deviation from bin frequency from the +/- Pi interval */
        freq = aSamples * freq / (2.0f * float(M_PI));

        /* compute the k-th partials' true frequency */
        freq = float(i) * freqPerBin + freq * freqPerBin;

        /* store magnitude and true frequency in analysis arrays */
        aFFTBuffer[i * 2 + 1] = freq;
    }
}

void FFTFilterInstance::magFreq2MagPhase(float* aFFTBuffer,
                                         size_t aSamples,
                                         float  aSamplerate,
                                         size_t aChannel)
{
    const float stepsize   = aSamples / aSamplerate;
    const float expct      = (stepsize / aSamples) * 2.0f * float(M_PI);
    const float freqPerBin = aSamplerate / aSamples;

    for (size_t i = 0; i < aSamples; ++i)
    {
        /* get magnitude and true frequency from synthesis arrays */
        float freq = aFFTBuffer[i * 2 + 1];

        /* subtract bin mid frequency */
        freq -= float(i) * freqPerBin;

        /* get bin deviation from freq deviation */
        freq /= freqPerBin;

        /* take osamp into account */
        freq = (freq / aSamples) * pi * 2.0f;

        /* add the overlap phase advance back in */
        freq += float(i) * expct;

        /* accumulate delta phase to get bin phase */

        mSumPhase[i + aChannel * STFT_WINDOW_SIZE] += freq;
        aFFTBuffer[i * 2 + 1] = mSumPhase[i + aChannel * STFT_WINDOW_SIZE];
    }
}

void FFTFilterInstance::magPhase2Comp(float* aFFTBuffer, size_t aSamples)
{
    for (size_t i = 0; i < aSamples; ++i)
    {
        float mag             = aFFTBuffer[i * 2];
        float pha             = aFFTBuffer[i * 2 + 1];
        aFFTBuffer[i * 2]     = cos(pha) * mag;
        aFFTBuffer[i * 2 + 1] = sin(pha) * mag;
    }
}

void FFTFilterInstance::fftFilterChannel(float* aFFTBuffer,
                                         size_t aSamples,
                                         float  aSamplerate,
                                         time_t /*aTime*/,
                                         size_t aChannel,
                                         size_t /*aChannels*/)
{
    comp2MagPhase(aFFTBuffer, aSamples);
    magPhase2MagFreq(aFFTBuffer, aSamples, aSamplerate, aChannel);

    float t[STFT_WINDOW_TWICE];
    memcpy(t, aFFTBuffer, sizeof(float) * aSamples);
    memset(aFFTBuffer, 0, sizeof(float) * aSamples * 2);

    for (size_t i = 0; i < aSamples / 4; ++i)
    {
        if (const size_t d = i * 2; d < aSamples / 4)
        {
            aFFTBuffer[d * 2] += t[i * 2];
            aFFTBuffer[d * 2 + 1] = t[i * 2 + 1] * 2;
        }
    }

    magFreq2MagPhase(aFFTBuffer, aSamples, aSamplerate, aChannel);
    magPhase2Comp(aFFTBuffer, aSamples);
}

std::shared_ptr<FilterInstance> FFTFilter::createInstance()
{
    return std::make_shared<FFTFilterInstance>(this);
}
} // namespace cer
