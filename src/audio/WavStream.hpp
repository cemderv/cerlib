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

#pragma once

#include "audio/AudioSource.hpp"
#include "util/MemoryReader.hpp"
#include <optional>
#include <variant>

struct stb_vorbis;
#ifndef dr_flac_h
struct drflac;
#endif
#ifndef dr_mp3_h
struct drmp3;
#endif
#ifndef dr_wav_h
struct drwav;
#endif

namespace cer
{
class WavStream;

class WavStreamInstance final : public AudioSourceInstance
{
    WavStream*   mParent = nullptr;
    size_t       mOffset = 0;
    MemoryReader mFile;

    std::variant<stb_vorbis*, drflac*, drmp3*, drwav*> mCodec;

    size_t  mOggFrameSize   = 0;
    size_t  mOggFrameOffset = 0;
    float** mOggOutputs     = nullptr;

  public:
    explicit WavStreamInstance(WavStream* aParent);
    size_t audio(float* aBuffer, size_t aSamplesToRead, size_t aBufferSize) override;
    bool   seek(double aSeconds, float* mScratch, size_t mScratchSize) override;
    bool   rewind() override;
    bool   has_ended() override;
    ~WavStreamInstance() override;
};

enum WAVSTREAM_FILETYPE
{
    WAVSTREAM_WAV  = 0,
    WAVSTREAM_OGG  = 1,
    WAVSTREAM_FLAC = 2,
    WAVSTREAM_MP3  = 3
};

class WavStream final : public AudioSource
{
    friend WavStreamInstance;

  public:
    int          mFiletype = WAVSTREAM_WAV;
    MemoryReader mFile;
    bool         mIsStream    = false;
    size_t       mSampleCount = 0;

    explicit WavStream(std::span<const std::byte> data);

    ~WavStream() override;

    std::shared_ptr<AudioSourceInstance> create_instance() override;
    SoundTime                            getLength() const;

  private:
    void loadwav(MemoryReader& fp);
    void loadogg(MemoryReader& fp);
    void loadflac(MemoryReader& fp);
    void loadmp3(MemoryReader& fp);
};
}; // namespace cer
