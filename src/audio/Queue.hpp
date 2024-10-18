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

namespace cer
{
class Queue;

class QueueInstance final : public AudioSourceInstance
{
  public:
    explicit QueueInstance(Queue* parent);

    auto audio(float* buffer, size_t samples_to_read, size_t buffer_size) -> size_t override;

    auto has_ended() -> bool override;

  private:
    Queue* m_parent = nullptr;
};

class Queue final : public AudioSource
{
    friend QueueInstance;

  public:
    static constexpr auto queue_max = size_t(32);

    auto create_instance() -> std::shared_ptr<AudioSourceInstance> override;

    // Play sound through the queue
    void play(AudioSource& sound);

    // Number of audio sources queued for replay
    auto queue_count() const -> size_t;

    // Is this audio source currently playing?
    auto is_currently_playing(const AudioSource& sound) const -> bool;

    // Set params by reading them from an audio source
    void set_params_from_audio_source(const AudioSource& sound);

    // Set params manually
    void set_params(float sample_rate, size_t channel_count = 2);

    void find_queue_handle();

  private:
    size_t                                                      m_read_index  = 0;
    size_t                                                      m_write_index = 0;
    size_t                                                      m_count       = 0;
    std::array<std::shared_ptr<AudioSourceInstance>, queue_max> m_source{};
    std::shared_ptr<QueueInstance>                              m_instance;
    SoundHandle                                                 m_queue_handle = 0;
};
}; // namespace cer
