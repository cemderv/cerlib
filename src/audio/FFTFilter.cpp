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

#include "audio/Common.hpp"
#include "audio/FFT.hpp"
#include "audio/Filter.hpp"
#include <cstring>

namespace cer
{
static constexpr size_t STFT_WINDOW_SIZE  = 256; // must be power of two
static constexpr size_t STFT_WINDOW_HALF  = STFT_WINDOW_SIZE / 2;
static constexpr size_t STFT_WINDOW_TWICE = STFT_WINDOW_SIZE * 2;

// Needed for subclasses
FFTFilterInstance::FFTFilterInstance()
{
    for (size_t i = 0; i < max_channels; ++i)
    {
        m_input_offset[i] = STFT_WINDOW_SIZE;
        m_mix_offset[i]   = STFT_WINDOW_HALF;
        m_read_offset[i]  = 0;
    }
}

FFTFilterInstance::FFTFilterInstance(FFTFilter* parent)
    : FFTFilterInstance()
{
    m_parent = parent;
    FilterInstance::init_params(1);
}

void FFTFilterInstance::filter_channel(const FilterChannelArgs& args)
{
    if (args.channel == 0)
    {
        update_params(args.time);
    }

    // Chicken-egg problem: we don't know channel count before this.
    // Could allocate max_channels but that would potentially waste a lot of memory.
    if (m_input_buffer == nullptr)
    {
        m_input_buffer = std::make_unique<float[]>(STFT_WINDOW_TWICE * args.channel_count);
        m_mix_buffer   = std::make_unique<float[]>(STFT_WINDOW_TWICE * args.channel_count);
        m_temp         = std::make_unique<float[]>(STFT_WINDOW_SIZE);
        m_last_phase   = std::make_unique<float[]>(STFT_WINDOW_SIZE * args.channel_count);
        m_sum_phase    = std::make_unique<float[]>(STFT_WINDOW_SIZE * args.channel_count);
    }

    size_t ofs      = 0;
    size_t chofs    = STFT_WINDOW_TWICE * args.channel;
    size_t inputofs = m_input_offset[args.channel];
    size_t mixofs   = m_mix_offset[args.channel];
    size_t readofs  = m_read_offset[args.channel];

    while (ofs < args.samples)
    {
        size_t samples = STFT_WINDOW_HALF - (inputofs & (STFT_WINDOW_HALF - 1));

        if (ofs + samples > args.samples)
        {
            assert(args.samples >= ofs);
            samples = args.samples - ofs;
        }

        for (size_t i = 0; i < samples; ++i)
        {
            m_input_buffer[chofs + ((inputofs + STFT_WINDOW_HALF) & (STFT_WINDOW_TWICE - 1))] =
                args.buffer[ofs + i];
            m_mix_buffer[chofs + ((inputofs + STFT_WINDOW_HALF) & (STFT_WINDOW_TWICE - 1))] = 0;
            inputofs++;
        }

        if ((inputofs & (STFT_WINDOW_HALF - 1)) == 0)
        {
            for (size_t i = 0; i < STFT_WINDOW_SIZE; ++i)
            {
                m_temp[i] =
                    m_input_buffer[chofs + ((inputofs + STFT_WINDOW_TWICE - STFT_WINDOW_HALF + i) &
                                            (STFT_WINDOW_TWICE - 1))];
            }

            FFT::fft(m_temp.get(), STFT_WINDOW_SIZE);

            // do magic
            fft_filter_channel(FilterChannelArgs{
                .buffer        = m_temp.get(),
                .samples       = STFT_WINDOW_HALF,
                .sample_rate   = args.sample_rate,
                .time          = args.time,
                .channel       = args.channel,
                .channel_count = args.channel_count,
            });

            FFT::ifft(m_temp.get(), STFT_WINDOW_SIZE);

            for (size_t i = 0; i < STFT_WINDOW_SIZE; ++i)
            {
                // mMixBuffer[chofs + (mixofs & (STFT_WINDOW_TWICE - 1))] = mTemp[i];
                m_mix_buffer[chofs + (mixofs & (STFT_WINDOW_TWICE - 1))] +=
                    m_temp[i] * (float(STFT_WINDOW_HALF) - std::abs(float(STFT_WINDOW_HALF - i))) *
                    (1.0f / float(STFT_WINDOW_HALF));

                mixofs++;
            }

            mixofs -= STFT_WINDOW_HALF;
        }

        for (size_t i = 0; i < samples; ++i)
        {
            args.buffer[ofs + i] +=
                (m_mix_buffer[chofs + (readofs & (STFT_WINDOW_TWICE - 1))] - args.buffer[ofs + i]) *
                m_params[0];

            readofs++;
        }

        ofs += samples;
    }

    m_input_offset[args.channel] = inputofs;
    m_read_offset[args.channel]  = readofs;
    m_mix_offset[args.channel]   = mixofs;
}

void FFTFilterInstance::comp2MagPhase(float* fft_buffer, size_t samples)
{
    for (size_t i = 0; i < samples; ++i)
    {
        const float re = fft_buffer[i * 2];
        const float im = fft_buffer[i * 2 + 1];

        fft_buffer[i * 2]     = sqrt(re * re + im * im) * 2;
        fft_buffer[i * 2 + 1] = atan2(im, re);
    }
}

void FFTFilterInstance::magPhase2MagFreq(float* fft_buffer,
                                         size_t samples,
                                         float  sample_rate,
                                         size_t channel)
{
    const auto stepsize   = samples / sample_rate;
    const auto expct      = (stepsize / samples) * 2.0f * (float)M_PI;
    const auto freqPerBin = sample_rate / samples;

    for (size_t i = 0; i < samples; ++i)
    {
        const auto pha = fft_buffer[i * 2 + 1];

        /* compute phase difference */
        auto freq = pha - m_last_phase[i + channel * STFT_WINDOW_SIZE];

        m_last_phase[i + channel * STFT_WINDOW_SIZE] = pha;

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
        freq = samples * freq / (2.0f * float(M_PI));

        /* compute the k-th partials' true frequency */
        freq = float(i) * freqPerBin + freq * freqPerBin;

        /* store magnitude and true frequency in analysis arrays */
        fft_buffer[i * 2 + 1] = freq;
    }
}

void FFTFilterInstance::magFreq2MagPhase(float* fft_buffer,
                                         size_t samples,
                                         float  sample_rate,
                                         size_t channel)
{
    const float stepsize   = samples / sample_rate;
    const float expct      = (stepsize / samples) * 2.0f * float(M_PI);
    const float freqPerBin = sample_rate / samples;

    for (size_t i = 0; i < samples; ++i)
    {
        /* get magnitude and true frequency from synthesis arrays */
        float freq = fft_buffer[i * 2 + 1];

        /* subtract bin mid frequency */
        freq -= float(i) * freqPerBin;

        /* get bin deviation from freq deviation */
        freq /= freqPerBin;

        /* take osamp into account */
        freq = (freq / samples) * pi * 2.0f;

        /* add the overlap phase advance back in */
        freq += float(i) * expct;

        /* accumulate delta phase to get bin phase */

        m_sum_phase[i + channel * STFT_WINDOW_SIZE] += freq;
        fft_buffer[i * 2 + 1] = m_sum_phase[i + channel * STFT_WINDOW_SIZE];
    }
}

void FFTFilterInstance::magPhase2Comp(float* fft_buffer, size_t samples)
{
    for (size_t i = 0; i < samples; ++i)
    {
        const float mag = fft_buffer[i * 2];
        const float pha = fft_buffer[i * 2 + 1];

        fft_buffer[i * 2]     = cos(pha) * mag;
        fft_buffer[i * 2 + 1] = sin(pha) * mag;
    }
}

void FFTFilterInstance::fft_filter_channel(const FilterChannelArgs& args)
{
    comp2MagPhase(args.buffer, args.samples);
    magPhase2MagFreq(args.buffer, args.samples, args.sample_rate, args.channel);

    auto t = std::array<float, STFT_WINDOW_TWICE>{};

    memcpy(t.data(), args.buffer, sizeof(float) * args.samples);
    memset(args.buffer, 0, sizeof(float) * args.samples * 2);

    for (size_t i = 0; i < args.samples / 4; ++i)
    {
        if (const auto d = i * 2; d < args.samples / 4)
        {
            args.buffer[d * 2] += t[i * 2];
            args.buffer[d * 2 + 1] = t[i * 2 + 1] * 2;
        }
    }

    magFreq2MagPhase(args.buffer, args.samples, args.sample_rate, args.channel);
    magPhase2Comp(args.buffer, args.samples);
}

auto FFTFilter::create_instance() -> std::shared_ptr<FilterInstance>
{
    return std::make_shared<FFTFilterInstance>(this);
}
} // namespace cer