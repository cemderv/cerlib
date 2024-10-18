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
#include "dr_flac.h"
#include "dr_mp3.h"
#include "dr_wav.h"
#include "stb_vorbis.h"
#include "util/MemoryReader.hpp"
#include <algorithm>
#include <cstring>

#define MAKEDWORD(a, b, c, d) (((d) << 24) | ((c) << 16) | ((b) << 8) | (a))

namespace cer
{
WavInstance::WavInstance(Wav* aParent)
{
    m_parent = aParent;
    m_offset = 0;
}

auto WavInstance::audio(float* buffer, size_t samples_to_read, size_t buffer_size) -> size_t
{
    if (m_parent->m_data == nullptr)
    {
        return 0;
    }

    const auto data_left   = m_parent->m_sample_count - m_offset;
    const auto copy_length = std::min(data_left, samples_to_read);

    for (size_t i = 0; i < channel_count; ++i)
    {
        memcpy(buffer + (i * buffer_size),
               m_parent->m_data.get() + m_offset + (i * m_parent->m_sample_count),
               sizeof(float) * copy_length);
    }

    m_offset += copy_length;

    return copy_length;
}

auto WavInstance::rewind() -> bool
{
    m_offset        = 0;
    stream_position = 0.0f;
    return true;
}

auto WavInstance::has_ended() -> bool
{
    return !flags.Looping && m_offset >= m_parent->m_sample_count;
}

Wav::Wav(std::span<const std::byte> data)
{
    assert(data.data() != nullptr);
    assert(!data.empty());

    auto dr = MemoryReader{data};

    channel_count = 1;

    switch (dr.read_u32())
    {
        case MAKEDWORD('O', 'g', 'g', 'S'): load_ogg(dr); break;
        case MAKEDWORD('R', 'I', 'F', 'F'): load_wav(dr); break;
        case MAKEDWORD('f', 'L', 'a', 'C'): load_flac(dr); break;
        default: load_mp3(dr); break;
    }
}

Wav::~Wav()
{
    stop();
}

void Wav::load_wav(const MemoryReader& reader)
{
    drwav decoder;

    if (drwav_init_memory(&decoder, reader.data(), reader.size(), nullptr) == 0u)
    {
        throw std::runtime_error{"Failed to load WAV"};
    }

    drwav_uint64 samples = decoder.totalPCMFrameCount;

    if (samples == 0u)
    {
        drwav_uninit(&decoder);
        throw std::runtime_error{"Failed to load WAV"};
    }

    m_data           = std::make_unique<float[]>(samples * decoder.channels);
    base_sample_rate = float(decoder.sampleRate);
    m_sample_count   = samples;
    channel_count    = decoder.channels;

    for (size_t i = 0; i < m_sample_count; i += 512)
    {
        auto       tmp       = std::array<float, 512 * max_channels>{};
        const auto blockSize = (m_sample_count - i) > 512 ? 512 : m_sample_count - i;

        drwav_read_pcm_frames_f32(&decoder, blockSize, tmp.data());

        for (size_t j = 0; j < blockSize; ++j)
        {
            for (size_t k = 0; k < decoder.channels; k++)
            {
                m_data[(k * m_sample_count) + i + j] = tmp[(j * decoder.channels) + k];
            }
        }
    }

    drwav_uninit(&decoder);
}

void Wav::load_ogg(const MemoryReader& reader)
{
    int   e      = 0;
    auto* vorbis = stb_vorbis_open_memory(reader.data_uc(), reader.size(), &e, nullptr);

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

    m_data         = std::make_unique<float[]>(samples * channel_count);
    m_sample_count = samples;
    samples        = 0;

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
            memcpy(m_data.get() + samples + (m_sample_count * ch), outputs[ch], sizeof(float) * n);
        }

        samples += n;
    }

    stb_vorbis_close(vorbis);
}

void Wav::load_mp3(const MemoryReader& reader)
{
    drmp3 decoder;

    if (drmp3_init_memory(&decoder, reader.data_uc(), reader.size(), nullptr) == 0u)
    {
        throw std::runtime_error{"Failed to load MP3"};
    }

    const auto samples = drmp3_get_pcm_frame_count(&decoder);

    if (samples == 0u)
    {
        drmp3_uninit(&decoder);
        throw std::runtime_error{"Failed to load MP3"};
    }

    m_data           = std::make_unique<float[]>(samples * decoder.channels);
    base_sample_rate = float(decoder.sampleRate);
    m_sample_count   = samples;
    channel_count    = decoder.channels;
    drmp3_seek_to_pcm_frame(&decoder, 0);

    for (size_t i = 0; i < m_sample_count; i += 512)
    {
        auto         tmp       = std::array<float, 512 * max_channels>{};
        const size_t blockSize = (m_sample_count - i) > 512 ? 512 : m_sample_count - i;
        drmp3_read_pcm_frames_f32(&decoder, blockSize, tmp.data());

        for (size_t j = 0; j < blockSize; ++j)
        {
            for (size_t k = 0; k < decoder.channels; k++)
            {
                m_data[(k * m_sample_count) + i + j] = tmp[j * decoder.channels + k];
            }
        }
    }

    drmp3_uninit(&decoder);
}

void Wav::load_flac(const MemoryReader& reader)
{
    drflac* decoder = drflac_open_memory(reader.data(), reader.size(), nullptr);

    if (decoder == nullptr)
    {
        throw std::runtime_error{"Failed to load FLAC"};
    }

    const auto samples = decoder->totalPCMFrameCount;

    if (samples == 0u)
    {
        drflac_close(decoder);
        throw std::runtime_error{"Failed to load FLAC"};
    }

    m_data           = std::make_unique<float[]>(samples * decoder->channels);
    base_sample_rate = float(decoder->sampleRate);
    m_sample_count   = samples;
    channel_count    = decoder->channels;
    drflac_seek_to_pcm_frame(decoder, 0);

    for (size_t i = 0; i < m_sample_count; i += 512)
    {
        auto         tmp       = std::array<float, 512 * max_channels>{};
        const size_t blockSize = (m_sample_count - i) > 512 ? 512 : m_sample_count - i;
        drflac_read_pcm_frames_f32(decoder, blockSize, tmp.data());

        for (size_t j = 0; j < blockSize; ++j)
        {
            for (size_t k = 0; k < decoder->channels; k++)
            {
                m_data[(k * m_sample_count) + i + j] = tmp[j * decoder->channels + k];
            }
        }
    }

    drflac_close(decoder);
}

auto Wav::create_instance() -> std::shared_ptr<AudioSourceInstance>
{
    return std::make_shared<WavInstance>(this);
}

auto Wav::length_time() const -> double
{
    return base_sample_rate == 0 ? 0 : m_sample_count / base_sample_rate;
}
}; // namespace cer
