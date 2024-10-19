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

#include "dr_flac.h"
#include "dr_mp3.h"
#include "dr_wav.h"

#include "audio/Common.hpp"
#include "audio/WavStream.hpp"
#include "stb_vorbis.hpp"
#include "util/MemoryReader.hpp"
#include <cstring>

#define MAKEDWORD(a, b, c, d) (((d) << 24) | ((c) << 16) | ((b) << 8) | (a))

namespace cer
{
size_t drflac_read_func(void* pUserData, void* pBufferOut, size_t bytesToRead)
{
    auto* fp = static_cast<MemoryReader*>(pUserData);
    return fp->read(static_cast<unsigned char*>(pBufferOut), (size_t)bytesToRead);
}

size_t drmp3_read_func(void* pUserData, void* pBufferOut, size_t bytesToRead)
{
    auto* fp = static_cast<MemoryReader*>(pUserData);
    return fp->read(static_cast<unsigned char*>(pBufferOut), (size_t)bytesToRead);
}

size_t drwav_read_func(void* pUserData, void* pBufferOut, size_t bytesToRead)
{
    auto* fp = static_cast<MemoryReader*>(pUserData);
    return fp->read(static_cast<unsigned char*>(pBufferOut), (size_t)bytesToRead);
}

drflac_bool32 drflac_seek_func(void* pUserData, int offset, drflac_seek_origin origin)
{
    auto* fp = static_cast<MemoryReader*>(pUserData);
    if (origin != drflac_seek_origin_start)
        offset += int(fp->pos());
    fp->seek(offset);
    return 1;
}

drmp3_bool32 drmp3_seek_func(void* pUserData, int offset, drmp3_seek_origin origin)
{
    MemoryReader* fp = (MemoryReader*)pUserData;
    if (origin != drmp3_seek_origin_start)
        offset += int(fp->pos());
    fp->seek(offset);
    return 1;
}

drmp3_bool32 drwav_seek_func(void* pUserData, int offset, drwav_seek_origin origin)
{
    MemoryReader* fp = (MemoryReader*)pUserData;
    if (origin != drwav_seek_origin_start)
        offset += int(fp->pos());
    fp->seek(offset);
    return 1;
}

WavStreamInstance::WavStreamInstance(WavStream* aParent)
    : mParent(aParent)
{
    mFile = mParent->mFile;

    if (mParent->mIsStream)
    {
        mFile.seek(0); // stb_vorbis assumes file offset to be at start of ogg
    }

    // if (mFile)
    {
        if (mParent->mFiletype == WAVSTREAM_WAV)
        {
            auto& wav = std::get<drwav*>(mCodec);
            wav       = new drwav();
            if (!drwav_init(wav, drwav_read_func, drwav_seek_func, &mFile, nullptr))
            {
                delete wav;
                throw std::runtime_error{"Failed to create instance"};
            }
        }
        else if (mParent->mFiletype == WAVSTREAM_OGG)
        {
            auto& ogg = std::get<stb_vorbis*>(mCodec);

            int e = 0;
            ogg   = stb_vorbis_open_memory(mFile.data_uc(), int(mFile.size()), &e, nullptr);

            if (!ogg)
            {
                throw std::runtime_error{"Failed to create instance"};
            }

            mOggFrameSize   = 0;
            mOggFrameOffset = 0;
            mOggOutputs     = nullptr;
        }
        else if (mParent->mFiletype == WAVSTREAM_FLAC)
        {
            auto& flac = std::get<drflac*>(mCodec);
            flac       = drflac_open(drflac_read_func, drflac_seek_func, &mFile, nullptr);

            if (!flac)
            {
                throw std::runtime_error{"Failed to create instance"};
            }
        }
        else if (mParent->mFiletype == WAVSTREAM_MP3)
        {
            auto& mp3 = std::get<drmp3*>(mCodec);

            mp3 = new drmp3();

            if (!drmp3_init(mp3, drmp3_read_func, drmp3_seek_func, &mFile, nullptr))
            {
                delete mp3;
                throw std::runtime_error{"Failed to create instance"};
            }
        }
        else
        {
            throw std::runtime_error{"Failed to create instance"};
        }
    }
}

WavStreamInstance::~WavStreamInstance()
{
    switch (mParent->mFiletype)
    {
        case WAVSTREAM_OGG: {
            if (auto** ogg = std::get_if<stb_vorbis*>(&mCodec))
            {
                stb_vorbis_close(*ogg);
                *ogg = nullptr;
            }
            break;
        }
        case WAVSTREAM_FLAC: {
            if (auto** flac = std::get_if<drflac*>(&mCodec))
            {
                drflac_close(*flac);
                *flac = nullptr;
            }
            break;
        }
        case WAVSTREAM_MP3: {
            if (auto** mp3 = std::get_if<drmp3*>(&mCodec))
            {
                drmp3_uninit(*mp3);
                delete *mp3;
                *mp3 = nullptr;
            }
            break;
        }
        case WAVSTREAM_WAV: {
            if (auto** wav = std::get_if<drwav*>(&mCodec))
            {
                drwav_uninit(*wav);
                delete *wav;
                *wav = nullptr;
            }
            break;
        }
        default: break;
    }
}

static int getOggData(float** aOggOutputs,
                      float*  aBuffer,
                      int     aSamples,
                      int     aPitch,
                      int     aFrameSize,
                      int     aFrameOffset,
                      int     aChannels)
{
    if (aFrameSize <= 0)
    {
        return 0;
    }

    int samples = aSamples;
    if (aFrameSize - aFrameOffset < samples)
    {
        samples = aFrameSize - aFrameOffset;
    }

    for (int i = 0; i < aChannels; ++i)
    {
        memcpy(aBuffer + aPitch * i, aOggOutputs[i] + aFrameOffset, sizeof(float) * samples);
    }

    return samples;
}


size_t WavStreamInstance::audio(float* aBuffer, size_t aSamplesToRead, size_t aBufferSize)
{
    size_t                                offset = 0;
    std::array<float, 512 * max_channels> tmp{};

#if 0
    if (mFile == nullptr)
        return 0;
#endif

    switch (mParent->mFiletype)
    {
        case WAVSTREAM_FLAC: {
            auto* flac = std::get<drflac*>(mCodec);

            for (size_t i = 0; i < aSamplesToRead; i += 512)
            {
                size_t blockSize = (aSamplesToRead - i) > 512 ? 512 : aSamplesToRead - i;
                offset += drflac_read_pcm_frames_f32(flac, blockSize, tmp.data());

                for (size_t j = 0; j < blockSize; ++j)
                {
                    for (size_t k = 0; k < channel_count; k++)
                    {
                        aBuffer[k * aSamplesToRead + i + j] = tmp[j * flac->channels + k];
                    }
                }
            }

            mOffset += offset;

            return offset;
        }
        case WAVSTREAM_MP3: {
            auto* mp3 = std::get<drmp3*>(mCodec);

            for (size_t i = 0; i < aSamplesToRead; i += 512)
            {
                size_t blockSize = (aSamplesToRead - i) > 512 ? 512 : aSamplesToRead - i;
                offset += (size_t)drmp3_read_pcm_frames_f32(mp3, blockSize, tmp.data());

                for (size_t j = 0; j < blockSize; ++j)
                {
                    for (size_t k = 0; k < channel_count; k++)
                    {
                        aBuffer[k * aSamplesToRead + i + j] = tmp[j * mp3->channels + k];
                    }
                }
            }
            mOffset += offset;
            return offset;
        }
        case WAVSTREAM_OGG: {
            if (mOggFrameOffset < mOggFrameSize)
            {
                const int b = getOggData(mOggOutputs,
                                         aBuffer,
                                         int(aSamplesToRead),
                                         int(aBufferSize),
                                         int(mOggFrameSize),
                                         int(mOggFrameOffset),
                                         int(channel_count));
                mOffset += b;
                offset += b;
                mOggFrameOffset += b;
            }

            auto* ogg = std::get<stb_vorbis*>(mCodec);

            while (offset < aSamplesToRead)
            {
                mOggFrameSize   = stb_vorbis_get_frame_float(ogg, nullptr, &mOggOutputs);
                mOggFrameOffset = 0;

                const int b = getOggData(mOggOutputs,
                                         aBuffer + offset,
                                         int(aSamplesToRead - offset),
                                         int(aBufferSize),
                                         int(mOggFrameSize),
                                         int(mOggFrameOffset),
                                         int(channel_count));

                mOffset += b;
                offset += b;
                mOggFrameOffset += b;

                if (mOffset >= mParent->mSampleCount || b == 0)
                {
                    mOffset += offset;
                    return offset;
                }
            }
        }
        break;
        case WAVSTREAM_WAV: {
            auto* wav = std::get<drwav*>(mCodec);

            for (size_t i = 0; i < aSamplesToRead; i += 512)
            {
                size_t blockSize = (aSamplesToRead - i) > 512 ? 512 : aSamplesToRead - i;
                offset += drwav_read_pcm_frames_f32(wav, blockSize, tmp.data());

                for (size_t j = 0; j < blockSize; ++j)
                {
                    for (size_t k = 0; k < channel_count; k++)
                    {
                        aBuffer[k * aSamplesToRead + i + j] = tmp[j * wav->channels + k];
                    }
                }
            }
            mOffset += offset;
            return offset;
        }
    }
    return aSamplesToRead;
}

bool WavStreamInstance::seek(double aSeconds, float* mScratch, size_t mScratchSize)
{
    if (auto** ogg = std::get_if<stb_vorbis*>(&mCodec))
    {
        const auto pos = int(floor(base_sample_rate * aSeconds));
        stb_vorbis_seek(*ogg, pos);
        // Since the position that we just sought to might not be *exactly*
        // the position we asked for, we're re-calculating the position just
        // for the sake of correctness.
        mOffset            = stb_vorbis_get_sample_offset(*ogg);
        double newPosition = float(mOffset / base_sample_rate);
        stream_position    = newPosition;

        return false;
    }

    return AudioSourceInstance::seek(aSeconds, mScratch, mScratchSize);
}

bool WavStreamInstance::rewind()
{
    switch (mParent->mFiletype)
    {
        case WAVSTREAM_OGG:
            if (auto** ogg = std::get_if<stb_vorbis*>(&mCodec))
            {
                stb_vorbis_seek_start(*ogg);
            }
            break;
        case WAVSTREAM_FLAC:
            if (auto** flac = std::get_if<drflac*>(&mCodec))
            {
                drflac_seek_to_pcm_frame(*flac, 0);
            }
            break;
        case WAVSTREAM_MP3:
            if (auto** mp3 = std::get_if<drmp3*>(&mCodec))
            {
                drmp3_seek_to_pcm_frame(*mp3, 0);
            }
            break;
        case WAVSTREAM_WAV:
            if (auto** wav = std::get_if<drwav*>(&mCodec))
            {
                drwav_seek_to_pcm_frame(*wav, 0);
            }
            break;
        default: break;
    }

    mOffset         = 0;
    stream_position = 0.0f;

    return false;
}

bool WavStreamInstance::has_ended()
{
    assert(mParent != nullptr);

    return mOffset >= mParent->mSampleCount;
}

WavStream::WavStream(std::span<const std::byte> data)
    : mFile(data)
{
    switch (mFile.read_u32())
    {
        case MAKEDWORD('O', 'g', 'g', 'S'): loadogg(mFile); break;
        case MAKEDWORD('R', 'I', 'F', 'F'): loadwav(mFile); break;
        case MAKEDWORD('f', 'L', 'a', 'C'): loadflac(mFile); break;
        default: loadmp3(mFile); break;
    }
}

WavStream::~WavStream()
{
    stop();
}

void WavStream::loadwav(MemoryReader& fp)
{
    fp.seek(0);
    drwav decoder;

    if (!drwav_init(&decoder, drwav_read_func, drwav_seek_func, &fp, nullptr))
    {
        throw std::runtime_error{"Failed to load WAV file"};
    }

    channel_count = decoder.channels;
    if (channel_count > max_channels)
    {
        channel_count = max_channels;
    }

    base_sample_rate = float(decoder.sampleRate);
    mSampleCount     = size_t(decoder.totalPCMFrameCount);
    mFiletype        = WAVSTREAM_WAV;
    drwav_uninit(&decoder);
}

void WavStream::loadogg(MemoryReader& fp)
{
    fp.seek(0);

    int         e = 0;
    stb_vorbis* v = stb_vorbis_open_memory(fp.data_uc(), 0, &e, nullptr);

    if (v == nullptr)
    {
        throw std::runtime_error{"Failed to load OGG file"};
    }

    const auto info = stb_vorbis_get_info(v);
    channel_count   = info.channels;
    if (size_t(info.channels) > max_channels)
    {
        channel_count = max_channels;
    }
    base_sample_rate = (float)info.sample_rate;
    int samples      = stb_vorbis_stream_length_in_samples(v);
    stb_vorbis_close(v);
    mFiletype = WAVSTREAM_OGG;

    mSampleCount = samples;
}

void WavStream::loadflac(MemoryReader& fp)
{
    fp.seek(0);
    drflac* decoder = drflac_open(drflac_read_func, drflac_seek_func, &fp, nullptr);

    if (decoder == nullptr)
    {
        throw std::runtime_error{"Failed to load FLAC file"};
    }

    channel_count = decoder->channels;
    if (channel_count > max_channels)
    {
        channel_count = max_channels;
    }

    base_sample_rate = float(decoder->sampleRate);
    mSampleCount     = size_t(decoder->totalPCMFrameCount);
    mFiletype        = WAVSTREAM_FLAC;
    drflac_close(decoder);
}

void WavStream::loadmp3(MemoryReader& fp)
{
    fp.seek(0);

    drmp3 decoder;
    if (!drmp3_init(&decoder, drmp3_read_func, drmp3_seek_func, &fp, nullptr))
    {
        throw std::runtime_error{"Failed to load MP3 file"};
    }

    channel_count = decoder.channels;
    if (channel_count > max_channels)
    {
        channel_count = max_channels;
    }

    const drmp3_uint64 samples = drmp3_get_pcm_frame_count(&decoder);

    base_sample_rate = float(decoder.sampleRate);
    mSampleCount     = size_t(samples);
    mFiletype        = WAVSTREAM_MP3;
    drmp3_uninit(&decoder);
}

std::shared_ptr<AudioSourceInstance> WavStream::create_instance()
{
    return std::make_shared<WavStreamInstance>(this);
}

double WavStream::getLength() const
{
    return base_sample_rate == 0 ? 0 : mSampleCount / base_sample_rate;
}
}; // namespace cer
