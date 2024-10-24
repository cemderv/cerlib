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
#include <span>

struct stb_vorbis;

namespace cer
{
class Wav;
class MemoryReader;

class WavInstance final : public AudioSourceInstance
{
  public:
    explicit WavInstance(Wav* aParent);

    auto audio(float* buffer, size_t samples_to_read, size_t buffer_size) -> size_t override;

    auto rewind() -> bool override;

    auto has_ended() -> bool override;

  private:
    Wav*   m_parent = nullptr;
    size_t m_offset = 0;
};

class Wav final : public AudioSource
{
    friend WavInstance;

  public:
    explicit Wav(std::span<const std::byte> data);

    ~Wav() override;

    auto create_instance() -> std::shared_ptr<AudioSourceInstance> override;

    auto length_time() const -> SoundTime;

  private:
    void load_wav(const MemoryReader& reader);

    void load_ogg(const MemoryReader& reader);

    void load_mp3(const MemoryReader& reader);

    void load_flac(const MemoryReader& reader);

    std::unique_ptr<float[]> m_data;
    size_t                   m_sample_count = 0;
};
}; // namespace cer
