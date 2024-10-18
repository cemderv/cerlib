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
#include "audio/soloud_internal.hpp"
#include <algorithm>
#include <cassert>

namespace cer
{
BusInstance::BusInstance(Bus* aParent)
    : m_parent(aParent)
    , m_scratch_size(sample_granularity)
    , m_scratch(m_scratch_size * max_channels)
{
    flags.Protected     = true;
    flags.InaudibleTick = true;
}

size_t BusInstance::audio(float* aBuffer, size_t aSamplesToRead, size_t aBufferSize)
{
    const auto handle = m_parent->m_channel_handle;

    if (handle == 0)
    {
        // Avoid reuse of scratch data if this bus hasn't played anything yet
        for (size_t i = 0; i < aBufferSize * channel_count; ++i)
        {
            aBuffer[i] = 0;
        }

        return aSamplesToRead;
    }

    auto* s = m_parent->engine;

    s->mixBus_internal(aBuffer,
                       aSamplesToRead,
                       aBufferSize,
                       m_scratch.mData,
                       handle,
                       sample_rate,
                       channel_count,
                       m_parent->m_resampler);

    if (m_parent->visualization_data)
    {
        std::ranges::fill(m_visualization_channel_volume, 0.0f);

        if (aSamplesToRead > 255)
        {
            for (size_t i = 0; i < 256; ++i)
            {
                m_visualization_wave_data[i] = 0;

                for (size_t j = 0; j < channel_count; ++j)
                {
                    const auto sample = aBuffer[i + aBufferSize * j];

                    if (const auto absvol = fabs(sample);
                        absvol > m_visualization_channel_volume[j])
                    {
                        m_visualization_channel_volume[j] = absvol;
                    }

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
                    const float sample = aBuffer[(i % aSamplesToRead) + aBufferSize * j];

                    if (const float absvol = fabs(sample);
                        absvol > m_visualization_channel_volume[j])
                    {
                        m_visualization_channel_volume[j] = absvol;
                    }

                    m_visualization_wave_data[i] += sample;
                }
            }
        }
    }
    return aSamplesToRead;
}

bool BusInstance::has_ended()
{
    return false;
}

BusInstance::~BusInstance() noexcept
{
    auto* s = m_parent->engine;
    for (size_t i = 0; i < s->m_highest_voice; ++i)
    {
        if (s->m_voice[i] && s->m_voice[i]->bus_handle == m_parent->m_channel_handle)
        {
            s->stopVoice_internal(i);
        }
    }
}

Bus::Bus()
{
    channel_count = 2;
}

std::shared_ptr<AudioSourceInstance> Bus::create_instance()
{
    if (m_channel_handle)
    {
        stop();
        m_channel_handle = 0;
    }
    m_instance = std::make_shared<BusInstance>(this);
    return m_instance;
}

void Bus::find_bus_handle()
{
    if (m_channel_handle == 0)
    {
        // Find the channel the bus is playing on to calculate handle..
        for (size_t i = 0; m_channel_handle == 0 && i < engine->m_highest_voice; ++i)
        {
            if (engine->m_voice[i].get() == m_instance.get())
            {
                m_channel_handle = engine->getHandleFromVoice_internal(i);
            }
        }
    }
}

SoundHandle Bus::play(AudioSource& aSound, float aVolume, float aPan, bool aPaused)
{
    if (!m_instance || !engine)
    {
        return 0;
    }

    find_bus_handle();

    if (m_channel_handle == 0)
    {
        return 0;
    }
    return engine->play(aSound, aVolume, aPan, aPaused, m_channel_handle);
}


SoundHandle Bus::play_clocked(SoundTime aSoundTime, AudioSource& aSound, float aVolume, float aPan)
{
    if (!m_instance || !engine)
    {
        return 0;
    }

    find_bus_handle();

    if (m_channel_handle == 0)
    {
        return 0;
    }

    return engine->play_clocked(aSoundTime, aSound, aVolume, aPan, m_channel_handle);
}

SoundHandle Bus::play3d(
    AudioSource& aSound, Vector3 aPos, Vector3 aVel, float aVolume, bool aPaused)
{
    if (!m_instance || !engine)
    {
        return 0;
    }

    find_bus_handle();

    if (m_channel_handle == 0)
    {
        return 0;
    }
    return engine->play3d(aSound, aPos, aVel, aVolume, aPaused, m_channel_handle);
}

SoundHandle Bus::play3d_clocked(
    SoundTime aSoundTime, AudioSource& aSound, Vector3 aPos, Vector3 aVel, float aVolume)
{
    if (!m_instance || !engine)
    {
        return 0;
    }

    find_bus_handle();

    if (m_channel_handle == 0)
    {
        return 0;
    }
    return engine->play3d_clocked(aSoundTime, aSound, aPos, aVel, aVolume, m_channel_handle);
}

void Bus::annex_sound(SoundHandle voice_handle)
{
    find_bus_handle();
    FOR_ALL_VOICES_PRE_EXT
    engine->m_voice[ch]->bus_handle = m_channel_handle;
    FOR_ALL_VOICES_POST_EXT
}

void Bus::set_filter(size_t filter_id, Filter* aFilter)
{
    if (filter_id >= filters_per_stream)
        return;

    filter[filter_id] = aFilter;

    if (m_instance)
    {
        engine->lockAudioMutex_internal();
        if (aFilter)
        {
            m_instance->filter[filter_id] = filter[filter_id]->create_instance();
        }
        engine->unlockAudioMutex_internal();
    }
}

void Bus::set_channels(size_t aChannels)
{
    assert(aChannels != 0 && aChannels != 3 && aChannels != 5 && aChannels != 7);
    assert(aChannels <= max_channels);

    channel_count = aChannels;
}

void Bus::set_visualization_enable(bool aEnable)
{
    visualization_data = aEnable;
}

float* Bus::calc_fft()
{
    if (m_instance && engine)
    {
        engine->lockAudioMutex_internal();
        auto temp = std::array<float, 1024>{};
        for (int i = 0; i < 256; ++i)
        {
            temp[i * 2]     = m_instance->m_visualization_wave_data[i];
            temp[i * 2 + 1] = 0;
            temp[i + 512]   = 0;
            temp[i + 768]   = 0;
        }
        engine->unlockAudioMutex_internal();

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

float* Bus::wave()
{
    if (m_instance && engine)
    {
        engine->lockAudioMutex_internal();
        for (int i = 0; i < 256; ++i)
            m_wave_data[i] = m_instance->m_visualization_wave_data[i];
        engine->unlockAudioMutex_internal();
    }
    return m_wave_data.data();
}

float Bus::approximate_volume(size_t aChannel)
{
    if (aChannel > channel_count)
    {
        return 0;
    }
    auto vol = 0.0f;
    if (m_instance && engine)
    {
        engine->lockAudioMutex_internal();
        vol = m_instance->m_visualization_channel_volume[aChannel];
        engine->unlockAudioMutex_internal();
    }
    return vol;
}

size_t Bus::active_voice_count()
{
    size_t count = 0;
    find_bus_handle();
    engine->lockAudioMutex_internal();
    for (size_t i = 0; i < voice_count; ++i)
    {
        if (engine->m_voice[i] && engine->m_voice[i]->bus_handle == m_channel_handle)
        {
            count++;
        }
    }
    engine->unlockAudioMutex_internal();
    return count;
}

Resampler Bus::resampler() const
{
    return m_resampler;
}

void Bus::set_resampler(Resampler aResampler)
{
    m_resampler = aResampler;
}
}; // namespace cer
