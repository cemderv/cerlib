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

#include "audio/Bus.hpp"
#include "audio/AudioDevice.hpp"
#include "audio/FFT.hpp"
#include <algorithm>
#include <cassert>

namespace cer
{
BusInstance::BusInstance(Bus* parent)
    : m_parent(parent)
    , m_scratch_size(sample_granularity)
    , m_scratch(m_scratch_size * max_channels)
{
    flags.is_protected   = true;
    flags.inaudible_tick = true;
}

auto BusInstance::audio(float* buffer, size_t samples_to_read, size_t buffer_size) -> size_t
{
    const auto handle = m_parent->m_channel_handle;

    if (handle == 0)
    {
        // Avoid reuse of scratch data if this bus hasn't played anything yet
        for (size_t i = 0; i < buffer_size * channel_count; ++i)
        {
            buffer[i] = 0;
        }

        return samples_to_read;
    }

    auto* s = m_parent->engine;

    s->mix_bus_internal(buffer,
                        samples_to_read,
                        buffer_size,
                        m_scratch.data(),
                        handle,
                        sample_rate,
                        channel_count,
                        m_parent->m_resampler);

    if (m_parent->visualization_data)
    {
        std::ranges::fill(m_visualization_channel_volume, 0.0f);

        if (samples_to_read > 255)
        {
            for (size_t i = 0; i < 256; ++i)
            {
                m_visualization_wave_data[i] = 0;

                for (size_t j = 0; j < channel_count; ++j)
                {
                    const auto sample = buffer[i + buffer_size * j];

                    m_visualization_channel_volume[j] =
                        max(std::abs(sample), m_visualization_channel_volume[j]);

                    m_visualization_wave_data[i] += sample;
                }
            }
        }
        else
        {
            // Very unlikely failsafe branch
            for (size_t i = 0; i < 256; ++i)
            {
                m_visualization_wave_data[i] = 0;

                for (size_t j = 0; j < channel_count; ++j)
                {
                    const float sample = buffer[(i % samples_to_read) + (buffer_size * j)];

                    m_visualization_channel_volume[j] =
                        max(std::abs(sample), m_visualization_channel_volume[j]);

                    m_visualization_wave_data[i] += sample;
                }
            }
        }
    }
    return samples_to_read;
}

auto BusInstance::has_ended() -> bool
{
    return false;
}

BusInstance::~BusInstance() noexcept
{
    auto&      s             = *m_parent->engine;
    const auto highest_voice = s.highest_voice();
    const auto voices        = s.voices();

    for (size_t i = 0; i < highest_voice; ++i)
    {
        if (voices[i] && voices[i]->bus_handle == m_parent->m_channel_handle)
        {
            s.stop_voice_internal(i);
        }
    }
}

Bus::Bus()
{
    channel_count = 2;
}

auto Bus::create_instance() -> std::shared_ptr<AudioSourceInstance>
{
    if (m_channel_handle != 0u)
    {
        stop();
        m_channel_handle = 0;
    }

    m_instance = std::make_shared<BusInstance>(this);
    return m_instance;
}

void Bus::find_bus_handle()
{
    if (m_channel_handle != 0)
    {
        return;
    }

    const auto highest_voice = engine->highest_voice();
    const auto voices        = engine->voices();

    // Find the channel the bus is playing on to calculate handle..
    for (size_t i = 0; m_channel_handle == 0 && i < highest_voice; ++i)
    {
        if (voices[i].get() == m_instance.get())
        {
            m_channel_handle = engine->get_handle_from_voice_internal(i);
        }
    }
}

auto Bus::play(AudioSource& sound, float volume, float pan, bool paused) -> SoundHandle
{
    if (!m_instance || engine == nullptr)
    {
        return 0;
    }

    find_bus_handle();

    if (m_channel_handle == 0)
    {
        return 0;
    }

    return engine->play(sound, volume, pan, paused, m_channel_handle);
}


auto Bus::play_clocked(SoundTime time, AudioSource& sound, float volume, float pan) -> SoundHandle
{
    if (!m_instance || engine == nullptr)
    {
        return 0;
    }

    find_bus_handle();

    if (m_channel_handle == 0)
    {
        return 0;
    }

    return engine->play_clocked(time, sound, volume, pan, m_channel_handle);
}

auto Bus::play3d(AudioSource& sound, Vector3 pos, Vector3 vel, float volume, bool paused)
    -> SoundHandle
{
    if (!m_instance || engine == nullptr)
    {
        return 0;
    }

    find_bus_handle();

    if (m_channel_handle == 0)
    {
        return 0;
    }

    return engine->play_3d(sound, pos, vel, volume, paused, m_channel_handle);
}

auto Bus::play3d_clocked(SoundTime time, AudioSource& sound, Vector3 pos, Vector3 vel, float volume)
    -> SoundHandle
{
    if (!m_instance || engine == nullptr)
    {
        return 0;
    }

    find_bus_handle();

    if (m_channel_handle == 0)
    {
        return 0;
    }

    return engine->play3d_clocked(time, sound, pos, vel, volume, m_channel_handle);
}

void Bus::annex_sound(SoundHandle voice_handle)
{
    find_bus_handle();

    engine->foreach_voice(voice_handle, [this](int ch) {
        engine->voices()[ch]->bus_handle = m_channel_handle;
    });
}

void Bus::set_filter(size_t filter_id, Filter* filter)
{
    if (filter_id >= filters_per_stream)
    {
        return;
    }

    this->filter[filter_id] = filter;

    if (m_instance != nullptr)
    {
        engine->lock_audio_mutex_internal();
        if (filter != nullptr)
        {
            m_instance->filter[filter_id] = this->filter[filter_id]->create_instance();
        }
        engine->unlock_audio_mutex_internal();
    }
}

void Bus::set_channels(size_t channels)
{
    assert(channels != 0 && channels != 3 && channels != 5 && channels != 7);
    assert(channels <= max_channels);

    channel_count = channels;
}

void Bus::set_visualization_enable(bool enable)
{
    visualization_data = enable;
}

auto Bus::calc_fft() -> float*
{
    if (m_instance && engine != nullptr)
    {
        engine->lock_audio_mutex_internal();
        auto temp = std::array<float, 1024>{};
        for (int i = 0; i < 256; ++i)
        {
            temp[i * 2]     = m_instance->m_visualization_wave_data[i];
            temp[i * 2 + 1] = 0;
            temp[i + 512]   = 0;
            temp[i + 768]   = 0;
        }
        engine->unlock_audio_mutex_internal();

        FFT::fft1024(temp.data());

        for (int i = 0; i < 256; ++i)
        {
            const float real = temp[i * 2];
            const float imag = temp[i * 2 + 1];
            m_fft_data[i]    = sqrt(real * real + imag * imag);
        }
    }

    return m_fft_data.data();
}

auto Bus::wave() -> float*
{
    if (m_instance && engine != nullptr)
    {
        engine->lock_audio_mutex_internal();
        for (int i = 0; i < 256; ++i)
        {
            m_wave_data[i] = m_instance->m_visualization_wave_data[i];
        }
        engine->unlock_audio_mutex_internal();
    }
    return m_wave_data.data();
}

auto Bus::approximate_volume(size_t channel) -> float
{
    if (channel > channel_count)
    {
        return 0;
    }

    auto vol = 0.0f;

    if (m_instance && engine != nullptr)
    {
        engine->lock_audio_mutex_internal();
        vol = m_instance->m_visualization_channel_volume[channel];
        engine->unlock_audio_mutex_internal();
    }

    return vol;
}

auto Bus::active_voice_count() -> size_t
{
    size_t count = 0;
    find_bus_handle();
    engine->lock_audio_mutex_internal();

    const auto voices = engine->voices();

    for (size_t i = 0; i < max_voice_count; ++i)
    {
        if (voices[i] != nullptr && voices[i]->bus_handle == m_channel_handle)
        {
            count++;
        }
    }

    engine->unlock_audio_mutex_internal();

    return count;
}

auto Bus::resampler() const -> Resampler
{
    return m_resampler;
}

void Bus::set_resampler(Resampler resampler)
{
    m_resampler = resampler;
}
}; // namespace cer
