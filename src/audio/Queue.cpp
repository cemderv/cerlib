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

#include "audio/Queue.hpp"
#include "audio/AudioDevice.hpp"

namespace cer
{
QueueInstance::QueueInstance(Queue* parent)
    : m_parent(parent)
{
    flags.Protected = true;
}

auto QueueInstance::audio(float* buffer, size_t samples_to_read, size_t buffer_size) -> size_t
{
    if (m_parent->m_count == 0)
    {
        return 0;
    }

    size_t copy_count  = samples_to_read;
    size_t copy_offset = 0;

    while (copy_count && m_parent->m_count)
    {
        const auto read_count =
            m_parent->m_source[m_parent->m_read_index]->audio(buffer + copy_offset,
                                                              copy_count,
                                                              buffer_size);
        copy_offset += read_count;
        copy_count -= read_count;

        if (m_parent->m_source[m_parent->m_read_index]->has_ended())
        {
            m_parent->m_source[m_parent->m_read_index].reset();
            m_parent->m_read_index = (m_parent->m_read_index + 1) % Queue::queue_max;
            m_parent->m_count--;
            loop_count++;
        }
    }

    return copy_offset;
}

auto QueueInstance::has_ended() -> bool
{
    return loop_count != 0 && m_parent->m_count == 0;
}

auto Queue::create_instance() -> std::shared_ptr<AudioSourceInstance>
{
    if (m_instance)
    {
        stop();
    }

    m_instance = std::make_shared<QueueInstance>(this);
    return m_instance;
}

void Queue::find_queue_handle()
{
    // Find the channel the queue is playing on to calculate handle..
    for (size_t i = 0; m_queue_handle == 0 && i < engine->m_highest_voice; ++i)
    {
        if (engine->m_voice[i] == m_instance)
        {
            m_queue_handle = engine->getHandleFromVoice_internal(i);
        }
    }
}

void Queue::play(AudioSource& sound)
{
    assert(engine != nullptr);

    find_queue_handle();

    assert(m_queue_handle != 0);
    assert(m_count < queue_max);

    if (!sound.audio_source_id)
    {
        sound.audio_source_id = engine->m_audio_source_id;
        engine->m_audio_source_id++;
    }

    auto instance = sound.create_instance();

    instance->init(sound, 0);
    instance->audio_source_id = sound.audio_source_id;

    engine->lockAudioMutex_internal();
    m_source[m_write_index] = std::move(instance);
    m_write_index           = (m_write_index + 1) % queue_max;
    m_count++;
    engine->unlockAudioMutex_internal();
}


auto Queue::queue_count() const -> size_t
{
    if (engine == nullptr)
    {
        return 0;
    }

    engine->lockAudioMutex_internal();
    const auto count = m_count;
    engine->unlockAudioMutex_internal();

    return count;
}

auto Queue::is_currently_playing(const AudioSource& sound) const -> bool
{
    if (engine == nullptr || m_count == 0 || sound.audio_source_id == 0)
    {
        return false;
    }

    engine->lockAudioMutex_internal();
    const auto res = m_source[m_read_index]->audio_source_id == sound.audio_source_id;
    engine->unlockAudioMutex_internal();

    return res;
}

void Queue::set_params_from_audio_source(const AudioSource& sound)
{
    channel_count    = sound.channel_count;
    base_sample_rate = sound.base_sample_rate;
}

void Queue::set_params(float sample_rate, size_t channel_count)
{
    assert(channel_count >= 1);
    assert(channel_count <= max_channels);

    this->channel_count = channel_count;
    base_sample_rate    = sample_rate;
}
} // namespace cer
