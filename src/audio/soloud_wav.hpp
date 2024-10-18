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

#include "audio/soloud_audiosource.hpp"
#include <span>

struct stb_vorbis;

namespace cer
{
class Wav;
class MemoryFile;

class WavInstance final : public AudioSourceInstance
{
  public:
    explicit WavInstance(Wav* aParent);

    size_t getAudio(float* aBuffer, size_t aSamplesToRead, size_t aBufferSize) override;

    bool rewind() override;

    bool hasEnded() override;

  private:
    Wav*   mParent = nullptr;
    size_t mOffset = 0;
};

class Wav final : public AudioSource
{
    friend WavInstance;

  public:
    explicit Wav(std::span<const std::byte> data);

    ~Wav() override;

    std::shared_ptr<AudioSourceInstance> createInstance() override;

    time_t getLength() const;

  private:
    void loadwav(const MemoryFile& aReader);
    void loadogg(const MemoryFile& aReader);
    void loadmp3(const MemoryFile& aReader);
    void loadflac(const MemoryFile& aReader);

    std::unique_ptr<float[]> mData;
    size_t                   mSampleCount = 0;
};
}; // namespace cer
