// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "AudioDevice.hpp"

#include "SoundChannelImpl.hpp"
#include "SoundImpl.hpp"
#include "audio/FFT.hpp"
#include "audio/Misc.hpp"
#include "audio/Thread.hpp"
#include "cerlib/Logging.hpp"
#include "cerlib/SoundChannel.hpp"
#include "soloud_internal.hpp"
#include <algorithm>
#include <cfloat> // _controlfp
#include <cmath> // sin
#include <cstring>


#ifdef SOLOUD_SSE_INTRINSICS
#include <xmmintrin.h>
#ifdef _M_IX86
#include <emmintrin.h>
#endif
#endif

namespace cer
{
AudioDevice::AudioDevice(EngineFlags flags, size_t sample_rate, size_t buffer_size, size_t channels)
    : m_flags(flags)
{
    assert(channels != 3 && channels != 5 && channels != 7);
    assert(channels <= max_channels);

    m_audio_thread_mutex = thread::create_mutex();

    const auto args = AudioBackendArgs{
        .device        = this,
        .flags         = flags,
        .sample_rate   = sample_rate,
        .buffer        = buffer_size,
        .channel_count = channels,
    };

#if defined(CERLIB_AUDIO_BACKEND_SDL2)
    audio_sdl2_init(args);
#elif defined(CERLIB_AUDIO_BACKEND_SDL3)
    audio_sdl3_init(args);
#endif
}

AudioDevice::~AudioDevice() noexcept
{
    log_verbose("Destroying AudioDevice");

    if (m_was_initialized_successfully)
    {
        m_playing_sounds.clear();
    }

    // let's stop all sounds before deinit, so we don't mess up our mutexes
    stop_all();

    // Make sure no audio operation is currently pending
    lock_audio_mutex_internal();
    unlock_audio_mutex_internal();
    assert(!m_inside_audio_thread_mutex);
    stop_all();

    if (m_backend_cleanup_func != nullptr)
    {
        m_backend_cleanup_func(this);
    }

    m_backend_cleanup_func = nullptr;

    if (m_audio_thread_mutex != nullptr)
    {
        thread::destroy_mutex(m_audio_thread_mutex);
    }

    m_audio_thread_mutex = nullptr;

    for (size_t i = 0; i < filters_per_stream; ++i)
    {
        m_filter_instance[i].reset();
    }
}

auto AudioDevice::play_sound(const Sound&      sound,
                             float             volume,
                             float             pan,
                             bool              start_paused,
                             Option<SoundTime> delay) -> SoundChannel
{
    if (!sound)
    {
        return {};
    }

    const auto channel_handle =
        delay ? play_clocked(*delay, sound.impl()->audio_source(), volume, pan)
              : play(sound.impl()->audio_source(), volume, pan, start_paused);

    // TODO: Use pool allocation for SoundChannelImpl objects
    auto channel_impl = std::make_unique<details::SoundChannelImpl>(*this, channel_handle);

    m_playing_sounds.insert(sound);

    return SoundChannel{channel_impl.release()};
}

void AudioDevice::play_sound_fire_and_forget(const Sound&      sound,
                                             float             volume,
                                             float             pan,
                                             Option<SoundTime> delay)
{
    if (!sound)
    {
        return;
    }

    if (delay.has_value())
    {
        play_clocked(*delay, sound.impl()->audio_source(), volume, pan);
    }
    else
    {
        play(sound.impl()->audio_source(), volume, pan, false);
    }

    m_playing_sounds.insert(sound);
}

auto AudioDevice::play_sound_in_background(const Sound& sound, float volume, bool start_paused)
    -> SoundChannel
{
    if (!sound)
    {
        return {};
    }

    auto channel = play_sound(sound, volume, 0.0f, start_paused, std::nullopt);

    set_pan_absolute(channel.id(), 1.0f, 1.0f);

    m_playing_sounds.insert(sound);

    return channel;
}

void AudioDevice::stop_all_sounds()
{
    stop_all();
}

void AudioDevice::pause_all_sounds()
{
    set_pause_all(true);
}

void AudioDevice::resume_all_sounds()
{
    set_pause_all(false);
}

void AudioDevice::purge_sounds()
{
    std::erase_if(m_playing_sounds, [this](const Sound& sound) {
        return count_audio_source(sound.impl()->audio_source()) == 0;
    });
}

auto AudioDevice::SoundHash::operator()(const Sound& sound) const -> size_t
{
    return reinterpret_cast<size_t>(sound.impl());
}

auto AudioDevice::play(AudioSource& sound, float volume, float pan, bool paused, size_t bus)
    -> SoundHandle
{
    if (sound.single_instance)
    {
        // Only one instance allowed, stop others
        sound.stop();
    }

    // Creation of an audio instance may take significant amount of time,
    // so let's not do it inside the audio thread mutex.
    sound.engine = this;

    const auto instance = sound.create_instance();

    lock_audio_mutex_internal();

    const auto ch = find_free_voice_internal();

    if (ch == size_t(-1))
    {
        unlock_audio_mutex_internal();
        return 7; // TODO: this was "UNKNOWN_ERROR"
    }

    if (sound.audio_source_id == 0u)
    {
        sound.audio_source_id = m_audio_source_id;
        m_audio_source_id++;
    }

    m_voice[ch]                  = instance;
    m_voice[ch]->audio_source_id = sound.audio_source_id;
    m_voice[ch]->bus_handle      = bus;
    m_voice[ch]->init(sound, m_play_index);

    m_3d_data[ch] = AudioSourceInstance3dData{sound};

    m_play_index++;

    // 20 bits, skip the last one (top bits full = voice group)
    if (m_play_index == 0xfffff)
    {
        m_play_index = 0;
    }

    if (paused)
    {
        m_voice[ch]->flags.is_paused = true;
    }

    set_voice_pan_internal(ch, pan);
    if (volume < 0)
    {
        set_voice_volume_internal(ch, sound.volume);
    }
    else
    {
        set_voice_volume_internal(ch, volume);
    }

    // Fix initial voice volume ramp up
    for (size_t i = 0; i < max_channels; ++i)
    {
        m_voice[ch]->current_channel_volume[i] =
            m_voice[ch]->channel_volume[i] * m_voice[ch]->overall_volume;
    }

    set_voice_relative_play_speed_internal(ch, 1);

    for (size_t i = 0; i < filters_per_stream; ++i)
    {
        if (sound.filter[i] != nullptr)
        {
            m_voice[ch]->filter[i] = sound.filter[i]->create_instance();
        }
    }

    m_active_voice_dirty = true;

    unlock_audio_mutex_internal();

    return get_handle_from_voice_internal(ch);
}

auto AudioDevice::play_clocked(
    SoundTime sound_time, AudioSource& sound, float volume, float pan, size_t bus) -> SoundHandle
{
    const SoundHandle h = play(sound, volume, pan, true, bus);
    lock_audio_mutex_internal();
    // mLastClockedTime is cleared to zero at start of every output buffer
    SoundTime last_time = m_last_clocked_time;
    if (last_time == 0)
    {
        m_last_clocked_time = sound_time;
        last_time           = sound_time;
    }
    unlock_audio_mutex_internal();
    int samples = int(floor((sound_time - last_time) * m_sample_rate));
    // Make sure we don't delay too much (or overflow)
    if (samples < 0 || samples > 2048)
    {
        samples = 0;
    }
    set_delay_samples(h, samples);
    set_pause(h, false);
    return h;
}

auto AudioDevice::play3d_background(AudioSource& sound, float volume, bool paused, size_t bus)
    -> SoundHandle
{
    const SoundHandle h = play(sound, volume, 0.0f, paused, bus);
    set_pan_absolute(h, 1.0f, 1.0f);
    return h;
}

auto AudioDevice::seek(SoundHandle voice_handle, SoundTime seconds) -> bool
{
    auto res = true;

    foreach_voice(voice_handle, [this, &res, seconds](int ch) {
        if (const auto single_res = m_voice[ch]->seek(seconds, m_scratch.data(), m_scratch_size);
            !single_res)
        {
            res = single_res;
        }
    });

    return res;
}


void AudioDevice::stop(SoundHandle voice_handle)
{
    foreach_voice(voice_handle, [this](int ch) {
        stop_voice_internal(ch);
    });
}

void AudioDevice::stop_audio_source(AudioSource& sound)
{
    if (sound.audio_source_id == 0)
    {
        return;
    }

    lock_audio_mutex_internal();

    for (size_t i = 0; i < m_highest_voice; ++i)
    {
        if (const auto& voice = m_voice[i];
            voice != nullptr && voice->audio_source_id == sound.audio_source_id)
        {
            stop_voice_internal(i);
        }
    }

    unlock_audio_mutex_internal();
}

void AudioDevice::stop_all()
{
    lock_audio_mutex_internal();
    for (size_t i = 0; i < m_highest_voice; ++i)
    {
        stop_voice_internal(i);
    }
    unlock_audio_mutex_internal();
}

auto AudioDevice::count_audio_source(const AudioSource& sound) -> int
{
    if (sound.audio_source_id == 0)
    {
        return 0;
    }

    auto count = 0;

    lock_audio_mutex_internal();

    for (size_t i = 0; i < m_highest_voice; ++i)
    {
        if (const auto& voice = m_voice[i];
            voice != nullptr && voice->audio_source_id == sound.audio_source_id)
        {
            ++count;
        }
    }

    unlock_audio_mutex_internal();

    return count;
}

void AudioDevice::schedule_pause(SoundHandle voice_handle, SoundTime time)
{
    if (time <= 0)
    {
        set_pause(voice_handle, 1);
        return;
    }

    foreach_voice(voice_handle, [this, time](int ch) {
        m_voice[ch]->pause_scheduler.set(1, 0, time, m_voice[ch]->stream_time);
    });
}

void AudioDevice::schedule_stop(SoundHandle voice_handle, SoundTime time)
{
    if (time <= 0)
    {
        stop(voice_handle);
        return;
    }

    foreach_voice(voice_handle, [this, time](int ch) {
        m_voice[ch]->stop_scheduler.set(1, 0, time, m_voice[ch]->stream_time);
    });
}

void AudioDevice::fade_volume(SoundHandle voice_handle, float to, SoundTime time)
{
    const auto from = volume(voice_handle);

    if (time <= 0.0 || to == from)
    {
        set_volume(voice_handle, to);
        return;
    }

    foreach_voice(voice_handle, [this, from, to, time](int ch) {
        m_voice[ch]->volume_fader.set(from, to, time, m_voice[ch]->stream_time);
    });
}

void AudioDevice::fade_pan(SoundHandle voice_handle, float to, SoundTime time)
{
    const auto from = pan(voice_handle);

    if (time <= 0.0 || to == from)
    {
        set_pan(voice_handle, to);
        return;
    }

    foreach_voice(voice_handle, [this, time, from, to](int ch) {
        m_voice[ch]->pan_fader.set(from, to, time, m_voice[ch]->stream_time);
    });
}

void AudioDevice::fade_relative_play_speed(SoundHandle voice_handle, float to, SoundTime time)
{
    const auto from = relative_play_speed(voice_handle);

    if (time <= 0.0 || to == from)
    {
        set_relative_play_speed(voice_handle, to);
        return;
    }

    foreach_voice(voice_handle, [this, from, to, time](int ch) {
        m_voice[ch]->relative_play_speed_fader.set(from, to, time, m_voice[ch]->stream_time);
    });
}

void AudioDevice::fade_global_volume(float to_volume, SoundTime fade_duration)
{
    const auto from = global_volume();

    if (fade_duration <= 0.0 || to_volume == from)
    {
        set_global_volume(to_volume);
        return;
    }

    m_global_volume_fader.set(from, to_volume, fade_duration, m_stream_time);
}


void AudioDevice::oscillate_volume(SoundHandle voice_handle, float from, float to, SoundTime aTime)
{
    if (aTime <= 0 || to == from)
    {
        set_volume(voice_handle, to);
        return;
    }

    foreach_voice(voice_handle, [this, from, to, aTime](int ch) {
        m_voice[ch]->volume_fader.setLFO(from, to, aTime, m_voice[ch]->stream_time);
    });
}

void AudioDevice::oscillate_pan(SoundHandle voice_handle, float aFrom, float aTo, SoundTime aTime)
{
    if (aTime <= 0 || aTo == aFrom)
    {
        set_pan(voice_handle, aTo);
        return;
    }

    foreach_voice(voice_handle, [this, aFrom, aTo, aTime](int ch) {
        m_voice[ch]->pan_fader.setLFO(aFrom, aTo, aTime, m_voice[ch]->stream_time);
    });
}

void AudioDevice::oscillate_relative_play_speed(SoundHandle voice_handle,
                                                float       aFrom,
                                                float       aTo,
                                                SoundTime   aTime)
{
    if (aTime <= 0 || aTo == aFrom)
    {
        set_relative_play_speed(voice_handle, aTo);
        return;
    }

    foreach_voice(voice_handle, [this, aFrom, aTo, aTime](int ch) {
        m_voice[ch]->relative_play_speed_fader.setLFO(aFrom, aTo, aTime, m_voice[ch]->stream_time);
    });
}

void AudioDevice::oscillate_global_volume(float aFrom, float aTo, SoundTime aTime)
{
    if (aTime <= 0 || aTo == aFrom)
    {
        set_global_volume(aTo);
        return;
    }

    m_global_volume_fader.setLFO(aFrom, aTo, aTime, m_stream_time);
}

AlignedFloatBuffer::AlignedFloatBuffer(size_t floats)
    : m_count(floats)
{
#ifndef SOLOUD_SSE_INTRINSICS
    m_data        = std::make_unique<unsigned char[]>(m_count * sizeof(float));
    m_aligned_ptr = reinterpret_cast<float*>(m_data.get());
#else
    m_data        = std::make_unique<unsigned char[]>(m_count * sizeof(float) + 16);
    m_aligned_ptr = (float*)(((size_t)m_data.get() + 15) & ~15);
#endif
}

auto AlignedFloatBuffer::data() -> float*
{
    return m_aligned_ptr;
}

auto AlignedFloatBuffer::data() const -> const float*
{
    return m_aligned_ptr;
}

auto AlignedFloatBuffer::operator[](size_t index) -> float&
{
    return m_aligned_ptr[index];
}

auto AlignedFloatBuffer::operator[](size_t index) const -> const float&
{
    return m_aligned_ptr[index];
}

TinyAlignedFloatBuffer::TinyAlignedFloatBuffer()
{
    m_aligned_ptr = reinterpret_cast<float*>((uintptr_t(m_data.data()) + 15) & ~15);
}

auto TinyAlignedFloatBuffer::data() -> float*
{
    return m_aligned_ptr;
}

auto TinyAlignedFloatBuffer::data() const -> const float*
{
    return m_aligned_ptr;
}

auto TinyAlignedFloatBuffer::operator[](size_t index) -> float&
{
    return m_aligned_ptr[index];
}

auto TinyAlignedFloatBuffer::operator[](size_t index) const -> const float&
{
    return m_aligned_ptr[index];
}

void AudioDevice::pause()
{
    if (m_backend_pause_func != nullptr)
    {
        m_backend_pause_func(this);
    }
}

void AudioDevice::resume()
{
    if (m_backend_resume_func != nullptr)
    {
        m_backend_resume_func(this);
    }
}


void AudioDevice::postinit_internal(size_t sample_rate, size_t buffer_size, size_t channels)
{
    m_global_volume = 1;
    m_channels      = channels;
    m_sample_rate   = sample_rate;
    m_buffer_size   = buffer_size;
    m_scratch_size  = (buffer_size + 15) & (~0xf); // round to the next div by 16

    m_scratch_size = std::max(m_scratch_size, sample_granularity * 2);
    m_scratch_size = std::max(m_scratch_size, size_t(4096));

    m_scratch        = AlignedFloatBuffer{m_scratch_size * max_channels};
    m_output_scratch = AlignedFloatBuffer{m_scratch_size * max_channels};

    m_resample_data.resize(m_max_active_voices * 2);
    m_resample_data_owner.resize(m_max_active_voices);

    m_resample_data_buffer =
        AlignedFloatBuffer{m_max_active_voices * 2 * sample_granularity * max_channels};

    for (size_t i = 0; i < m_max_active_voices * 2; ++i)
    {
        m_resample_data[i] =
            m_resample_data_buffer.data() + (sample_granularity * max_channels * i);
    }

    m_post_clip_scaler = 0.95f;

    switch (m_channels)
    {
        case 1: {
            m_3d_speaker_position[0] = {0, 0, 1};
            break;
        }
        case 2: {
            m_3d_speaker_position[0] = {2, 0, 1};
            m_3d_speaker_position[1] = {-2, 0, 1};
            break;
        }
        case 4: {
            m_3d_speaker_position[0] = {2, 0, 1};
            m_3d_speaker_position[1] = {-2, 0, 1};

            // I suppose technically the second pair should be straight left & right,
            // but I prefer moving them a bit back to mirror the front speakers.
            m_3d_speaker_position[2] = {2, 0, -1};
            m_3d_speaker_position[3] = {-2, 0, -1};

            break;
        }
        case 6: {
            m_3d_speaker_position[0] = {2, 0, 1};
            m_3d_speaker_position[1] = {-2, 0, 1};

            // center and subwoofer.
            m_3d_speaker_position[2] = {0, 0, 1};

            // Sub should be "mix of everything". We'll handle it as a special case and make it a
            // null vector.
            m_3d_speaker_position[3] = {0, 0, 0};

            // I suppose technically the second pair should be straight left & right,
            // but I prefer moving them a bit back to mirror the front speakers.
            m_3d_speaker_position[4] = {2, 0, -1};
            m_3d_speaker_position[5] = {-2, 0, -1};

            break;
        }
        case 8: {
            m_3d_speaker_position[0] = {2, 0, 1};
            m_3d_speaker_position[1] = {-2, 0, 1};

            // center and subwoofer.
            m_3d_speaker_position[2] = {0, 0, 1};

            // Sub should be "mix of everything". We'll handle it as a special case and make it a
            // null vector.
            m_3d_speaker_position[3] = {0, 0, 0};

            // side
            m_3d_speaker_position[4] = {2, 0, 0};
            m_3d_speaker_position[5] = {-2, 0, 0};

            // back
            m_3d_speaker_position[6] = {2, 0, -1};
            m_3d_speaker_position[7] = {-2, 0, -1};

            break;
        }
        default: break;
    }
}

auto AudioDevice::get_wave() -> float*
{
    lock_audio_mutex_internal();

    for (int i = 0; i < 256; ++i)
    {
        m_wave_data[i] = m_visualization_wave_data[i];
    }

    unlock_audio_mutex_internal();
    return m_wave_data.data();
}

auto AudioDevice::get_approximate_volume(size_t channel) -> float
{
    if (channel > m_channels)
    {
        return 0;
    }

    float vol = 0;

    lock_audio_mutex_internal();
    vol = m_visualization_channel_volume[channel];
    unlock_audio_mutex_internal();

    return vol;
}

auto AudioDevice::calc_fft() -> float*
{
    lock_audio_mutex_internal();
    auto temp = std::array<float, 1024>{};

    for (size_t i = 0; i < 256; ++i)
    {
        temp[i * 2]       = m_visualization_wave_data[i];
        temp[(i * 2) + 1] = 0;
        temp[i + 512]     = 0;
        temp[i + 768]     = 0;
    }
    unlock_audio_mutex_internal();

    FFT::fft1024(temp.data());

    for (size_t i = 0; i < 256; ++i)
    {
        const auto real = temp[i * 2];
        const auto imag = temp[i * 2 + 1];

        m_fft_data[i] = std::sqrt((real * real) + imag * imag);
    }

    return m_fft_data.data();
}

#if defined(SOLOUD_SSE_INTRINSICS)
void AudioDevice::clip_internal(const AlignedFloatBuffer& buffer,
                                AlignedFloatBuffer&       dst_buffer,
                                size_t                    samples,
                                float                     volume0,
                                float                     volume1)
{
    float  vd = (volume1 - volume0) / samples;
    float  v  = volume0;
    size_t i, c, d;
    size_t samplequads = (samples + 3) / 4; // rounded up

    // Clip
    if (m_flags.clip_roundoff)
    {
        float                  nb          = -1.65f;
        __m128                 negbound    = _mm_load_ps1(&nb);
        float                  pb          = 1.65f;
        __m128                 posbound    = _mm_load_ps1(&pb);
        float                  ls          = 0.87f;
        __m128                 linearscale = _mm_load_ps1(&ls);
        float                  cs          = -0.1f;
        __m128                 cubicscale  = _mm_load_ps1(&cs);
        float                  nw          = -0.9862875f;
        __m128                 negwall     = _mm_load_ps1(&nw);
        float                  pw          = 0.9862875f;
        __m128                 poswall     = _mm_load_ps1(&pw);
        __m128                 postscale   = _mm_load_ps1(&m_post_clip_scaler);
        TinyAlignedFloatBuffer volumes;
        volumes[0] = v;
        volumes[1] = v + vd;
        volumes[2] = v + vd + vd;
        volumes[3] = v + vd + vd + vd;
        vd *= 4;
        __m128 vdelta = _mm_load_ps1(&vd);
        c             = 0;
        d             = 0;
        for (size_t j = 0; j < m_channels; ++j)
        {
            __m128 vol = _mm_load_ps(volumes.data());

            for (i = 0; i < samplequads; ++i)
            {
                // float f1 = origdata[c] * v;	++c; v += vd;
                __m128 f = _mm_load_ps(&buffer[c]);
                c += 4;
                f   = _mm_mul_ps(f, vol);
                vol = _mm_add_ps(vol, vdelta);

                // float u1 = (f1 > -1.65f);
                __m128 u = _mm_cmpgt_ps(f, negbound);

                // float o1 = (f1 < 1.65f);
                __m128 o = _mm_cmplt_ps(f, posbound);

                // f1 = (0.87f * f1 - 0.1f * f1 * f1 * f1) * u1 * o1;
                __m128 lin   = _mm_mul_ps(f, linearscale);
                __m128 cubic = _mm_mul_ps(f, f);
                cubic        = _mm_mul_ps(cubic, f);
                cubic        = _mm_mul_ps(cubic, cubicscale);
                f            = _mm_add_ps(cubic, lin);

                // f1 = f1 * u1 + !u1 * -0.9862875f;
                __m128 lowmask  = _mm_andnot_ps(u, negwall);
                __m128 ilowmask = _mm_and_ps(u, f);
                f               = _mm_add_ps(lowmask, ilowmask);

                // f1 = f1 * o1 + !o1 * 0.9862875f;
                __m128 himask  = _mm_andnot_ps(o, poswall);
                __m128 ihimask = _mm_and_ps(o, f);
                f              = _mm_add_ps(himask, ihimask);

                // outdata[d] = f1 * postclip; d++;
                f = _mm_mul_ps(f, postscale);
                _mm_store_ps(&dst_buffer[d], f);
                d += 4;
            }
        }
    }
    else
    {
        float                  nb        = -1.0f;
        __m128                 negbound  = _mm_load_ps1(&nb);
        float                  pb        = 1.0f;
        __m128                 posbound  = _mm_load_ps1(&pb);
        __m128                 postscale = _mm_load_ps1(&m_post_clip_scaler);
        TinyAlignedFloatBuffer volumes;
        volumes[0] = v;
        volumes[1] = v + vd;
        volumes[2] = v + vd + vd;
        volumes[3] = v + vd + vd + vd;
        vd *= 4;
        __m128 vdelta = _mm_load_ps1(&vd);
        c             = 0;
        d             = 0;
        for (size_t j = 0; j < m_channels; ++j)
        {
            __m128 vol = _mm_load_ps(volumes.data());
            for (i = 0; i < samplequads; ++i)
            {
                // float f1 = buffer[c] * v; ++c; v += vd;
                __m128 f = _mm_load_ps(&buffer[c]);
                c += 4;
                f   = _mm_mul_ps(f, vol);
                vol = _mm_add_ps(vol, vdelta);

                // f1 = (f1 <= -1) ? -1 : (f1 >= 1) ? 1 : f1;
                f = _mm_max_ps(f, negbound);
                f = _mm_min_ps(f, posbound);

                // dst_buffer[d] = f1 * mPostClipScaler; d++;
                f = _mm_mul_ps(f, postscale);
                _mm_store_ps(&dst_buffer[d], f);
                d += 4;
            }
        }
    }
}
#else // fallback code
void AudioDevice::clip_internal(const AlignedFloatBuffer& buffer,
                                AlignedFloatBuffer&       dst_buffer,
                                size_t                    samples,
                                float                     volume0,
                                float                     volume1)
{
    const float vd           = (volume1 - volume0) / samples;
    const auto  sample_quads = (samples + 3) / 4; // rounded up

    // Clip
    if (m_flags.clip_roundoff)
    {
        auto c = size_t(0);
        auto d = size_t(0);

        for (size_t j = 0; j < m_channels; ++j)
        {
            auto v = volume0;

            for (size_t i = 0; i < sample_quads; ++i)
            {
                float f1 = buffer[c] * v;
                ++c;
                v += vd;

                float f2 = buffer[c] * v;
                ++c;
                v += vd;

                float f3 = buffer[c] * v;
                ++c;
                v += vd;

                float f4 = buffer[c] * v;
                ++c;
                v += vd;

                constexpr auto mn1 = 0.9862875f;
                constexpr auto mn2 = 1.65f;
                constexpr auto mn3 = 0.87f;
                constexpr auto mn4 = 0.1f;

                f1 = f1 <= -mn2 ? -mn1 : f1 >= mn2 ? mn1 : (mn3 * f1) - mn4 * f1 * f1 * f1;
                f2 = f2 <= -mn2 ? -mn1 : f2 >= mn2 ? mn1 : (mn3 * f2) - mn4 * f2 * f2 * f2;
                f3 = f3 <= -mn2 ? -mn1 : f3 >= mn2 ? mn1 : (mn3 * f3) - mn4 * f3 * f3 * f3;
                f4 = f4 <= -mn2 ? -mn1 : f4 >= mn2 ? mn1 : (mn3 * f4) - mn4 * f4 * f4 * f4;

                dst_buffer[d] = f1 * m_post_clip_scaler;
                d++;

                dst_buffer[d] = f2 * m_post_clip_scaler;
                d++;

                dst_buffer[d] = f3 * m_post_clip_scaler;
                d++;

                dst_buffer[d] = f4 * m_post_clip_scaler;
                d++;
            }
        }
    }
    else
    {
        auto c = size_t(0);
        auto d = size_t(0);

        for (size_t j = 0; j < m_channels; ++j)
        {
            auto v = volume0;

            for (size_t i = 0; i < sample_quads; ++i)
            {
                float f1 = buffer[c] * v;
                ++c;
                v += vd;

                float f2 = buffer[c] * v;
                ++c;
                v += vd;

                float f3 = buffer[c] * v;
                ++c;
                v += vd;

                float f4 = buffer[c] * v;
                ++c;
                v += vd;

                f1 = f1 <= -1 ? -1 : f1 >= 1 ? 1 : f1;
                f2 = f2 <= -1 ? -1 : f2 >= 1 ? 1 : f2;
                f3 = f3 <= -1 ? -1 : f3 >= 1 ? 1 : f3;
                f4 = f4 <= -1 ? -1 : f4 >= 1 ? 1 : f4;

                dst_buffer[d] = f1 * m_post_clip_scaler;
                d++;

                dst_buffer[d] = f2 * m_post_clip_scaler;
                d++;

                dst_buffer[d] = f3 * m_post_clip_scaler;
                d++;

                dst_buffer[d] = f4 * m_post_clip_scaler;
                d++;
            }
        }
    }
}
#endif

static constexpr auto s_fixpoint_frac_bits = 20;
static constexpr auto s_fixpoint_frac_mul  = 1 << s_fixpoint_frac_bits;
static constexpr auto s_fixpoint_frac_mask = (1 << s_fixpoint_frac_bits) - 1;

static auto catmull_rom(float t, float p0, float p1, float p2, float p3) -> float
{
    return 0.5f * (2 * p1 + (-p0 + p2) * t + (2 * p0 - 5 * p1 + 4 * p2 - p3) * t * t +
                   (-p0 + 3 * p1 - 3 * p2 + p3) * t * t * t);
}

static void resample_catmull_rom(const float* aSrc,
                                 const float* aSrc1,
                                 float*       aDst,
                                 size_t       aSrcOffset,
                                 size_t       aDstSampleCount,
                                 size_t       aStepFixed)
{
    auto pos = aSrcOffset;

    for (size_t i = 0; i < aDstSampleCount; ++i, pos += aStepFixed)
    {
        const auto p = pos >> s_fixpoint_frac_bits;
        const auto f = pos & s_fixpoint_frac_mask;

        auto s3 = 0.0f;

        if (p < 3)
        {
            s3 = aSrc1[512 + p - 3];
        }
        else
        {
            s3 = aSrc[p - 3];
        }

        auto s2 = 0.0f;

        if (p < 2)
        {
            s2 = aSrc1[512 + p - 2];
        }
        else
        {
            s2 = aSrc[p - 2];
        }

        auto s1 = 0.0f;

        if (p < 1)
        {
            s1 = aSrc1[512 + p - 1];
        }
        else
        {
            s1 = aSrc[p - 1];
        }

        const auto s0 = aSrc[p];

        aDst[i] = catmull_rom(f / float(s_fixpoint_frac_mul), s3, s2, s1, s0);
    }
}

static void resample_linear(const float* aSrc,
                            const float* aSrc1,
                            float*       aDst,
                            size_t       aSrcOffset,
                            size_t       aDstSampleCount,
                            size_t       aStepFixed)
{
    auto pos = aSrcOffset;

    for (size_t i = 0; i < aDstSampleCount; ++i, pos += aStepFixed)
    {
        const auto p  = pos >> s_fixpoint_frac_bits;
        const auto f  = pos & s_fixpoint_frac_mask;
        auto       s1 = aSrc1[sample_granularity - 1];
        const auto s2 = aSrc[p];
        if (p != 0)
        {
            s1 = aSrc[p - 1];
        }
        aDst[i] = s1 + (s2 - s1) * f * (1 / (float)s_fixpoint_frac_mul);
    }
}

static void resample_point(const float* src,
                           const float* src1,
                           float*       dst,
                           size_t       src_offset,
                           size_t       dst_sample_count,
                           size_t       step_fixed)
{
    auto pos = src_offset;

    for (size_t i = 0; i < dst_sample_count; ++i, pos += step_fixed)
    {
        const auto p = pos >> s_fixpoint_frac_bits;
        dst[i]       = src[p];
    }
}

void panAndExpand(SharedPtr<AudioSourceInstance>& voice,
                  float*                          buffer,
                  size_t                          samples_to_read,
                  size_t                          buffer_size,
                  float*                          scratch,
                  size_t                          channels)
{
#ifdef SOLOUD_SSE_INTRINSICS
    assert(((size_t)buffer & 0xf) == 0);
    assert(((size_t)scratch & 0xf) == 0);
    assert(((size_t)buffer_size & 0xf) == 0);
#endif

    auto pan  = std::array<float, max_channels>{}; // current speaker volume
    auto pand = std::array<float, max_channels>{}; // destination speaker volume
    auto pani = std::array<float, max_channels>{}; // speaker volume increment per sample

    for (size_t k = 0; k < channels; k++)
    {
        pan[k]  = voice->current_channel_volume[k];
        pand[k] = voice->channel_volume[k] * voice->overall_volume;
        pani[k] =
            (pand[k] - pan[k]) /
            samples_to_read; // TODO: this is a bit inconsistent.. but it's a hack to begin with
    }

    switch (channels)
    {
        case 1: // Target is mono. Sum everything. (1->1, 2->1, 4->1, 6->1, 8->1)
            for (size_t j = 0, ofs = 0; j < voice->channel_count; ++j, ofs += buffer_size)
            {
                pan[0] = voice->current_channel_volume[0];
                for (size_t k = 0; k < samples_to_read; k++)
                {
                    pan[0] += pani[0];
                    buffer[k] += scratch[ofs + k] * pan[0];
                }
            }
            break;
        case 2:
            switch (voice->channel_count)
            {
                case 8: // 8->2, just sum lefties and righties, add a bit of center and sub?
                    for (size_t j = 0; j < samples_to_read; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        float s1 = scratch[j];
                        float s2 = scratch[buffer_size + j];
                        float s3 = scratch[buffer_size * 2 + j];
                        float s4 = scratch[buffer_size * 3 + j];
                        float s5 = scratch[buffer_size * 4 + j];
                        float s6 = scratch[buffer_size * 5 + j];
                        float s7 = scratch[buffer_size * 6 + j];
                        float s8 = scratch[buffer_size * 7 + j];
                        buffer[j + 0] += 0.2f * (s1 + s3 + s4 + s5 + s7) * pan[0];
                        buffer[j + buffer_size] += 0.2f * (s2 + s3 + s4 + s6 + s8) * pan[1];
                    }
                    break;
                case 6: // 6->2, just sum lefties and righties, add a bit of center and sub?
                    for (size_t j = 0; j < samples_to_read; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        float s1 = scratch[j];
                        float s2 = scratch[buffer_size + j];
                        float s3 = scratch[buffer_size * 2 + j];
                        float s4 = scratch[buffer_size * 3 + j];
                        float s5 = scratch[buffer_size * 4 + j];
                        float s6 = scratch[buffer_size * 5 + j];
                        buffer[j + 0] += 0.3f * (s1 + s3 + s4 + s5) * pan[0];
                        buffer[j + buffer_size] += 0.3f * (s2 + s3 + s4 + s6) * pan[1];
                    }
                    break;
                case 4: // 4->2, just sum lefties and righties
                    for (size_t j = 0; j < samples_to_read; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        float s1 = scratch[j];
                        float s2 = scratch[buffer_size + j];
                        float s3 = scratch[buffer_size * 2 + j];
                        float s4 = scratch[buffer_size * 3 + j];
                        buffer[j + 0] += 0.5f * (s1 + s3) * pan[0];
                        buffer[j + buffer_size] += 0.5f * (s2 + s4) * pan[1];
                    }
                    break;
                case 2: // 2->2
#if defined(SOLOUD_SSE_INTRINSICS)
                {
                    int c = 0;
                    // if ((buffer_size & 3) == 0)
                    {
                        size_t                 samplequads = samples_to_read / 4; // rounded down
                        TinyAlignedFloatBuffer pan0;
                        pan0[0] = pan[0] + pani[0];
                        pan0[1] = pan[0] + pani[0] * 2;
                        pan0[2] = pan[0] + pani[0] * 3;
                        pan0[3] = pan[0] + pani[0] * 4;
                        TinyAlignedFloatBuffer pan1;
                        pan1[0] = pan[1] + pani[1];
                        pan1[1] = pan[1] + pani[1] * 2;
                        pan1[2] = pan[1] + pani[1] * 3;
                        pan1[3] = pan[1] + pani[1] * 4;
                        pani[0] *= 4;
                        pani[1] *= 4;
                        __m128 pan0delta = _mm_load_ps1(&pani[0]);
                        __m128 pan1delta = _mm_load_ps1(&pani[1]);
                        __m128 p0        = _mm_load_ps(pan0.data());
                        __m128 p1        = _mm_load_ps(pan1.data());

                        for (size_t j = 0; j < samplequads; ++j)
                        {
                            __m128 f0 = _mm_load_ps(scratch + c);
                            __m128 c0 = _mm_mul_ps(f0, p0);
                            __m128 f1 = _mm_load_ps(scratch + c + buffer_size);
                            __m128 c1 = _mm_mul_ps(f1, p1);
                            __m128 o0 = _mm_load_ps(buffer + c);
                            __m128 o1 = _mm_load_ps(buffer + c + buffer_size);
                            c0        = _mm_add_ps(c0, o0);
                            c1        = _mm_add_ps(c1, o1);
                            _mm_store_ps(buffer + c, c0);
                            _mm_store_ps(buffer + c + buffer_size, c1);
                            p0 = _mm_add_ps(p0, pan0delta);
                            p1 = _mm_add_ps(p1, pan1delta);
                            c += 4;
                        }
                    }

                    // If buffer size or samples to read are not divisible by 4, handle leftovers
                    for (size_t j = c; j < samples_to_read; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        float s1 = scratch[j];
                        float s2 = scratch[buffer_size + j];
                        buffer[j + 0] += s1 * pan[0];
                        buffer[j + buffer_size] += s2 * pan[1];
                    }
                }
#else // fallback
                    for (size_t j = 0; j < samples_to_read; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        float s1 = scratch[j];
                        float s2 = scratch[buffer_size + j];
                        buffer[j + 0] += s1 * pan[0];
                        buffer[j + buffer_size] += s2 * pan[1];
                    }
#endif
                break;
                case 1: // 1->2
#if defined(SOLOUD_SSE_INTRINSICS)
                {
                    int c = 0;
                    // if ((buffer_size & 3) == 0)
                    {
                        size_t                 samplequads = samples_to_read / 4; // rounded down
                        TinyAlignedFloatBuffer pan0;
                        pan0[0] = pan[0] + pani[0];
                        pan0[1] = pan[0] + pani[0] * 2;
                        pan0[2] = pan[0] + pani[0] * 3;
                        pan0[3] = pan[0] + pani[0] * 4;
                        TinyAlignedFloatBuffer pan1;
                        pan1[0] = pan[1] + pani[1];
                        pan1[1] = pan[1] + pani[1] * 2;
                        pan1[2] = pan[1] + pani[1] * 3;
                        pan1[3] = pan[1] + pani[1] * 4;
                        pani[0] *= 4;
                        pani[1] *= 4;
                        __m128 pan0delta = _mm_load_ps1(&pani[0]);
                        __m128 pan1delta = _mm_load_ps1(&pani[1]);
                        __m128 p0        = _mm_load_ps(pan0.data());
                        __m128 p1        = _mm_load_ps(pan1.data());

                        for (size_t j = 0; j < samplequads; ++j)
                        {
                            __m128 f  = _mm_load_ps(scratch + c);
                            __m128 c0 = _mm_mul_ps(f, p0);
                            __m128 c1 = _mm_mul_ps(f, p1);
                            __m128 o0 = _mm_load_ps(buffer + c);
                            __m128 o1 = _mm_load_ps(buffer + c + buffer_size);
                            c0        = _mm_add_ps(c0, o0);
                            c1        = _mm_add_ps(c1, o1);
                            _mm_store_ps(buffer + c, c0);
                            _mm_store_ps(buffer + c + buffer_size, c1);
                            p0 = _mm_add_ps(p0, pan0delta);
                            p1 = _mm_add_ps(p1, pan1delta);
                            c += 4;
                        }
                    }
                    // If buffer size or samples to read are not divisible by 4, handle leftovers
                    for (size_t j = c; j < samples_to_read; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        float s = scratch[j];
                        buffer[j + 0] += s * pan[0];
                        buffer[j + buffer_size] += s * pan[1];
                    }
                }
#else // fallback
                    for (size_t j = 0; j < samples_to_read; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        float s = scratch[j];
                        buffer[j + 0] += s * pan[0];
                        buffer[j + buffer_size] += s * pan[1];
                    }
#endif
                break;
            }
            break;
        case 4:
            switch (voice->channel_count)
            {
                case 8: // 8->4, add a bit of center, sub?
                    for (size_t j = 0; j < samples_to_read; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        pan[2] += pani[2];
                        pan[3] += pani[3];
                        float s1 = scratch[j];
                        float s2 = scratch[buffer_size + j];
                        float s3 = scratch[buffer_size * 2 + j];
                        float s4 = scratch[buffer_size * 3 + j];
                        float s5 = scratch[buffer_size * 4 + j];
                        float s6 = scratch[buffer_size * 5 + j];
                        float s7 = scratch[buffer_size * 6 + j];
                        float s8 = scratch[buffer_size * 7 + j];
                        float c  = (s3 + s4) * 0.7f;
                        buffer[j + 0] += s1 * pan[0] + c;
                        buffer[j + buffer_size] += s2 * pan[1] + c;
                        buffer[j + buffer_size * 2] += 0.5f * (s5 + s7) * pan[2];
                        buffer[j + buffer_size * 3] += 0.5f * (s6 + s8) * pan[3];
                    }
                    break;
                case 6: // 6->4, add a bit of center, sub?
                    for (size_t j = 0; j < samples_to_read; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        pan[2] += pani[2];
                        pan[3] += pani[3];
                        float s1 = scratch[j];
                        float s2 = scratch[buffer_size + j];
                        float s3 = scratch[buffer_size * 2 + j];
                        float s4 = scratch[buffer_size * 3 + j];
                        float s5 = scratch[buffer_size * 4 + j];
                        float s6 = scratch[buffer_size * 5 + j];
                        float c  = (s3 + s4) * 0.7f;
                        buffer[j + 0] += s1 * pan[0] + c;
                        buffer[j + buffer_size] += s2 * pan[1] + c;
                        buffer[j + buffer_size * 2] += s5 * pan[2];
                        buffer[j + buffer_size * 3] += s6 * pan[3];
                    }
                    break;
                case 4: // 4->4
                    for (size_t j = 0; j < samples_to_read; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        pan[2] += pani[2];
                        pan[3] += pani[3];
                        float s1 = scratch[j];
                        float s2 = scratch[buffer_size + j];
                        float s3 = scratch[buffer_size * 2 + j];
                        float s4 = scratch[buffer_size * 3 + j];
                        buffer[j + 0] += s1 * pan[0];
                        buffer[j + buffer_size] += s2 * pan[1];
                        buffer[j + buffer_size * 2] += s3 * pan[2];
                        buffer[j + buffer_size * 3] += s4 * pan[3];
                    }
                    break;
                case 2: // 2->4
                    for (size_t j = 0; j < samples_to_read; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        pan[2] += pani[2];
                        pan[3] += pani[3];
                        float s1 = scratch[j];
                        float s2 = scratch[buffer_size + j];
                        buffer[j + 0] += s1 * pan[0];
                        buffer[j + buffer_size] += s2 * pan[1];
                        buffer[j + buffer_size * 2] += s1 * pan[2];
                        buffer[j + buffer_size * 3] += s2 * pan[3];
                    }
                    break;
                case 1: // 1->4
                    for (size_t j = 0; j < samples_to_read; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        pan[2] += pani[2];
                        pan[3] += pani[3];
                        float s = scratch[j];
                        buffer[j + 0] += s * pan[0];
                        buffer[j + buffer_size] += s * pan[1];
                        buffer[j + buffer_size * 2] += s * pan[2];
                        buffer[j + buffer_size * 3] += s * pan[3];
                    }
                    break;
            }
            break;
        case 6:
            switch (voice->channel_count)
            {
                case 8: // 8->6
                    for (size_t j = 0; j < samples_to_read; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        pan[2] += pani[2];
                        pan[3] += pani[3];
                        pan[4] += pani[4];
                        pan[5] += pani[5];
                        float s1 = scratch[j];
                        float s2 = scratch[buffer_size + j];
                        float s3 = scratch[buffer_size * 2 + j];
                        float s4 = scratch[buffer_size * 3 + j];
                        float s5 = scratch[buffer_size * 4 + j];
                        float s6 = scratch[buffer_size * 5 + j];
                        float s7 = scratch[buffer_size * 6 + j];
                        float s8 = scratch[buffer_size * 7 + j];
                        buffer[j + 0] += s1 * pan[0];
                        buffer[j + buffer_size] += s2 * pan[1];
                        buffer[j + buffer_size * 2] += s3 * pan[2];
                        buffer[j + buffer_size * 3] += s4 * pan[3];
                        buffer[j + buffer_size * 4] += 0.5f * (s5 + s7) * pan[4];
                        buffer[j + buffer_size * 5] += 0.5f * (s6 + s8) * pan[5];
                    }
                    break;
                case 6: // 6->6
                    for (size_t j = 0; j < samples_to_read; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        pan[2] += pani[2];
                        pan[3] += pani[3];
                        pan[4] += pani[4];
                        pan[5] += pani[5];
                        float s1 = scratch[j];
                        float s2 = scratch[buffer_size + j];
                        float s3 = scratch[buffer_size * 2 + j];
                        float s4 = scratch[buffer_size * 3 + j];
                        float s5 = scratch[buffer_size * 4 + j];
                        float s6 = scratch[buffer_size * 5 + j];
                        buffer[j + 0] += s1 * pan[0];
                        buffer[j + buffer_size] += s2 * pan[1];
                        buffer[j + buffer_size * 2] += s3 * pan[2];
                        buffer[j + buffer_size * 3] += s4 * pan[3];
                        buffer[j + buffer_size * 4] += s5 * pan[4];
                        buffer[j + buffer_size * 5] += s6 * pan[5];
                    }
                    break;
                case 4: // 4->6
                    for (size_t j = 0; j < samples_to_read; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        pan[2] += pani[2];
                        pan[3] += pani[3];
                        pan[4] += pani[4];
                        pan[5] += pani[5];
                        float s1 = scratch[j];
                        float s2 = scratch[buffer_size + j];
                        float s3 = scratch[buffer_size * 2 + j];
                        float s4 = scratch[buffer_size * 3 + j];
                        buffer[j + 0] += s1 * pan[0];
                        buffer[j + buffer_size] += s2 * pan[1];
                        buffer[j + buffer_size * 2] += 0.5f * (s1 + s2) * pan[2];
                        buffer[j + buffer_size * 3] += 0.25f * (s1 + s2 + s3 + s4) * pan[3];
                        buffer[j + buffer_size * 4] += s3 * pan[4];
                        buffer[j + buffer_size * 5] += s4 * pan[5];
                    }
                    break;
                case 2: // 2->6
                    for (size_t j = 0; j < samples_to_read; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        pan[2] += pani[2];
                        pan[3] += pani[3];
                        pan[4] += pani[4];
                        pan[5] += pani[5];
                        float s1 = scratch[j];
                        float s2 = scratch[buffer_size + j];
                        buffer[j + 0] += s1 * pan[0];
                        buffer[j + buffer_size] += s2 * pan[1];
                        buffer[j + buffer_size * 2] += 0.5f * (s1 + s2) * pan[2];
                        buffer[j + buffer_size * 3] += 0.5f * (s1 + s2) * pan[3];
                        buffer[j + buffer_size * 4] += s1 * pan[4];
                        buffer[j + buffer_size * 5] += s2 * pan[5];
                    }
                    break;
                case 1: // 1->6
                    for (size_t j = 0; j < samples_to_read; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        pan[2] += pani[2];
                        pan[3] += pani[3];
                        pan[4] += pani[4];
                        pan[5] += pani[5];
                        float s = scratch[j];
                        buffer[j + 0] += s * pan[0];
                        buffer[j + buffer_size] += s * pan[1];
                        buffer[j + buffer_size * 2] += s * pan[2];
                        buffer[j + buffer_size * 3] += s * pan[3];
                        buffer[j + buffer_size * 4] += s * pan[4];
                        buffer[j + buffer_size * 5] += s * pan[5];
                    }
                    break;
            }
            break;
        case 8:
            switch (voice->channel_count)
            {
                case 8: // 8->8
                    for (size_t j = 0; j < samples_to_read; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        pan[2] += pani[2];
                        pan[3] += pani[3];
                        pan[4] += pani[4];
                        pan[5] += pani[5];
                        pan[6] += pani[6];
                        pan[7] += pani[7];
                        float s1 = scratch[j];
                        float s2 = scratch[buffer_size + j];
                        float s3 = scratch[buffer_size * 2 + j];
                        float s4 = scratch[buffer_size * 3 + j];
                        float s5 = scratch[buffer_size * 4 + j];
                        float s6 = scratch[buffer_size * 5 + j];
                        float s7 = scratch[buffer_size * 6 + j];
                        float s8 = scratch[buffer_size * 7 + j];
                        buffer[j + 0] += s1 * pan[0];
                        buffer[j + buffer_size] += s2 * pan[1];
                        buffer[j + buffer_size * 2] += s3 * pan[2];
                        buffer[j + buffer_size * 3] += s4 * pan[3];
                        buffer[j + buffer_size * 4] += s5 * pan[4];
                        buffer[j + buffer_size * 5] += s6 * pan[5];
                        buffer[j + buffer_size * 6] += s7 * pan[6];
                        buffer[j + buffer_size * 7] += s8 * pan[7];
                    }
                    break;
                case 6: // 6->8
                    for (size_t j = 0; j < samples_to_read; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        pan[2] += pani[2];
                        pan[3] += pani[3];
                        pan[4] += pani[4];
                        pan[5] += pani[5];
                        pan[6] += pani[6];
                        pan[7] += pani[7];
                        float s1 = scratch[j];
                        float s2 = scratch[buffer_size + j];
                        float s3 = scratch[buffer_size * 2 + j];
                        float s4 = scratch[buffer_size * 3 + j];
                        float s5 = scratch[buffer_size * 4 + j];
                        float s6 = scratch[buffer_size * 5 + j];
                        buffer[j + 0] += s1 * pan[0];
                        buffer[j + buffer_size] += s2 * pan[1];
                        buffer[j + buffer_size * 2] += s3 * pan[2];
                        buffer[j + buffer_size * 3] += s4 * pan[3];
                        buffer[j + buffer_size * 4] += 0.5f * (s5 + s1) * pan[4];
                        buffer[j + buffer_size * 5] += 0.5f * (s6 + s2) * pan[5];
                        buffer[j + buffer_size * 6] += s5 * pan[6];
                        buffer[j + buffer_size * 7] += s6 * pan[7];
                    }
                    break;
                case 4: // 4->8
                    for (size_t j = 0; j < samples_to_read; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        pan[2] += pani[2];
                        pan[3] += pani[3];
                        pan[4] += pani[4];
                        pan[5] += pani[5];
                        pan[6] += pani[6];
                        pan[7] += pani[7];
                        float s1 = scratch[j];
                        float s2 = scratch[buffer_size + j];
                        float s3 = scratch[buffer_size * 2 + j];
                        float s4 = scratch[buffer_size * 3 + j];
                        buffer[j + 0] += s1 * pan[0];
                        buffer[j + buffer_size] += s2 * pan[1];
                        buffer[j + buffer_size * 2] += 0.5f * (s1 + s2) * pan[2];
                        buffer[j + buffer_size * 3] += 0.25f * (s1 + s2 + s3 + s4) * pan[3];
                        buffer[j + buffer_size * 4] += 0.5f * (s1 + s3) * pan[4];
                        buffer[j + buffer_size * 5] += 0.5f * (s2 + s4) * pan[5];
                        buffer[j + buffer_size * 6] += s3 * pan[4];
                        buffer[j + buffer_size * 7] += s4 * pan[5];
                    }
                    break;
                case 2: // 2->8
                    for (size_t j = 0; j < samples_to_read; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        pan[2] += pani[2];
                        pan[3] += pani[3];
                        pan[4] += pani[4];
                        pan[5] += pani[5];
                        pan[6] += pani[6];
                        pan[7] += pani[7];
                        float s1 = scratch[j];
                        float s2 = scratch[buffer_size + j];
                        buffer[j + 0] += s1 * pan[0];
                        buffer[j + buffer_size] += s2 * pan[1];
                        buffer[j + buffer_size * 2] += 0.5f * (s1 + s2) * pan[2];
                        buffer[j + buffer_size * 3] += 0.5f * (s1 + s2) * pan[3];
                        buffer[j + buffer_size * 4] += s1 * pan[4];
                        buffer[j + buffer_size * 5] += s2 * pan[5];
                        buffer[j + buffer_size * 6] += s1 * pan[6];
                        buffer[j + buffer_size * 7] += s2 * pan[7];
                    }
                    break;
                case 1: // 1->8
                    for (size_t j = 0; j < samples_to_read; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        pan[2] += pani[2];
                        pan[3] += pani[3];
                        pan[4] += pani[4];
                        pan[5] += pani[5];
                        pan[6] += pani[6];
                        pan[7] += pani[7];
                        float s = scratch[j];
                        buffer[j + 0] += s * pan[0];
                        buffer[j + buffer_size] += s * pan[1];
                        buffer[j + buffer_size * 2] += s * pan[2];
                        buffer[j + buffer_size * 3] += s * pan[3];
                        buffer[j + buffer_size * 4] += s * pan[4];
                        buffer[j + buffer_size * 5] += s * pan[5];
                        buffer[j + buffer_size * 6] += s * pan[6];
                        buffer[j + buffer_size * 7] += s * pan[7];
                    }
                    break;
            }
            break;
    }

    for (size_t k = 0; k < channels; k++)
    {
        voice->current_channel_volume[k] = pand[k];
    }
}

void AudioDevice::mix_bus_internal(float*    buffer,
                                   size_t    samples_to_read,
                                   size_t    buffer_size,
                                   float*    scratch,
                                   size_t    bus,
                                   float     sample_rate,
                                   size_t    channels,
                                   Resampler resampler)
{
    // Clear accumulation buffer
    for (size_t i = 0; i < samples_to_read; ++i)
    {
        for (size_t j = 0; j < channels; ++j)
        {
            buffer[i + j * buffer_size] = 0;
        }
    }

    // Accumulate sound sources
    for (size_t i = 0; i < m_active_voice_count; ++i)
    {
        if (auto& voice = m_voice[m_active_voice[i]];
            voice != nullptr && voice->bus_handle == bus && !voice->flags.is_paused &&
            !voice->flags.inaudible)
        {
            float step = voice->sample_rate / sample_rate;

            // avoid step overflow
            if (step > (1 << (32 - s_fixpoint_frac_bits)))
            {
                step = 0;
            }

            const auto step_fixed = int(floor(step * s_fixpoint_frac_mul));
            auto       outofs     = size_t(0);

            if (voice->delay_samples)
            {
                if (voice->delay_samples > samples_to_read)
                {
                    outofs = samples_to_read;
                    voice->delay_samples -= samples_to_read;
                }
                else
                {
                    outofs               = voice->delay_samples;
                    voice->delay_samples = 0;
                }

                // Clear scratch where we're skipping
                for (size_t k = 0; k < voice->channel_count; k++)
                {
                    memset(scratch + k * buffer_size, 0, sizeof(float) * outofs);
                }
            }

            while (step_fixed != 0 && outofs < samples_to_read)
            {
                if (voice->leftover_samples == 0)
                {
                    // Swap resample buffers (ping-pong)
                    float* t                = voice->resample_data[0];
                    voice->resample_data[0] = voice->resample_data[1];
                    voice->resample_data[1] = t;

                    // Get a block of source data

                    auto read_count = size_t(0);

                    if (!voice->has_ended() || voice->flags.loops)
                    {
                        read_count = voice->audio(voice->resample_data[0],
                                                  sample_granularity,
                                                  sample_granularity);
                        if (read_count < sample_granularity)
                        {
                            if (voice->flags.loops)
                            {
                                while (read_count < sample_granularity &&
                                       voice->seek(voice->loop_point,
                                                   m_scratch.data(),
                                                   m_scratch_size))
                                {
                                    voice->loop_count++;

                                    const auto inc =
                                        voice->audio(voice->resample_data[0] + read_count,
                                                     sample_granularity - read_count,
                                                     sample_granularity);

                                    read_count += inc;
                                    if (inc == 0)
                                        break;
                                }
                            }
                        }
                    }

                    // Clear remaining of the resample data if the full scratch wasn't used
                    if (read_count < sample_granularity)
                    {
                        for (size_t k = 0; k < voice->channel_count; k++)
                        {
                            memset(voice->resample_data[0] + read_count + sample_granularity * k,
                                   0,
                                   sizeof(float) * (sample_granularity - read_count));
                        }
                    }

                    // If we go past zero, crop to zero (a bit of a kludge)
                    if (voice->src_offset < sample_granularity * s_fixpoint_frac_mul)
                    {
                        voice->src_offset = 0;
                    }
                    else
                    {
                        // We have new block of data, move pointer backwards
                        voice->src_offset -= sample_granularity * s_fixpoint_frac_mul;
                    }


                    // Run the per-stream filters to get our source data

                    for (size_t j = 0; j < filters_per_stream; ++j)
                    {
                        if (voice->filter[j])
                        {
                            voice->filter[j]->filter(FilterArgs{
                                .buffer      = voice->resample_data[0],
                                .samples     = sample_granularity,
                                .buffer_size = sample_granularity,
                                .channels    = voice->channel_count,
                                .sample_rate = voice->sample_rate,
                                .time        = m_stream_time,
                            });
                        }
                    }
                }
                else
                {
                    voice->leftover_samples = 0;
                }

                // Figure out how many samples we can generate from this source data.
                // The value may be zero.

                size_t writesamples = 0;

                if (voice->src_offset < sample_granularity * s_fixpoint_frac_mul)
                {
                    writesamples =
                        ((sample_granularity * s_fixpoint_frac_mul) - voice->src_offset) /
                            step_fixed +
                        1;

                    // avoid reading past the current buffer..
                    if (((writesamples * step_fixed + voice->src_offset) >> s_fixpoint_frac_bits) >=
                        sample_granularity)
                        writesamples--;
                }


                // If this is too much for our output buffer, don't write that many:
                if (writesamples + outofs > samples_to_read)
                {
                    voice->leftover_samples = (writesamples + outofs) - samples_to_read;
                    writesamples            = samples_to_read - outofs;
                }

                // Call resampler to generate the samples, once per channel
                if (writesamples != 0u)
                {
                    for (size_t j = 0; j < voice->channel_count; ++j)
                    {
                        switch (resampler)
                        {
                            case Resampler::Point:
                                resample_point(voice->resample_data[0] + sample_granularity * j,
                                               voice->resample_data[1] + sample_granularity * j,
                                               scratch + buffer_size * j + outofs,
                                               voice->src_offset,
                                               writesamples,
                                               /*voice->mSamplerate,
                                               aSamplerate,*/
                                               step_fixed);
                                break;
                            case Resampler::CatmullRom:
                                resample_catmull_rom(
                                    voice->resample_data[0] + sample_granularity * j,
                                    voice->resample_data[1] + sample_granularity * j,
                                    scratch + buffer_size * j + outofs,
                                    voice->src_offset,
                                    writesamples,
                                    /*voice->mSamplerate,
                                    aSamplerate,*/
                                    step_fixed);
                                break;
                            default:
                                // case RESAMPLER_LINEAR:
                                resample_linear(voice->resample_data[0] + sample_granularity * j,
                                                voice->resample_data[1] + sample_granularity * j,
                                                scratch + buffer_size * j + outofs,
                                                voice->src_offset,
                                                writesamples,
                                                /*voice->mSamplerate,
                                                aSamplerate,*/
                                                step_fixed);
                                break;
                        }
                    }
                }

                // Keep track of how many samples we've written so far
                outofs += writesamples;

                // Move source pointer onwards (writesamples may be zero)
                voice->src_offset += writesamples * step_fixed;
            }

            // Handle panning and channel expansion (and/or shrinking)
            panAndExpand(voice, buffer, samples_to_read, buffer_size, scratch, channels);

            // clear voice if the sound is over
            // TODO: check this condition some day
            if (!voice->flags.loops && !voice->flags.disable_autostop && voice->has_ended())
            {
                stop_voice_internal(m_active_voice[i]);
            }
        }
        else if (voice && voice->bus_handle == bus && !voice->flags.is_paused &&
                 voice->flags.inaudible && voice->flags.inaudible_tick)
        {
            // Inaudible but needs ticking. Do minimal work (keep counters up to date and ask
            // audiosource for data)
            auto step       = voice->sample_rate / sample_rate;
            auto step_fixed = int(floor(step * s_fixpoint_frac_mul));
            auto outofs     = size_t(0);

            if (voice->delay_samples != 0u)
            {
                if (voice->delay_samples > samples_to_read)
                {
                    outofs = samples_to_read;
                    voice->delay_samples -= samples_to_read;
                }
                else
                {
                    outofs               = voice->delay_samples;
                    voice->delay_samples = 0;
                }
            }

            while (step_fixed != 0 && outofs < samples_to_read)
            {
                if (voice->leftover_samples == 0)
                {
                    // Swap resample buffers (ping-pong)
                    float* t                = voice->resample_data[0];
                    voice->resample_data[0] = voice->resample_data[1];
                    voice->resample_data[1] = t;

                    // Get a block of source data

                    if (!voice->has_ended() || voice->flags.loops)
                    {
                        auto readcount = voice->audio(voice->resample_data[0],
                                                      sample_granularity,
                                                      sample_granularity);
                        if (readcount < sample_granularity)
                        {
                            if (voice->flags.loops)
                            {
                                while (readcount < sample_granularity &&
                                       voice->seek(voice->loop_point,
                                                   m_scratch.data(),
                                                   m_scratch_size))
                                {
                                    voice->loop_count++;
                                    readcount += voice->audio(voice->resample_data[0] + readcount,
                                                              sample_granularity - readcount,
                                                              sample_granularity);
                                }
                            }
                        }
                    }

                    // If we go past zero, crop to zero (a bit of a kludge)
                    if (voice->src_offset < sample_granularity * s_fixpoint_frac_mul)
                    {
                        voice->src_offset = 0;
                    }
                    else
                    {
                        // We have new block of data, move pointer backwards
                        voice->src_offset -= sample_granularity * s_fixpoint_frac_mul;
                    }

                    // Skip filters
                }
                else
                {
                    voice->leftover_samples = 0;
                }

                // Figure out how many samples we can generate from this source data.
                // The value may be zero.

                auto writesamples = size_t(0);

                if (voice->src_offset < sample_granularity * s_fixpoint_frac_mul)
                {
                    writesamples =
                        ((sample_granularity * s_fixpoint_frac_mul) - voice->src_offset) /
                            step_fixed +
                        1;

                    // avoid reading past the current buffer..
                    if (((writesamples * step_fixed + voice->src_offset) >> s_fixpoint_frac_bits) >=
                        sample_granularity)
                    {
                        --writesamples;
                    }
                }


                // If this is too much for our output buffer, don't write that many:
                if (writesamples + outofs > samples_to_read)
                {
                    voice->leftover_samples = (writesamples + outofs) - samples_to_read;
                    writesamples            = samples_to_read - outofs;
                }

                // Skip resampler

                // Keep track of how many samples we've written so far
                outofs += writesamples;

                // Move source pointer onwards (writesamples may be zero)
                voice->src_offset += writesamples * step_fixed;
            }

            // clear voice if the sound is over
            // TODO: check this condition some day
            if (!voice->flags.loops && !voice->flags.disable_autostop && voice->has_ended())
            {
                stop_voice_internal(m_active_voice[i]);
            }
        }
    }
}

void AudioDevice::map_resample_buffers_internal()
{
    assert(m_max_active_voices < 256);
    auto live = std::array<char, 256>{};

    for (size_t i = 0; i < m_max_active_voices; ++i)
    {
        for (size_t j = 0; j < m_max_active_voices; ++j)
        {
            if (m_resample_data_owner[i] &&
                m_resample_data_owner[i].get() == m_voice[m_active_voice[j]].get())
            {
                live[i] |= 1; // Live channel
                live[j] |= 2; // Live voice
            }
        }
    }

    for (size_t i = 0; i < m_max_active_voices; ++i)
    {
        if (((live[i] & 1) == 0) && m_resample_data_owner[i]) // For all dead channels with owners..
        {
            m_resample_data_owner[i]->resample_data[0] = nullptr;
            m_resample_data_owner[i]->resample_data[1] = nullptr;
            m_resample_data_owner[i]                   = nullptr;
        }
    }

    auto latest_free = size_t(0);

    for (size_t i = 0; i < m_active_voice_count; ++i)
    {
        if (!(live[i] & 2) && m_voice[m_active_voice[i]]) // For all live voices with no channel..
        {
            auto found = size_t(-1);

            for (size_t j = latest_free; found == size_t(-1) && j < m_max_active_voices; ++j)
            {
                if (m_resample_data_owner[j] == nullptr)
                {
                    found = j;
                }
            }

            assert(found != size_t(-1));

            m_resample_data_owner[found]                   = m_voice[m_active_voice[i]];
            m_resample_data_owner[found]->resample_data[0] = m_resample_data[found * 2 + 0];
            m_resample_data_owner[found]->resample_data[1] = m_resample_data[found * 2 + 1];

            memset(m_resample_data_owner[found]->resample_data[0],
                   0,
                   sizeof(float) * sample_granularity * max_channels);

            memset(m_resample_data_owner[found]->resample_data[1],
                   0,
                   sizeof(float) * sample_granularity * max_channels);

            latest_free = found + 1;
        }
    }
}

void AudioDevice::calc_active_voices_internal()
{
    // TODO: consider whether we need to re-evaluate the active voices all the time.
    // It is a must when new voices are started, but otherwise we could get away
    // with postponing it sometimes..

    m_active_voice_dirty = false;

    // Populate
    size_t candidates = 0;
    size_t must_live  = 0;

    for (size_t i = 0; i < m_highest_voice; ++i)
    {
        const auto voice = m_voice[i];
        if (voice == nullptr)
        {
            continue;
        }

        // TODO: check this some day
        if ((!voice->flags.inaudible && !voice->flags.is_paused) || voice->flags.inaudible_tick)
        {
            m_active_voice[candidates] = i;
            candidates++;
            if (m_voice[i]->flags.inaudible_tick)
            {
                m_active_voice[candidates - 1] = m_active_voice[must_live];
                m_active_voice[must_live]      = i;
                ++must_live;
            }
        }
    }

    // Check for early out
    if (candidates <= m_max_active_voices)
    {
        // everything is audible, early out
        m_active_voice_count = candidates;
        map_resample_buffers_internal();
        return;
    }

    m_active_voice_count = m_max_active_voices;

    if (must_live >= m_max_active_voices)
    {
        // Oopsie. Well, nothing to sort, since the "must live" voices already
        // ate all our active voice slots.
        // This is a potentially an error situation, but we have no way to report
        // error from here. And asserting could be bad, too.
        return;
    }

    // If we get this far, there's nothing to it: we'll have to sort the voices to find the most
    // audible.

    // Iterative partial quicksort:
    auto       left  = size_t(0);
    auto       stack = std::array<size_t, 24>{};
    auto       pos   = size_t(0);
    auto       len   = candidates - must_live;
    size_t*    data  = m_active_voice.data() + must_live;
    const auto k     = m_active_voice_count;
    while (true)
    {
        for (; left + 1 < len; len++)
        {
            if (pos == 24)
            {
                len = stack[pos = 0];
            }

            const auto pivot    = data[left];
            const auto pivotvol = m_voice[pivot]->overall_volume;
            stack[pos++]        = len;

            for (auto right = left - 1;;)
            {
                do
                {
                    ++right;
                } while (m_voice[data[right]]->overall_volume > pivotvol);

                do
                {
                    --len;
                } while (pivotvol > m_voice[data[len]]->overall_volume);

                if (right >= len)
                {
                    break;
                }

                std::swap(data[left], data[right]);
            }
        }

        if (pos == 0)
        {
            break;
        }

        if (left >= k)
        {
            break;
        }

        left = len;
        len  = stack[--pos];
    }

    // TODO: should the rest of the voices be flagged INAUDIBLE?
    map_resample_buffers_internal();
}

void AudioDevice::mix_internal(size_t samples, size_t stride)
{
#ifdef __arm__
    // flush to zero (FTZ) for ARM
    {
        static bool once = false;
        if (!once)
        {
            once = true;
            asm("vmsr fpscr,%0" ::"r"(1 << 24));
        }
    }
#endif

#ifdef _MCW_DN
    {
        static bool once = false;
        if (!once)
        {
            once = true;
            if (!m_flags.no_fpu_register_change)
            {
                _controlfp(_DN_FLUSH, _MCW_DN);
            }
        }
    }
#endif

#ifdef SOLOUD_SSE_INTRINSICS
    {
        static bool once = false;
        if (!once)
        {
            once = true;
            // Set denorm clear to zero (CTZ) and denorms are zero (DAZ) flags on.
            // This causes all math to consider really tiny values as zero, which
            // helps performance. I'd rather use constants from the sse headers,
            // but for some reason the DAZ value is not defined there(!)
            if (!m_flags.no_fpu_register_change)
            {
                _mm_setcsr(_mm_getcsr() | 0x8040);
            }
        }
    }
#endif

    const auto buffertime   = samples / float(m_sample_rate);
    auto       globalVolume = std::array<float, 2>{};

    m_stream_time += buffertime;
    m_last_clocked_time = 0;

    globalVolume[0] = m_global_volume;

    if (m_global_volume_fader.active != 0)
    {
        m_global_volume = m_global_volume_fader.get(m_stream_time);
    }

    globalVolume[1] = m_global_volume;

    lock_audio_mutex_internal();

    // Process faders. May change scratch size.
    for (size_t i = 0; i < m_highest_voice; ++i)
    {
        if (m_voice[i] != nullptr && !m_voice[i]->flags.is_paused)
        {
            auto volume = std::array<float, 2>{};

            m_voice[i]->active_fader = 0;

            if (m_global_volume_fader.active > 0)
            {
                m_voice[i]->active_fader = 1;
            }

            m_voice[i]->stream_time += buffertime;
            m_voice[i]->stream_position +=
                double(buffertime) * double(m_voice[i]->overall_relative_play_speed);

            // TODO: this is actually unstable, because mStreamTime depends on the relative play
            // speed.
            if (m_voice[i]->relative_play_speed_fader.active > 0)
            {
                float speed = m_voice[i]->relative_play_speed_fader.get(m_voice[i]->stream_time);
                set_voice_relative_play_speed_internal(i, speed);
            }

            volume[0] = m_voice[i]->overall_volume;

            if (m_voice[i]->volume_fader.active > 0)
            {
                m_voice[i]->set_volume   = m_voice[i]->volume_fader.get(m_voice[i]->stream_time);
                m_voice[i]->active_fader = 1;
                update_voice_volume_internal(i);
                m_active_voice_dirty = true;
            }

            volume[1] = m_voice[i]->overall_volume;

            if (m_voice[i]->pan_fader.active > 0)
            {
                float pan = m_voice[i]->pan_fader.get(m_voice[i]->stream_time);
                set_voice_pan_internal(i, pan);
                m_voice[i]->active_fader = 1;
            }

            if (m_voice[i]->pause_scheduler.active)
            {
                m_voice[i]->pause_scheduler.get(m_voice[i]->stream_time);
                if (m_voice[i]->pause_scheduler.active == -1)
                {
                    m_voice[i]->pause_scheduler.active = 0;
                    set_voice_pause_internal(i, 1);
                }
            }

            if (m_voice[i]->stop_scheduler.active)
            {
                m_voice[i]->stop_scheduler.get(m_voice[i]->stream_time);
                if (m_voice[i]->stop_scheduler.active == -1)
                {
                    m_voice[i]->stop_scheduler.active = 0;
                    stop_voice_internal(i);
                }
            }
        }
    }

    if (m_active_voice_dirty)
    {
        calc_active_voices_internal();
    }

    mix_bus_internal(m_output_scratch.data(),
                     samples,
                     stride,
                     m_scratch.data(),
                     0,
                     float(m_sample_rate),
                     m_channels,
                     m_resampler);

    for (size_t i = 0; i < filters_per_stream; ++i)
    {
        if (m_filter_instance[i] == nullptr)
        {
            continue;
        }

        m_filter_instance[i]->filter(FilterArgs{
            .buffer      = m_output_scratch.data(),
            .samples     = samples,
            .buffer_size = stride,
            .channels    = m_channels,
            .sample_rate = float(m_sample_rate),
            .time        = m_stream_time,
        });
    }

    unlock_audio_mutex_internal();

    // Note: clipping channels*aStride, not channels*aSamples, so we're possibly clipping some
    // unused data. The buffers should be large enough for it, we just may do a few bytes of
    // unnecessary work.
    clip_internal(m_output_scratch, m_scratch, stride, globalVolume[0], globalVolume[1]);

    if (m_flags.enable_visualization)
    {
        for (size_t i = 0; i < max_channels; ++i)
        {
            m_visualization_channel_volume[i] = 0;
        }

        if (samples > 255)
        {
            for (size_t i = 0; i < 256; ++i)
            {
                m_visualization_wave_data[i] = 0;
                for (size_t j = 0; j < m_channels; ++j)
                {
                    const auto sample = m_scratch[i + (j * stride)];
                    const auto absvol = fabs(sample);

                    if (m_visualization_channel_volume[j] < absvol)
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

                for (size_t j = 0; j < m_channels; ++j)
                {
                    const auto sample = m_scratch[(i % samples) + (j * stride)];
                    const auto absvol = fabs(sample);

                    if (m_visualization_channel_volume[j] < absvol)
                    {
                        m_visualization_channel_volume[j] = absvol;
                    }

                    m_visualization_wave_data[i] += sample;
                }
            }
        }
    }
}

void interlace_samples_float(
    const float* src_buffer, float* dst_buffer, size_t samples, size_t channels, size_t stride)
{
    // 111222 -> 121212
    for (size_t j = 0; j < channels; ++j)
    {
        auto c = j * stride;
        for (auto i = j; i < samples * channels; i += channels)
        {
            dst_buffer[i] = src_buffer[c];
            ++c;
        }
    }
}

void interlace_samples_s16(
    const float* src_buffer, short* dst_buffer, size_t samples, size_t channels, size_t stride)
{
    // 111222 -> 121212
    for (size_t j = 0; j < channels; ++j)
    {
        size_t c = j * stride;

        for (size_t i = j; i < samples * channels; i += channels)
        {
            dst_buffer[i] = short(src_buffer[c] * 0x7fff);
            ++c;
        }
    }
}

void AudioDevice::mix(float* buffer, size_t samples)
{
    size_t stride = (samples + 15) & ~0xf;
    mix_internal(samples, stride);
    interlace_samples_float(m_scratch.data(), buffer, samples, m_channels, stride);
}

void AudioDevice::mix_signed16(short* buffer, size_t samples)
{
    size_t stride = (samples + 15) & ~0xf;
    mix_internal(samples, stride);
    interlace_samples_s16(m_scratch.data(), buffer, samples, m_channels, stride);
}

void AudioDevice::lock_audio_mutex_internal()
{
    if (m_audio_thread_mutex != nullptr)
    {
        thread::lock_mutex(m_audio_thread_mutex);
    }
    assert(!m_inside_audio_thread_mutex);
    m_inside_audio_thread_mutex = true;
}

void AudioDevice::unlock_audio_mutex_internal()
{
    assert(m_inside_audio_thread_mutex);
    m_inside_audio_thread_mutex = false;
    if (m_audio_thread_mutex)
    {
        thread::unlock_mutex(m_audio_thread_mutex);
    }
}

auto AudioDevice::post_clip_scaler() const -> float
{
    return m_post_clip_scaler;
}

auto AudioDevice::main_resampler() const -> Resampler
{
    return m_resampler;
}

auto AudioDevice::global_volume() const -> float
{
    return m_global_volume;
}

auto AudioDevice::get_handle_from_voice_internal(size_t aVoice) const -> SoundHandle
{
    if (m_voice[aVoice] == nullptr)
    {
        return 0;
    }

    return (aVoice + 1) | (m_voice[aVoice]->play_index << 12);
}

auto AudioDevice::get_voice_from_handle_internal(SoundHandle voice_handle) const -> int
{
    // If this is a voice group handle, pick the first handle from the group
    if (const auto* h = voice_group_handle_to_array_internal(voice_handle); h != nullptr)
    {
        voice_handle = *h;
    }

    if (voice_handle == 0)
    {
        return -1;
    }

    const int    ch  = (voice_handle & 0xfff) - 1;
    const size_t idx = voice_handle >> 12;

    if (m_voice[ch] != nullptr && (m_voice[ch]->play_index & 0xfffff) == idx)
    {
        return ch;
    }

    return -1;
}

auto AudioDevice::max_active_voice_count() const -> size_t
{
    return m_max_active_voices;
}

auto AudioDevice::active_voice_count() -> size_t
{
    lock_audio_mutex_internal();
    if (m_active_voice_dirty)
    {
        calc_active_voices_internal();
    }
    const size_t c = m_active_voice_count;
    unlock_audio_mutex_internal();

    return c;
}

auto AudioDevice::voice_count() -> size_t
{
    lock_audio_mutex_internal();
    int c = 0;
    for (size_t i = 0; i < m_highest_voice; ++i)
    {
        if (m_voice[i])
        {
            ++c;
        }
    }
    unlock_audio_mutex_internal();
    return c;
}

auto AudioDevice::is_valid_voice_handle(SoundHandle voice_handle) -> bool
{
    // voice groups are not valid voice handles
    if ((voice_handle & 0xfffff000) == 0xfffff000)
    {
        return false;
    }

    lock_audio_mutex_internal();
    if (get_voice_from_handle_internal(voice_handle) != -1)
    {
        unlock_audio_mutex_internal();
        return true;
    }
    unlock_audio_mutex_internal();
    return false;
}


auto AudioDevice::get_loop_point(SoundHandle voice_handle) -> SoundTime
{
    lock_audio_mutex_internal();
    const int ch = get_voice_from_handle_internal(voice_handle);
    if (ch == -1)
    {
        unlock_audio_mutex_internal();
        return 0;
    }
    const SoundTime v = m_voice[ch]->loop_point;
    unlock_audio_mutex_internal();
    return v;
}

auto AudioDevice::is_voice_looping(SoundHandle voice_handle) -> bool
{
    lock_audio_mutex_internal();
    const int ch = get_voice_from_handle_internal(voice_handle);
    if (ch == -1)
    {
        unlock_audio_mutex_internal();
        return 0;
    }
    const bool v = m_voice[ch]->flags.loops;
    unlock_audio_mutex_internal();
    return v;
}

auto AudioDevice::get_auto_stop(SoundHandle voice_handle) -> bool
{
    lock_audio_mutex_internal();
    const int ch = get_voice_from_handle_internal(voice_handle);
    if (ch == -1)
    {
        unlock_audio_mutex_internal();
        return false;
    }
    const auto v = m_voice[ch]->flags.disable_autostop;
    unlock_audio_mutex_internal();
    return !v;
}

auto AudioDevice::get_info(SoundHandle voice_handle, size_t mInfoKey) -> float
{
    lock_audio_mutex_internal();
    const int ch = get_voice_from_handle_internal(voice_handle);
    if (ch == -1)
    {
        unlock_audio_mutex_internal();
        return 0;
    }
    const float v = m_voice[ch]->get_info(mInfoKey);
    unlock_audio_mutex_internal();
    return v;
}

auto AudioDevice::volume(SoundHandle voice_handle) -> float
{
    lock_audio_mutex_internal();
    const int ch = get_voice_from_handle_internal(voice_handle);
    if (ch == -1)
    {
        unlock_audio_mutex_internal();
        return 0;
    }
    const float v = m_voice[ch]->set_volume;
    unlock_audio_mutex_internal();
    return v;
}

auto AudioDevice::overall_volume(SoundHandle voice_handle) -> float
{
    lock_audio_mutex_internal();
    const int ch = get_voice_from_handle_internal(voice_handle);
    if (ch == -1)
    {
        unlock_audio_mutex_internal();
        return 0;
    }
    const float v = m_voice[ch]->overall_volume;
    unlock_audio_mutex_internal();
    return v;
}

auto AudioDevice::pan(SoundHandle voice_handle) -> float
{
    lock_audio_mutex_internal();
    const int ch = get_voice_from_handle_internal(voice_handle);
    if (ch == -1)
    {
        unlock_audio_mutex_internal();
        return 0;
    }
    const float v = m_voice[ch]->pan;
    unlock_audio_mutex_internal();
    return v;
}

auto AudioDevice::stream_time(SoundHandle voice_handle) -> SoundTime
{
    lock_audio_mutex_internal();
    const int ch = get_voice_from_handle_internal(voice_handle);
    if (ch == -1)
    {
        unlock_audio_mutex_internal();
        return 0;
    }
    const double v = m_voice[ch]->stream_time;
    unlock_audio_mutex_internal();
    return v;
}

auto AudioDevice::stream_position(SoundHandle voice_handle) -> SoundTime
{
    lock_audio_mutex_internal();
    const int ch = get_voice_from_handle_internal(voice_handle);
    if (ch == -1)
    {
        unlock_audio_mutex_internal();
        return 0;
    }
    const double v = m_voice[ch]->stream_position;
    unlock_audio_mutex_internal();
    return v;
}

auto AudioDevice::relative_play_speed(SoundHandle voice_handle) -> float
{
    lock_audio_mutex_internal();
    const int ch = get_voice_from_handle_internal(voice_handle);
    if (ch == -1)
    {
        unlock_audio_mutex_internal();
        return 1;
    }
    const float v = m_voice[ch]->set_relative_play_speed;
    unlock_audio_mutex_internal();
    return v;
}

auto AudioDevice::sample_rate(SoundHandle voice_handle) -> float
{
    lock_audio_mutex_internal();
    const int ch = get_voice_from_handle_internal(voice_handle);
    if (ch == -1)
    {
        unlock_audio_mutex_internal();
        return 0;
    }
    const float v = m_voice[ch]->base_sample_rate;
    unlock_audio_mutex_internal();
    return v;
}

auto AudioDevice::pause(SoundHandle voice_handle) -> bool
{
    lock_audio_mutex_internal();
    const int ch = get_voice_from_handle_internal(voice_handle);
    if (ch == -1)
    {
        unlock_audio_mutex_internal();
        return false;
    }
    const auto v = m_voice[ch]->flags.is_paused;
    unlock_audio_mutex_internal();
    return v;
}

auto AudioDevice::is_voice_protected(SoundHandle voice_handle) -> bool
{
    lock_audio_mutex_internal();
    const int ch = get_voice_from_handle_internal(voice_handle);
    if (ch == -1)
    {
        unlock_audio_mutex_internal();
        return false;
    }
    const auto v = m_voice[ch]->flags.is_protected;
    unlock_audio_mutex_internal();
    return v;
}

auto AudioDevice::find_free_voice_internal() -> size_t
{
    size_t lowest_play_index_value = 0xffffffff;
    size_t lowest_play_index       = size_t(-1);

    // (slowly) drag the highest active voice index down
    if (m_highest_voice > 0 && m_voice[m_highest_voice - 1] == nullptr)
    {
        m_highest_voice--;
    }

    for (size_t i = 0; i < cer::max_voice_count; ++i)
    {
        if (m_voice[i] == nullptr)
        {
            m_highest_voice = std::max(i + 1, m_highest_voice);
            return i;
        }

        if (!m_voice[i]->flags.is_protected && m_voice[i]->play_index < lowest_play_index_value)
        {
            lowest_play_index_value = m_voice[i]->play_index;
            lowest_play_index       = i;
        }
    }
    stop_voice_internal(lowest_play_index);
    return lowest_play_index;
}

auto AudioDevice::get_loop_count(SoundHandle voice_handle) -> size_t
{
    lock_audio_mutex_internal();
    const int ch = get_voice_from_handle_internal(voice_handle);
    if (ch == -1)
    {
        unlock_audio_mutex_internal();
        return 0;
    }
    const auto v = m_voice[ch]->loop_count;
    unlock_audio_mutex_internal();
    return v;
}

// Returns current backend channel count (1 mono, 2 stereo, etc)
auto AudioDevice::backend_channels() const -> size_t
{
    return m_channels;
}

// Returns current backend sample rate
auto AudioDevice::backend_sample_rate() const -> size_t
{
    return m_sample_rate;
}

// Returns current backend buffer size
auto AudioDevice::backend_buffer_size() const -> size_t
{
    return m_buffer_size;
}

// Get speaker position in 3d space
auto AudioDevice::speaker_position(size_t channel) const -> Vector3
{
    return m_3d_speaker_position.at(channel);
}

void AudioDevice::set_post_clip_scaler(float scaler)
{
    m_post_clip_scaler = scaler;
}

void AudioDevice::set_main_resampler(Resampler resampler)
{
    m_resampler = resampler;
}

void AudioDevice::set_global_volume(float volume)
{
    m_global_volume_fader.active = 0;
    m_global_volume              = volume;
}

void AudioDevice::set_relative_play_speed(SoundHandle voice_handle, float speed)
{
    foreach_voice(voice_handle, [this, speed](int ch) {
        m_voice[ch]->relative_play_speed_fader.active = 0;
        set_voice_relative_play_speed_internal(ch, speed);
    });
}

void AudioDevice::set_sample_rate(SoundHandle voice_handle, float sample_rate)
{
    foreach_voice(voice_handle, [this, sample_rate](int ch) {
        m_voice[ch]->base_sample_rate = sample_rate;
        update_voice_relative_play_speed_internal(ch);
    });
}

void AudioDevice::set_pause(SoundHandle voice_handle, bool pause)
{
    foreach_voice(voice_handle, [this, pause](int ch) {
        set_voice_pause_internal(ch, pause);
    });
}

void AudioDevice::set_max_active_voice_count(size_t voice_count)
{
    assert(voice_count > 0);
    assert(voice_count <= cer::max_voice_count);

    lock_audio_mutex_internal();
    m_max_active_voices = voice_count;

    m_resample_data.resize(voice_count * 2);
    m_resample_data_owner.resize(voice_count);

    m_resample_data_buffer =
        AlignedFloatBuffer{sample_granularity * max_channels * voice_count * 2};

    for (size_t i = 0; i < voice_count * 2; ++i)
    {
        m_resample_data[i] =
            m_resample_data_buffer.data() + (sample_granularity * max_channels * i);
    }

    for (size_t i = 0; i < voice_count; ++i)
    {
        m_resample_data_owner[i] = nullptr;
    }

    m_active_voice_dirty = true;
    unlock_audio_mutex_internal();
}

void AudioDevice::set_pause_all(bool pause)
{
    lock_audio_mutex_internal();
    for (size_t ch = 0; ch < m_highest_voice; ++ch)
    {
        set_voice_pause_internal(ch, pause);
    }
    unlock_audio_mutex_internal();
}

void AudioDevice::set_protect_voice(SoundHandle voice_handle, bool protect)
{
    foreach_voice(voice_handle, [this, protect](int ch) {
        m_voice[ch]->flags.is_protected = protect;
    });
}

void AudioDevice::set_pan(SoundHandle voice_handle, float pan)
{
    foreach_voice(voice_handle, [this, pan](int ch) {
        set_voice_pan_internal(ch, pan);
    });
}

void AudioDevice::set_channel_volume(SoundHandle voice_handle, size_t channel, float volume)
{
    foreach_voice(voice_handle, [this, channel, volume](int ch) {
        auto& voice = m_voice[ch];
        if (voice->channel_count > channel)
        {
            voice->channel_volume[channel] = volume;
        }
    });
}

void AudioDevice::set_pan_absolute(SoundHandle voice_handle, float left_volume, float right_volume)
{
    const auto center_volume = (left_volume + right_volume) / 2.0f;

    foreach_voice(voice_handle, [&](int ch) {
        auto& voice   = *m_voice[ch];
        auto& volumes = voice.channel_volume;

        voice.pan_fader.active = 0;

        volumes[0] = left_volume;
        volumes[1] = right_volume;

        switch (voice.channel_count)
        {
            case 4:
                volumes[2] = left_volume;
                volumes[3] = right_volume;
                break;
            case 6:
                volumes[2] = center_volume;
                volumes[3] = center_volume;
                volumes[4] = left_volume;
                volumes[5] = right_volume;
                break;
            case 8:
                volumes[2] = center_volume;
                volumes[3] = center_volume;
                volumes[4] = left_volume;
                volumes[5] = right_volume;
                volumes[6] = left_volume;
                volumes[7] = right_volume;
                break;
            default: break;
        }
    });
}

void AudioDevice::set_inaudible_behavior(SoundHandle voice_handle, bool must_tick, bool kill)
{
    foreach_voice(voice_handle, [this, must_tick, kill](int ch) {
        auto& voice                = *m_voice[ch];
        voice.flags.inaudible_kill = kill;
        voice.flags.inaudible_tick = must_tick;
    });
}

void AudioDevice::set_loop_point(SoundHandle voice_handle, SoundTime loop_point)
{
    foreach_voice(voice_handle, [this, loop_point](int ch) {
        m_voice[ch]->loop_point = loop_point;
    });
}

void AudioDevice::set_looping(SoundHandle voice_handle, bool looping)
{
    foreach_voice(voice_handle, [this, looping](int ch) {
        m_voice[ch]->flags.loops = looping;
    });
}

void AudioDevice::set_auto_stop(SoundHandle voice_handle, bool auto_stop)
{
    foreach_voice(voice_handle, [this, auto_stop](int ch) {
        m_voice[ch]->flags.disable_autostop = !auto_stop;
    });
}

void AudioDevice::set_volume(SoundHandle voice_handle, float volume)
{
    foreach_voice(voice_handle, [this, volume](int ch) {
        m_voice[ch]->volume_fader.active = 0;
        set_voice_volume_internal(ch, volume);
    });
}

void AudioDevice::set_delay_samples(SoundHandle voice_handle, size_t samples)
{
    foreach_voice(voice_handle, [this, samples](int ch) {
        m_voice[ch]->delay_samples = samples;
    });
}

void AudioDevice::set_visualization_enable(bool enable)
{
    m_flags.enable_visualization = enable;
}

void AudioDevice::speaker_position(size_t channel, Vector3 value)
{
    m_3d_speaker_position.at(channel) = value;
}

struct mat3 : std::array<Vector3, 3>
{
};

static auto operator*(const mat3& m, const Vector3& a) -> Vector3
{
    return {
        (m[0].x * a.x) + m[0].y * a.y + m[0].z * a.z,
        (m[1].x * a.x) + m[1].y * a.y + m[1].z * a.z,
        (m[2].x * a.x) + m[2].y * a.y + m[2].z * a.z,
    };
}

static auto look_at(const Vector3& at, Vector3 up) -> mat3
{
    const auto z = normalize(at);
    const auto x = normalize(cross(up, z));
    const auto y = cross(z, x);

    return {x, y, z};
}

auto doppler(Vector3        aDeltaPos,
             const Vector3& aSrcVel,
             const Vector3& aDstVel,
             float          aFactor,
             float          aSoundSpeed) -> float
{
    const float deltamag = length(aDeltaPos);
    if (deltamag == 0)
    {
        return 1.0f;
    }

    float vls      = dot(aDeltaPos, aDstVel) / deltamag;
    float vss      = dot(aDeltaPos, aSrcVel) / deltamag;
    float maxspeed = aSoundSpeed / aFactor;
    vss            = min(vss, maxspeed);
    vls            = min(vls, maxspeed);

    return (aSoundSpeed - aFactor * vls) / (aSoundSpeed - aFactor * vss);
}

auto attenuateInvDistance(float distance,
                          float min_distance,
                          float max_distance,
                          float rolloff_factor) -> float
{
    distance = clamp(distance, min_distance, max_distance);

    return min_distance / (min_distance + rolloff_factor * (distance - min_distance));
}

auto attenuateLinearDistance(float distance,
                             float min_distance,
                             float max_distance,
                             float rolloff_factor) -> float
{
    distance = clamp(distance, min_distance, max_distance);

    return 1.0f - rolloff_factor * (distance - min_distance) / (max_distance - min_distance);
}

auto attenuate_exponential_distance(float distance,
                                    float min_distance,
                                    float max_distance,
                                    float rolloff_factor) -> float
{
    distance = clamp(distance, min_distance, max_distance);

    return pow(distance / min_distance, -rolloff_factor);
}

static auto attenuation_factor(const AudioSourceInstance3dData& v, float distance) -> float
{
    switch (v.attenuation_model_3d)
    {
        case AttenuationModel::InverseDistance:
            return attenuateInvDistance(distance,
                                        v.min_distance_3d,
                                        v.max_distance_3d,
                                        v.attenuation_rolloff_3d);
        case AttenuationModel::LinearDistance:
            return attenuateLinearDistance(distance,
                                           v.min_distance_3d,
                                           v.max_distance_3d,
                                           v.attenuation_rolloff_3d);
        case AttenuationModel::ExponentialDistance:
            return attenuate_exponential_distance(distance,
                                                  v.min_distance_3d,
                                                  v.max_distance_3d,
                                                  v.attenuation_rolloff_3d);
        default: return 1.0f;
    }
}

void AudioDevice::update_3d_voices_internal(std::span<const size_t> voice_list)
{
    auto speaker = std::array<Vector3, max_channels>{};

    for (size_t i = 0; i < m_channels; ++i)
    {
        speaker[i] = normalize(m_3d_speaker_position[i]);
    }

    const auto m = look_at(m_3d_at, m_3d_up);

    for (const auto voice_id : voice_list)
    {
        auto& v = m_3d_data[voice_id];

        auto vol = v.collider != nullptr ? v.collider->collide(this, v, v.collider_data) : 1.0f;
        auto pos = v.position_3d;
        const auto vel = v.velocity_3d;

        if (!v.flags.listener_relative)
        {
            pos -= m_3d_position;
        }

        const float dist = length(pos);

        if (v.attenuator != nullptr)
        {
            vol *= v.attenuator->attenuate(dist,
                                           v.min_distance_3d,
                                           v.max_distance_3d,
                                           v.attenuation_rolloff_3d);
        }
        else
        {
            vol *= attenuation_factor(v, dist);
        }

        // doppler
        v.doppler_value = doppler(pos, vel, m_3d_velocity, v.doppler_factor_3d, m_3d_sound_speed);

        // panning
        pos = normalize(m * pos);

        v.channel_volume = {};

        // Apply volume to channels based on speaker vectors
        for (size_t j = 0; j < m_channels; ++j)
        {
            const auto sp             = speaker[j];
            const auto speaker_volume = is_zero(sp) ? 1.0f : (dot(sp, pos) + 1) / 2;
            v.channel_volume[j]       = vol * speaker_volume;
        }

        v.volume_3d = vol;
    }
}

void AudioDevice::update_3d_audio()
{
    auto voice_count = size_t(0);
    auto voices      = std::array<size_t, max_voice_count>{};

    // Step 1 - find voices that need 3d processing
    lock_audio_mutex_internal();
    for (size_t i = 0; i < m_highest_voice; ++i)
    {
        const auto& voice = m_voice[i];
        if (voice != nullptr && voice->flags.process_3d)
        {
            voices[voice_count] = i;
            ++voice_count;
            m_3d_data[i].flags = voice->flags;
        }
    }
    unlock_audio_mutex_internal();

    // Step 2 - do 3d processing

    update_3d_voices_internal({voices.data(), voice_count});

    // Step 3 - update SoLoud voices

    lock_audio_mutex_internal();
    for (size_t i = 0; i < voice_count; ++i)
    {
        auto& v = m_3d_data[voices[i]];

        if (const auto& vi = m_voice[voices[i]])
        {
            update_voice_relative_play_speed_internal(voices[i]);
            update_voice_volume_internal(voices[i]);

            for (size_t j = 0; j < max_channels; ++j)
            {
                vi->channel_volume[j] = v.channel_volume[j];
            }

            if (vi->overall_volume < 0.001f)
            {
                // Inaudible.
                vi->flags.inaudible = true;

                if (vi->flags.inaudible_kill)
                {
                    stop_voice_internal(voices[i]);
                }
            }
            else
            {
                vi->flags.inaudible = false;
            }
        }
    }

    m_active_voice_dirty = true;
    unlock_audio_mutex_internal();
}


auto AudioDevice::play_3d(
    AudioSource& sound, Vector3 pos, Vector3 vel, float volume, bool paused, size_t bus)
    -> SoundHandle
{
    const SoundHandle h = play(sound, volume, 0, true, bus);
    lock_audio_mutex_internal();
    const auto v = get_voice_from_handle_internal(h);

    if (v < 0)
    {
        unlock_audio_mutex_internal();
        return h;
    }

    m_3d_data[v].handle          = h;
    m_voice[v]->flags.process_3d = true;

    set_3d_source_parameters(h, pos, vel);

    int samples = 0;
    if (sound.distance_delay)
    {
        const auto corrected_pos = m_voice[v]->flags.listener_relative ? pos : pos - m_3d_position;

        const float dist = length(corrected_pos);
        samples += int(floor(dist / m_3d_sound_speed * float(m_sample_rate)));
    }

    update_3d_voices_internal({reinterpret_cast<const size_t*>(&v), 1});
    update_voice_relative_play_speed_internal(v);

    for (size_t j = 0; j < max_channels; ++j)
    {
        m_voice[v]->channel_volume[j] = m_3d_data[v].channel_volume[j];
    }

    update_voice_volume_internal(v);

    // Fix initial voice volume ramp up
    for (size_t i = 0; i < max_channels; ++i)
    {
        m_voice[v]->current_channel_volume[i] =
            m_voice[v]->channel_volume[i] * m_voice[v]->overall_volume;
    }

    if (m_voice[v]->overall_volume < 0.01f)
    {
        // Inaudible.
        m_voice[v]->flags.inaudible = true;

        if (m_voice[v]->flags.inaudible_kill)
        {
            stop_voice_internal(v);
        }
    }
    else
    {
        m_voice[v]->flags.inaudible = false;
    }

    m_active_voice_dirty = true;

    unlock_audio_mutex_internal();
    set_delay_samples(h, samples);
    set_pause(h, paused);

    return h;
}

auto AudioDevice::play3d_clocked(
    SoundTime sound_time, AudioSource& sound, Vector3 pos, Vector3 vel, float volume, size_t bus)
    -> SoundHandle
{
    const SoundHandle h = play(sound, volume, 0, 1, bus);
    lock_audio_mutex_internal();
    int v = get_voice_from_handle_internal(h);
    if (v < 0)
    {
        unlock_audio_mutex_internal();
        return h;
    }
    m_3d_data[v].handle          = h;
    m_voice[v]->flags.process_3d = true;
    set_3d_source_parameters(h, pos, vel);
    SoundTime lasttime = m_last_clocked_time;
    if (lasttime == 0)
    {
        lasttime            = sound_time;
        m_last_clocked_time = sound_time;
    }
    unlock_audio_mutex_internal();

    auto samples = int(floor((sound_time - lasttime) * m_sample_rate));

    // Make sure we don't delay too much (or overflow)
    if (samples < 0 || samples > 2048)
    {
        samples = 0;
    }

    if (sound.distance_delay)
    {
        const float dist = length(pos);
        samples += int(floor((dist / m_3d_sound_speed) * m_sample_rate));
    }

    update_3d_voices_internal({reinterpret_cast<size_t*>(&v), 1});
    lock_audio_mutex_internal();
    update_voice_relative_play_speed_internal(v);

    for (size_t j = 0; j < max_channels; ++j)
    {
        m_voice[v]->channel_volume[j] = m_3d_data[v].channel_volume[j];
    }

    update_voice_volume_internal(v);

    // Fix initial voice volume ramp up
    for (size_t i = 0; i < max_channels; ++i)
    {
        m_voice[v]->current_channel_volume[i] =
            m_voice[v]->channel_volume[i] * m_voice[v]->overall_volume;
    }

    if (m_voice[v]->overall_volume < 0.01f)
    {
        // Inaudible.
        m_voice[v]->flags.inaudible = true;

        if (m_voice[v]->flags.inaudible_kill)
        {
            stop_voice_internal(v);
        }
    }
    else
    {
        m_voice[v]->flags.inaudible = false;
    }

    m_active_voice_dirty = true;
    unlock_audio_mutex_internal();

    set_delay_samples(h, samples);
    set_pause(h, false);

    return h;
}

void AudioDevice::set_3d_sound_speed(float speed)
{
    assert(speed > 0.0f);
    m_3d_sound_speed = speed;
}

auto AudioDevice::get_3d_sound_speed() const -> float
{
    return m_3d_sound_speed;
}

void AudioDevice::set_3d_listener_parameters(Vector3 pos, Vector3 at, Vector3 up, Vector3 velocity)
{
    m_3d_position = pos;
    m_3d_at       = at;
    m_3d_up       = up;
    m_3d_velocity = velocity;
}

void AudioDevice::set_3d_listener_position(Vector3 value)
{
    m_3d_position = value;
}

void AudioDevice::set_3d_listener_at(Vector3 value)
{
    m_3d_at = value;
}

void AudioDevice::set_3d_listener_up(Vector3 value)
{
    m_3d_up = value;
}

void AudioDevice::set_3d_listener_velocity(Vector3 value)
{
    m_3d_velocity = value;
}

void AudioDevice::set_3d_source_parameters(SoundHandle voice_handle,
                                           Vector3     aPos,
                                           Vector3     aVelocity)
{
    foreach_voice_3d(voice_handle, [&](int ch) {
        m_3d_data[ch].position_3d = aPos;
        m_3d_data[ch].velocity_3d = aVelocity;
    });
}


void AudioDevice::set_3d_source_position(SoundHandle voice_handle, Vector3 value)
{
    foreach_voice_3d(voice_handle, [&](int ch) {
        m_3d_data[ch].position_3d = value;
    });
}


void AudioDevice::set_3d_source_velocity(SoundHandle voice_handle, Vector3 velocity)
{
    foreach_voice_3d(voice_handle, [&](int ch) {
        m_3d_data[ch].velocity_3d = velocity;
    });
}


void AudioDevice::set_3d_source_min_max_distance(SoundHandle voice_handle,
                                                 float       aMinDistance,
                                                 float       aMaxDistance)
{
    foreach_voice_3d(voice_handle, [&](int ch) {
        m_3d_data[ch].min_distance_3d = aMinDistance;
        m_3d_data[ch].max_distance_3d = aMaxDistance;
    });
}


void AudioDevice::set_3d_source_attenuation(SoundHandle      voice_handle,
                                            AttenuationModel aAttenuationModel,
                                            float            aAttenuationRolloffFactor)
{
    foreach_voice_3d(voice_handle, [&](int ch) {
        m_3d_data[ch].attenuation_model_3d   = aAttenuationModel;
        m_3d_data[ch].attenuation_rolloff_3d = aAttenuationRolloffFactor;
    });
}


void AudioDevice::set_3d_source_doppler_factor(SoundHandle voice_handle, float doppler_factor)
{
    foreach_voice_3d(voice_handle, [&](int ch) {
        m_3d_data[ch].doppler_factor_3d = doppler_factor;
    });
}

void AudioDevice::set_global_filter(size_t aFilterId, Filter* aFilter)
{
    if (aFilterId >= filters_per_stream)
    {
        return;
    }

    lock_audio_mutex_internal();

    m_filter[aFilterId] = aFilter;

    if (aFilter != nullptr)
    {
        m_filter_instance[aFilterId] = m_filter[aFilterId]->create_instance();
    }

    unlock_audio_mutex_internal();
}

auto AudioDevice::filter_parameter(SoundHandle voice_handle, size_t filter_id, size_t attribute_id)
    -> Option<float>
{
    if (filter_id >= filters_per_stream)
    {
        return {};
    }

    auto ret = Option<float>{};

    if (voice_handle == 0)
    {
        lock_audio_mutex_internal();
        if (m_filter_instance[filter_id])
        {
            ret = m_filter_instance[filter_id]->filter_parameter(attribute_id);
        }
        unlock_audio_mutex_internal();
        return ret;
    }

    const int ch = get_voice_from_handle_internal(voice_handle);
    if (ch == -1)
    {
        return ret;
    }

    lock_audio_mutex_internal();
    if (m_voice[ch] && m_voice[ch]->filter[filter_id])
    {
        ret = m_voice[ch]->filter[filter_id]->filter_parameter(attribute_id);
    }
    unlock_audio_mutex_internal();

    return ret;
}

void AudioDevice::set_filter_parameter(SoundHandle voice_handle,
                                       size_t      filter_id,
                                       size_t      attribute_id,
                                       float       value)
{
    if (filter_id >= filters_per_stream)
    {
        return;
    }

    if (voice_handle == 0)
    {
        lock_audio_mutex_internal();
        if (m_filter_instance[filter_id])
        {
            m_filter_instance[filter_id]->set_filter_parameter(attribute_id, value);
        }
        unlock_audio_mutex_internal();
        return;
    }

    foreach_voice(voice_handle, [&](int ch) {
        if (m_voice[ch] && m_voice[ch]->filter[filter_id])
        {
            m_voice[ch]->filter[filter_id]->set_filter_parameter(attribute_id, value);
        }
    });
}

void AudioDevice::fade_filter_parameter(
    SoundHandle voice_handle, size_t aFilterId, size_t attribute_id, float to, double time)
{
    if (aFilterId >= filters_per_stream)
    {
        return;
    }

    if (voice_handle == 0)
    {
        lock_audio_mutex_internal();
        if (m_filter_instance[aFilterId])
        {
            m_filter_instance[aFilterId]->fade_filter_parameter(attribute_id,
                                                                to,
                                                                time,
                                                                m_stream_time);
        }
        unlock_audio_mutex_internal();
        return;
    }

    foreach_voice(voice_handle, [&](int ch) {
        if (m_voice[ch] && m_voice[ch]->filter[aFilterId])
        {
            m_voice[ch]->filter[aFilterId]->fade_filter_parameter(attribute_id,
                                                                  to,
                                                                  time,
                                                                  m_stream_time);
        }
    });
}

void AudioDevice::oscillate_filter_parameter(SoundHandle voice_handle,
                                             size_t      filter_id,
                                             size_t      attribute_id,
                                             float       from,
                                             float       to,
                                             double      time)
{
    if (filter_id >= filters_per_stream)
    {
        return;
    }

    if (voice_handle == 0)
    {
        lock_audio_mutex_internal();
        if (m_filter_instance[filter_id])
        {
            m_filter_instance[filter_id]->oscillate_filter_parameter(attribute_id,
                                                                     from,
                                                                     to,
                                                                     time,
                                                                     m_stream_time);
        }
        unlock_audio_mutex_internal();
        return;
    }

    foreach_voice(voice_handle, [&](int ch) {
        if (const auto& voice = m_voice[ch]; voice != nullptr && voice->filter[filter_id])
        {
            voice->filter[filter_id]->oscillate_filter_parameter(attribute_id,
                                                                 from,
                                                                 to,
                                                                 time,
                                                                 m_stream_time);
        }
    });
}

// Create a voice group. Returns 0 if unable (out of voice groups / out of memory)
auto AudioDevice::create_voice_group() -> SoundHandle
{
    lock_audio_mutex_internal();

    size_t i = 0;

    // Check if there's any deleted voice groups and re-use if found
    for (i = 0; i < m_voice_group_count; ++i)
    {
        auto& group = m_voice_group[i];

        if (group == nullptr)
        {
            group    = new size_t[17];
            group[0] = 16;
            group[1] = 0;
            unlock_audio_mutex_internal();
            return 0xfffff000 | i;
        }
    }

    if (m_voice_group_count == 4096)
    {
        unlock_audio_mutex_internal();
        return 0;
    }

    const auto old_count = m_voice_group_count;
    if (m_voice_group_count == 0)
    {
        m_voice_group_count = 4;
    }

    m_voice_group_count *= 2;

    auto** vg = new size_t*[m_voice_group_count];

    for (i = 0; i < old_count; ++i)
    {
        vg[i] = m_voice_group[i];
    }

    for (; i < m_voice_group_count; ++i)
    {
        vg[i] = nullptr;
    }

    delete[] m_voice_group;
    m_voice_group = vg;
    i             = old_count;

    m_voice_group[i]    = new size_t[17];
    m_voice_group[i][0] = 16;
    m_voice_group[i][1] = 0;

    unlock_audio_mutex_internal();

    return 0xfffff000 | i;
}

// Destroy a voice group.
void AudioDevice::destroy_voice_group(SoundHandle voice_group_handle)
{
    if (!is_voice_group(voice_group_handle))
    {
        return;
    }

    const int c = voice_group_handle & 0xfff;

    lock_audio_mutex_internal();
    delete[] m_voice_group[c];
    m_voice_group[c] = nullptr;
    unlock_audio_mutex_internal();
}

// Add a voice handle to a voice group
void AudioDevice::add_voice_to_group(SoundHandle aVoiceGroupHandle, SoundHandle voice_handle)
{
    if (!is_voice_group(aVoiceGroupHandle))
        return;

    // Don't consider adding invalid voice handles as an error, since the voice may just have ended.
    if (!is_valid_voice_handle(voice_handle))
        return;

    trim_voice_group_internal(aVoiceGroupHandle);

    const int c = aVoiceGroupHandle & 0xfff;

    lock_audio_mutex_internal();

    for (size_t i = 1; i < m_voice_group[c][0]; ++i)
    {
        if (m_voice_group[c][i] == voice_handle)
        {
            unlock_audio_mutex_internal();
            return; // already there
        }

        if (m_voice_group[c][i] == 0)
        {
            m_voice_group[c][i]     = voice_handle;
            m_voice_group[c][i + 1] = 0;

            unlock_audio_mutex_internal();
            return;
        }
    }

    // Full group, allocate more memory
    const auto n = new size_t[m_voice_group[c][0] * 2 + 1];

    for (size_t i = 0; i < m_voice_group[c][0]; ++i)
    {
        n[i] = m_voice_group[c][i];
    }

    n[n[0]]     = voice_handle;
    n[n[0] + 1] = 0;
    n[0] *= 2;
    delete[] m_voice_group[c];
    m_voice_group[c] = n;
    unlock_audio_mutex_internal();
}

// Is this handle a valid voice group?
bool AudioDevice::is_voice_group(SoundHandle voice_group_handle)
{
    if ((voice_group_handle & 0xfffff000) != 0xfffff000)
    {
        return false;
    }

    const size_t c = voice_group_handle & 0xfff;
    if (c >= m_voice_group_count)
    {
        return false;
    }

    lock_audio_mutex_internal();
    const bool res = m_voice_group[c] != nullptr;
    unlock_audio_mutex_internal();

    return res;
}

// Is this voice group empty?
bool AudioDevice::is_voice_group_empty(SoundHandle voice_group_handle)
{
    // If not a voice group, yeah, we're empty alright..
    if (!is_voice_group(voice_group_handle))
    {
        return true;
    }

    trim_voice_group_internal(voice_group_handle);
    const int c = voice_group_handle & 0xfff;

    lock_audio_mutex_internal();
    const bool res = m_voice_group[c][1] == 0;
    unlock_audio_mutex_internal();

    return res;
}

// Remove all non-active voices from group
void AudioDevice::trim_voice_group_internal(SoundHandle voice_group_handle)
{
    if (!is_voice_group(voice_group_handle))
    {
        return;
    }

    const int c = voice_group_handle & 0xfff;

    lock_audio_mutex_internal();

    // empty group
    if (m_voice_group[c][1] == 0)
    {
        unlock_audio_mutex_internal();
        return;
    }

    // first item in voice group is number of allocated indices
    for (size_t i = 1; i < m_voice_group[c][0]; ++i)
    {
        // If we hit a voice in the group that's not set, we're done
        if (m_voice_group[c][i] == 0)
        {
            unlock_audio_mutex_internal();
            return;
        }

        unlock_audio_mutex_internal();
        while (!is_valid_voice_handle(
            m_voice_group[c][i])) // function locks mutex, so we need to unlock it before the call
        {
            lock_audio_mutex_internal();
            // current index is an invalid handle, move all following handles backwards
            for (size_t j = i; j < m_voice_group[c][0] - 1; ++j)
            {
                m_voice_group[c][j] = m_voice_group[c][j + 1];
                // not a full group, we can stop copying
                if (m_voice_group[c][j] == 0)
                    break;
            }
            // be sure to mark the last one as unused in any case
            m_voice_group[c][m_voice_group[c][0] - 1] = 0;
            // did we end up with an empty group? we're done then
            if (m_voice_group[c][i] == 0)
            {
                unlock_audio_mutex_internal();
                return;
            }
            unlock_audio_mutex_internal();
        }
        lock_audio_mutex_internal();
    }
    unlock_audio_mutex_internal();
}

auto AudioDevice::voice_group_handle_to_array_internal(SoundHandle aVoiceGroupHandle) const
    -> SoundHandle*
{
    if ((aVoiceGroupHandle & 0xfffff000) != 0xfffff000)
    {
        return nullptr;
    }

    const size_t c = aVoiceGroupHandle & 0xfff;
    if (c >= m_voice_group_count)
    {
        return nullptr;
    }

    if (m_voice_group[c] == nullptr)
    {
        return nullptr;
    }

    return m_voice_group[c] + 1;
}

void AudioDevice::set_voice_relative_play_speed_internal(size_t aVoice, float aSpeed)
{
    assert(aVoice < cer::max_voice_count);
    assert(m_inside_audio_thread_mutex);
    assert(aSpeed > 0.0f);

    if (m_voice[aVoice])
    {
        m_voice[aVoice]->set_relative_play_speed = aSpeed;
        update_voice_relative_play_speed_internal(aVoice);
    }
}

void AudioDevice::set_voice_pause_internal(size_t aVoice, bool aPause)
{
    assert(aVoice < cer::max_voice_count);
    assert(m_inside_audio_thread_mutex);
    m_active_voice_dirty = true;

    if (m_voice[aVoice])
    {
        m_voice[aVoice]->pause_scheduler.active = 0;
        m_voice[aVoice]->flags.is_paused        = aPause;
    }
}

void AudioDevice::set_voice_pan_internal(size_t aVoice, float aPan)
{
    assert(aVoice < cer::max_voice_count);
    assert(m_inside_audio_thread_mutex);
    if (m_voice[aVoice])
    {
        m_voice[aVoice]->pan               = aPan;
        const auto l                       = float(std::cos((aPan + 1) * cer::pi / 4));
        const auto r                       = float(std::sin((aPan + 1) * cer::pi / 4));
        m_voice[aVoice]->channel_volume[0] = l;
        m_voice[aVoice]->channel_volume[1] = r;
        if (m_voice[aVoice]->channel_count == 4)
        {
            m_voice[aVoice]->channel_volume[2] = l;
            m_voice[aVoice]->channel_volume[3] = r;
        }
        if (m_voice[aVoice]->channel_count == 6)
        {
            m_voice[aVoice]->channel_volume[2] = 1.0f / std::sqrt(2.0f);
            m_voice[aVoice]->channel_volume[3] = 1;
            m_voice[aVoice]->channel_volume[4] = l;
            m_voice[aVoice]->channel_volume[5] = r;
        }
        if (m_voice[aVoice]->channel_count == 8)
        {
            m_voice[aVoice]->channel_volume[2] = 1.0f / std::sqrt(2.0f);
            m_voice[aVoice]->channel_volume[3] = 1;
            m_voice[aVoice]->channel_volume[4] = l;
            m_voice[aVoice]->channel_volume[5] = r;
            m_voice[aVoice]->channel_volume[6] = l;
            m_voice[aVoice]->channel_volume[7] = r;
        }
    }
}

void AudioDevice::set_voice_volume_internal(size_t aVoice, float aVolume)
{
    assert(aVoice < cer::max_voice_count);
    assert(m_inside_audio_thread_mutex);
    m_active_voice_dirty = true;
    if (m_voice[aVoice])
    {
        m_voice[aVoice]->set_volume = aVolume;
        update_voice_volume_internal(aVoice);
    }
}

void AudioDevice::stop_voice_internal(size_t aVoice)
{
    assert(aVoice < cer::max_voice_count);
    assert(m_inside_audio_thread_mutex);
    m_active_voice_dirty = true;
    if (m_voice[aVoice])
    {
        // Delete via temporary variable to avoid recursion
        auto v = m_voice[aVoice];
        m_voice[aVoice].reset();

        for (size_t i = 0; i < m_max_active_voices; ++i)
        {
            if (m_resample_data_owner[i].get() == v.get())
            {
                m_resample_data_owner[i].reset();
            }
        }
    }
}

void AudioDevice::update_voice_relative_play_speed_internal(size_t aVoice)
{
    assert(aVoice < cer::max_voice_count);
    assert(m_inside_audio_thread_mutex);

    m_voice[aVoice]->overall_relative_play_speed =
        m_3d_data[aVoice].doppler_value * m_voice[aVoice]->set_relative_play_speed;

    m_voice[aVoice]->sample_rate =
        m_voice[aVoice]->base_sample_rate * m_voice[aVoice]->overall_relative_play_speed;
}

void AudioDevice::update_voice_volume_internal(size_t aVoice)
{
    assert(aVoice < cer::max_voice_count);
    assert(m_inside_audio_thread_mutex);
    m_voice[aVoice]->overall_volume = m_voice[aVoice]->set_volume * m_3d_data[aVoice].volume_3d;

    if (m_voice[aVoice]->flags.is_paused)
    {
        for (size_t i = 0; i < max_channels; ++i)
        {
            m_voice[aVoice]->current_channel_volume[i] =
                m_voice[aVoice]->channel_volume[i] * m_voice[aVoice]->overall_volume;
        }
    }
}
} // namespace cer
