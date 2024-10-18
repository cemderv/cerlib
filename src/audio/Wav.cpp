/*
SoLoud audio engine
Copyright (c) 2013-2018 Jari Komppa

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

#include "audio/Wav.hpp"
#include "audio/Common.hpp"
#include "audio/MemoryFile.hpp"
#include "dr_flac.h"
#include "dr_mp3.h"
#include "dr_wav.h"
#include "stb_vorbis.h"
#include <cstring>

#define MAKEDWORD(a, b, c, d) (((d) << 24) | ((c) << 16) | ((b) << 8) | (a))

namespace cer
{
WavInstance::WavInstance(Wav* aParent)
{
    mParent = aParent;
    mOffset = 0;
}

size_t WavInstance::getAudio(float* aBuffer, size_t aSamplesToRead, size_t aBufferSize)
{
    if (mParent->mData == nullptr)
        return 0;

    size_t dataleft = mParent->mSampleCount - mOffset;
    size_t copylen  = dataleft;

    if (copylen > aSamplesToRead)
        copylen = aSamplesToRead;

    for (size_t i = 0; i < mChannels; ++i)
    {
        memcpy(aBuffer + i * aBufferSize,
               mParent->mData.get() + mOffset + i * mParent->mSampleCount,
               sizeof(float) * copylen);
    }

    mOffset += copylen;
    return copylen;
}

bool WavInstance::rewind()
{
    mOffset         = 0;
    mStreamPosition = 0.0f;
    return true;
}

bool WavInstance::hasEnded()
{
    return !mFlags.Looping && mOffset >= mParent->mSampleCount;
}

Wav::Wav(std::span<const std::byte> data)
{
    assert(data.data() != nullptr);
    assert(data.size() > 0);

    auto dr = MemoryFile{data};

    channel_count = 1;

    switch (dr.read32())
    {
        case MAKEDWORD('O', 'g', 'g', 'S'): loadogg(dr); break;
        case MAKEDWORD('R', 'I', 'F', 'F'): loadwav(dr); break;
        case MAKEDWORD('f', 'L', 'a', 'C'): loadflac(dr); break;
        default: loadmp3(dr); break;
    }
}

Wav::~Wav()
{
    stop();
}

void Wav::loadwav(const MemoryFile& aReader)
{
    drwav decoder;

    if (!drwav_init_memory(&decoder, aReader.data(), aReader.size(), nullptr))
    {
        throw std::runtime_error{"Failed to load WAV"};
    }

    drwav_uint64 samples = decoder.totalPCMFrameCount;

    if (!samples)
    {
        drwav_uninit(&decoder);
        throw std::runtime_error{"Failed to load WAV"};
    }

    mData            = std::make_unique<float[]>(samples * decoder.channels);
    base_sample_rate = float(decoder.sampleRate);
    mSampleCount     = samples;
    channel_count    = decoder.channels;

    for (size_t i = 0; i < mSampleCount; i += 512)
    {
        float      tmp[512 * max_channels];
        const auto blockSize = (mSampleCount - i) > 512 ? 512 : mSampleCount - i;
        drwav_read_pcm_frames_f32(&decoder, blockSize, tmp);

        for (size_t j = 0; j < blockSize; ++j)
        {
            for (size_t k = 0; k < decoder.channels; k++)
            {
                mData[k * mSampleCount + i + j] = tmp[j * decoder.channels + k];
            }
        }
    }

    drwav_uninit(&decoder);
}

void Wav::loadogg(const MemoryFile& aReader)
{
    int   e      = 0;
    auto* vorbis = stb_vorbis_open_memory(aReader.data_uc(), aReader.size(), &e, 0);

    if (vorbis == nullptr)
    {
        throw std::runtime_error{"Failed to load OGG"};
    }

    const auto info  = stb_vorbis_get_info(vorbis);
    base_sample_rate = float(info.sample_rate);
    auto samples     = stb_vorbis_stream_length_in_samples(vorbis);

    if (size_t(info.channels) > max_channels)
    {
        channel_count = max_channels;
    }
    else
    {
        channel_count = info.channels;
    }

    mData        = std::make_unique<float[]>(samples * channel_count);
    mSampleCount = samples;
    samples      = 0;

    while (true)
    {
        float**   outputs = nullptr;
        const int n       = stb_vorbis_get_frame_float(vorbis, nullptr, &outputs);

        if (n == 0)
        {
            break;
        }

        for (size_t ch = 0; ch < channel_count; ch++)
        {
            memcpy(mData.get() + samples + mSampleCount * ch, outputs[ch], sizeof(float) * n);
        }

        samples += n;
    }

    stb_vorbis_close(vorbis);
}

void Wav::loadmp3(const MemoryFile& aReader)
{
    drmp3 decoder;

    if (!drmp3_init_memory(&decoder, aReader.data_uc(), aReader.size(), nullptr))
    {
        throw std::runtime_error{"Failed to load MP3"};
    }

    const auto samples = drmp3_get_pcm_frame_count(&decoder);

    if (!samples)
    {
        drmp3_uninit(&decoder);
        throw std::runtime_error{"Failed to load MP3"};
    }

    mData            = std::make_unique<float[]>(samples * decoder.channels);
    base_sample_rate = float(decoder.sampleRate);
    mSampleCount     = samples;
    channel_count    = decoder.channels;
    drmp3_seek_to_pcm_frame(&decoder, 0);

    for (size_t i = 0; i < mSampleCount; i += 512)
    {
        auto         tmp       = std::array<float, 512 * max_channels>{};
        const size_t blockSize = (mSampleCount - i) > 512 ? 512 : mSampleCount - i;
        drmp3_read_pcm_frames_f32(&decoder, blockSize, tmp.data());

        for (size_t j = 0; j < blockSize; ++j)
        {
            for (size_t k = 0; k < decoder.channels; k++)
            {
                mData[k * mSampleCount + i + j] = tmp[j * decoder.channels + k];
            }
        }
    }

    drmp3_uninit(&decoder);
}

void Wav::loadflac(const MemoryFile& aReader)
{
    drflac* decoder = drflac_open_memory(aReader.data(), aReader.size(), nullptr);

    if (!decoder)
    {
        throw std::runtime_error{"Failed to load FLAC"};
    }

    const auto samples = decoder->totalPCMFrameCount;

    if (!samples)
    {
        drflac_close(decoder);
        throw std::runtime_error{"Failed to load FLAC"};
    }

    mData            = std::make_unique<float[]>(samples * decoder->channels);
    base_sample_rate = float(decoder->sampleRate);
    mSampleCount     = samples;
    channel_count    = decoder->channels;
    drflac_seek_to_pcm_frame(decoder, 0);

    for (size_t i = 0; i < mSampleCount; i += 512)
    {
        auto         tmp       = std::array<float, 512 * max_channels>{};
        const size_t blockSize = (mSampleCount - i) > 512 ? 512 : mSampleCount - i;
        drflac_read_pcm_frames_f32(decoder, blockSize, tmp.data());
        for (size_t j = 0; j < blockSize; ++j)
        {
            for (size_t k = 0; k < decoder->channels; k++)
            {
                mData[k * mSampleCount + i + j] = tmp[j * decoder->channels + k];
            }
        }
    }

    drflac_close(decoder);
}

std::shared_ptr<AudioSourceInstance> Wav::createInstance()
{
    return std::make_shared<WavInstance>(this);
}

double Wav::getLength() const
{
    return base_sample_rate == 0 ? 0 : mSampleCount / base_sample_rate;
}
}; // namespace cer
