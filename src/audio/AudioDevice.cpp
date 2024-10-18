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
AudioDevice::AudioDevice(EngineFlags           flags,
                         std::optional<size_t> sample_rate,
                         std::optional<size_t> buffer_size,
                         size_t                channels)
    : m_flags(flags)
{
    assert(channels != 3 && channels != 5 && channels != 7);
    assert(channels <= max_channels);

    m_audio_thread_mutex = thread::create_mutex();

    sample_rate = sample_rate.value_or(44100);
    buffer_size = buffer_size.value_or(2048);

#if defined(WITH_SDL2_STATIC)
    {
        if (!aBufferSize.has_value())
            buffersize = 2048;

        sdl2static_init(this, flags, samplerate, buffersize, channels);
    }
#endif

#if defined(WITH_XAUDIO2)
    {
        if (!aBufferSize.has_value())
            buffersize = 4096;

        xaudio2_init(this, flags, samplerate, buffersize, channels);
    }
#endif

#if defined(WITH_WINMM)
    {
        if (!buffer_size.has_value())
            buffer_size = 4096;

        winmm_init(this, flags, sample_rate, buffer_size, channels);
    }
#endif

#if defined(WITH_WASAPI)
    {
        if (!aBufferSize.has_value())
            buffersize = 4096;

        if (!aSamplerate.has_value())
            samplerate = 48000;

        wasapi_init(this, flags, samplerate, buffersize, channels);
    }
#endif

#if defined(WITH_ALSA)
    {
        if (!buffer_size.has_value())
            buffer_size = 2048;

        alsa_init(AudioBackendArgs{
            .engine        = this,
            .flags         = flags,
            .sample_rate   = *sample_rate,
            .buffer        = *buffer_size,
            .channel_count = channels,
        });
    }
#endif

#if defined(WITH_COREAUDIO)
    {
        if (!buffer_size.has_value())
        {
            *buffer_size = 2048;
        }

        coreaudio_init(AudioBackendArgs{
            .engine        = this,
            .flags         = flags,
            .sample_rate   = *sample_rate,
            .buffer        = *buffer_size,
            .channel_count = channels,
        });
    }
#endif

#if defined(WITH_OPENSLES)
    {
        if (!aBufferSize.has_value())
            buffersize = 4096;

        opensles_init(this, flags, samplerate, buffersize, channels);
    }
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
    lockAudioMutex_internal();
    unlockAudioMutex_internal();
    assert(!m_inside_audio_thread_mutex);
    stop_all();
    if (m_backend_cleanup_func)
        m_backend_cleanup_func(this);
    m_backend_cleanup_func = 0;
    if (m_audio_thread_mutex)
        thread::destroy_mutex(m_audio_thread_mutex);
    m_audio_thread_mutex = nullptr;

    for (size_t i = 0; i < filters_per_stream; ++i)
    {
        m_filter_instance[i].reset();
    }
}

auto AudioDevice::play_sound(const Sound&             sound,
                             float                    volume,
                             float                    pan,
                             bool                     start_paused,
                             std::optional<SoundTime> delay) -> SoundChannel
{
    if (!sound)
    {
        return {};
    }

    const auto channel_handle =
        delay ? play_clocked(*delay, sound.impl()->audio_source(), volume, pan)
              : play(sound.impl()->audio_source(), volume, pan, start_paused);

    // TODO: Use pool allocation for SoundChannelImpl objects
    auto channel_impl = std::make_unique<details::SoundChannelImpl>(this, channel_handle);

    m_playing_sounds.insert(sound);

    return SoundChannel{channel_impl.release()};
}

void AudioDevice::play_sound_fire_and_forget(const Sound&             sound,
                                             float                    volume,
                                             float                    pan,
                                             std::optional<SoundTime> delay)
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

    setPanAbsolute(channel.id(), 1.0f, 1.0f);

    m_playing_sounds.insert(sound);

    return channel;
}

void AudioDevice::stop_all_sounds()
{
    stop_all();
}

void AudioDevice::pause_all_sounds()
{
    setPauseAll(true);
}

void AudioDevice::resume_all_sounds()
{
    setPauseAll(false);
}

void AudioDevice::set_global_volume(float value)
{
    setGlobalVolume(value);
}

void AudioDevice::fade_global_volume(float to_volume, SoundTime fade_duration)
{
    fadeGlobalVolume(to_volume, fade_duration);
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
    sound.engine  = this;
    auto instance = sound.create_instance();

    lockAudioMutex_internal();
    int ch = findFreeVoice_internal();
    if (ch < 0)
    {
        unlockAudioMutex_internal();
        return 7; // TODO: this was "UNKNOWN_ERROR"
    }
    if (!sound.audio_source_id)
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
        m_voice[ch]->flags.Paused = true;
    }

    setVoicePan_internal(ch, pan);
    if (volume < 0)
    {
        setVoiceVolume_internal(ch, sound.volume);
    }
    else
    {
        setVoiceVolume_internal(ch, volume);
    }

    // Fix initial voice volume ramp up
    for (size_t i = 0; i < max_channels; ++i)
    {
        m_voice[ch]->current_channel_volume[i] =
            m_voice[ch]->channel_volume[i] * m_voice[ch]->overall_volume;
    }

    setVoiceRelativePlaySpeed_internal(ch, 1);

    for (size_t i = 0; i < filters_per_stream; ++i)
    {
        if (sound.filter[i])
        {
            m_voice[ch]->filter[i] = sound.filter[i]->create_instance();
        }
    }

    m_active_voice_dirty = true;

    unlockAudioMutex_internal();

    return getHandleFromVoice_internal(ch);
}

SoundHandle AudioDevice::play_clocked(
    SoundTime sound_time, AudioSource& sound, float volume, float pan, size_t bus)
{
    const SoundHandle h = play(sound, volume, pan, 1, bus);
    lockAudioMutex_internal();
    // mLastClockedTime is cleared to zero at start of every output buffer
    SoundTime lasttime = m_last_clocked_time;
    if (lasttime == 0)
    {
        m_last_clocked_time = sound_time;
        lasttime            = sound_time;
    }
    unlockAudioMutex_internal();
    int samples = (int)floor((sound_time - lasttime) * m_samplerate);
    // Make sure we don't delay too much (or overflow)
    if (samples < 0 || samples > 2048)
        samples = 0;
    setDelaySamples(h, samples);
    setPause(h, false);
    return h;
}

SoundHandle AudioDevice::play3d_background(AudioSource& sound,
                                           float        volume,
                                           bool         paused,
                                           size_t       bus)
{
    const SoundHandle h = play(sound, volume, 0.0f, paused, bus);
    setPanAbsolute(h, 1.0f, 1.0f);
    return h;
}

bool AudioDevice::seek(SoundHandle voice_handle, SoundTime seconds)
{
    bool res = true;

    FOR_ALL_VOICES_PRE
    const auto singleres = m_voice[ch]->seek(seconds, m_scratch.data(), m_scratch_size);
    if (!singleres)
        res = singleres;
    FOR_ALL_VOICES_POST
    return res;
}


void AudioDevice::stop(SoundHandle voice_handle)
{
    FOR_ALL_VOICES_PRE
    stopVoice_internal(ch);
    FOR_ALL_VOICES_POST
}

void AudioDevice::stop_audio_source(AudioSource& aSound)
{
    if (aSound.audio_source_id)
    {
        lockAudioMutex_internal();

        for (size_t i = 0; i < m_highest_voice; ++i)
        {
            if (m_voice[i] && m_voice[i]->audio_source_id == aSound.audio_source_id)
            {
                stopVoice_internal(i);
            }
        }
        unlockAudioMutex_internal();
    }
}

void AudioDevice::stop_all()
{
    lockAudioMutex_internal();
    for (size_t i = 0; i < m_highest_voice; ++i)
    {
        stopVoice_internal(i);
    }
    unlockAudioMutex_internal();
}

int AudioDevice::count_audio_source(AudioSource& aSound)
{
    int count = 0;
    if (aSound.audio_source_id)
    {
        lockAudioMutex_internal();

        for (size_t i = 0; i < m_highest_voice; ++i)
        {
            if (m_voice[i] && m_voice[i]->audio_source_id == aSound.audio_source_id)
            {
                count++;
            }
        }
        unlockAudioMutex_internal();
    }
    return count;
}

void AudioDevice::schedulePause(SoundHandle voice_handle, SoundTime aTime)
{
    if (aTime <= 0)
    {
        setPause(voice_handle, 1);
        return;
    }
    FOR_ALL_VOICES_PRE
    m_voice[ch]->pause_scheduler.set(1, 0, aTime, m_voice[ch]->stream_time);
    FOR_ALL_VOICES_POST
}

void AudioDevice::scheduleStop(SoundHandle voice_handle, SoundTime aTime)
{
    if (aTime <= 0)
    {
        stop(voice_handle);
        return;
    }
    FOR_ALL_VOICES_PRE
    m_voice[ch]->stop_scheduler.set(1, 0, aTime, m_voice[ch]->stream_time);
    FOR_ALL_VOICES_POST
}

void AudioDevice::fadeVolume(SoundHandle voice_handle, float aTo, SoundTime aTime)
{
    float from = volume(voice_handle);
    if (aTime <= 0 || aTo == from)
    {
        setVolume(voice_handle, aTo);
        return;
    }

    FOR_ALL_VOICES_PRE
    m_voice[ch]->volume_fader.set(from, aTo, aTime, m_voice[ch]->stream_time);
    FOR_ALL_VOICES_POST
}

void AudioDevice::fadePan(SoundHandle voice_handle, float aTo, SoundTime aTime)
{
    const float from = pan(voice_handle);

    if (aTime <= 0 || aTo == from)
    {
        setPan(voice_handle, aTo);
        return;
    }

    FOR_ALL_VOICES_PRE
    m_voice[ch]->pan_fader.set(from, aTo, aTime, m_voice[ch]->stream_time);
    FOR_ALL_VOICES_POST
}

void AudioDevice::fadeRelativePlaySpeed(SoundHandle voice_handle, float aTo, SoundTime aTime)
{
    const float from = relative_play_speed(voice_handle);
    if (aTime <= 0 || aTo == from)
    {
        setRelativePlaySpeed(voice_handle, aTo);
        return;
    }
    FOR_ALL_VOICES_PRE
    m_voice[ch]->relative_play_speed_fader.set(from, aTo, aTime, m_voice[ch]->stream_time);
    FOR_ALL_VOICES_POST
}

void AudioDevice::fadeGlobalVolume(float aTo, SoundTime aTime)
{
    const float from = global_volume();
    if (aTime <= 0 || aTo == from)
    {
        setGlobalVolume(aTo);
        return;
    }
    m_global_volume_fader.set(from, aTo, aTime, m_stream_time);
}


void AudioDevice::oscillateVolume(SoundHandle voice_handle, float aFrom, float aTo, SoundTime aTime)
{
    if (aTime <= 0 || aTo == aFrom)
    {
        setVolume(voice_handle, aTo);
        return;
    }

    FOR_ALL_VOICES_PRE
    m_voice[ch]->volume_fader.setLFO(aFrom, aTo, aTime, m_voice[ch]->stream_time);
    FOR_ALL_VOICES_POST
}

void AudioDevice::oscillatePan(SoundHandle voice_handle, float aFrom, float aTo, SoundTime aTime)
{
    if (aTime <= 0 || aTo == aFrom)
    {
        setPan(voice_handle, aTo);
        return;
    }

    FOR_ALL_VOICES_PRE
    m_voice[ch]->pan_fader.setLFO(aFrom, aTo, aTime, m_voice[ch]->stream_time);
    FOR_ALL_VOICES_POST
}

void AudioDevice::oscillateRelativePlaySpeed(SoundHandle voice_handle,
                                             float       aFrom,
                                             float       aTo,
                                             SoundTime   aTime)
{
    if (aTime <= 0 || aTo == aFrom)
    {
        setRelativePlaySpeed(voice_handle, aTo);
        return;
    }

    FOR_ALL_VOICES_PRE
    m_voice[ch]->relative_play_speed_fader.setLFO(aFrom, aTo, aTime, m_voice[ch]->stream_time);
    FOR_ALL_VOICES_POST
}

void AudioDevice::oscillateGlobalVolume(float aFrom, float aTo, SoundTime aTime)
{
    if (aTime <= 0 || aTo == aFrom)
    {
        setGlobalVolume(aTo);
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

void AlignedFloatBuffer::clear()
{
    std::fill_n(m_aligned_ptr, m_count, 0.0f);
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
    m_aligned_ptr = reinterpret_cast<float*>(uintptr_t(m_data.data()) + 15 & ~15);
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


void AudioDevice::postinit_internal(size_t      sample_rate,
                                    size_t      buffer_size,
                                    EngineFlags flags,
                                    size_t      channels)
{
    m_global_volume = 1;
    m_channels      = channels;
    m_samplerate    = sample_rate;
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

    m_flags            = flags;
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

auto AudioDevice::getWave() -> float*
{
    lockAudioMutex_internal();

    for (int i = 0; i < 256; ++i)
    {
        m_wave_data[i] = m_visualization_wave_data[i];
    }

    unlockAudioMutex_internal();
    return m_wave_data.data();
}

auto AudioDevice::getApproximateVolume(size_t aChannel) -> float
{
    if (aChannel > m_channels)
    {
        return 0;
    }

    float vol = 0;

    lockAudioMutex_internal();
    vol = m_visualization_channel_volume[aChannel];
    unlockAudioMutex_internal();

    return vol;
}


auto AudioDevice::calcFFT() -> float*
{
    lockAudioMutex_internal();
    auto temp = std::array<float, 1024>{};

    for (size_t i = 0; i < 256; ++i)
    {
        temp[i * 2]     = m_visualization_wave_data[i];
        temp[i * 2 + 1] = 0;
        temp[i + 512]   = 0;
        temp[i + 768]   = 0;
    }
    unlockAudioMutex_internal();

    FFT::fft1024(temp.data());

    for (size_t i = 0; i < 256; ++i)
    {
        const auto real = temp[i * 2];
        const auto imag = temp[i * 2 + 1];

        m_fft_data[i] = std::sqrt(real * real + imag * imag);
    }

    return m_fft_data.data();
}

#if defined(SOLOUD_SSE_INTRINSICS)
void AudioDevice::clip_internal(const AlignedFloatBuffer& aBuffer,
                                AlignedFloatBuffer&       aDestBuffer,
                                size_t                    aSamples,
                                float                     aVolume0,
                                float                     aVolume1)
{
    float  vd = (aVolume1 - aVolume0) / aSamples;
    float  v  = aVolume0;
    size_t i, c, d;
    size_t samplequads = (aSamples + 3) / 4; // rounded up

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
        volumes.mData[0] = v;
        volumes.mData[1] = v + vd;
        volumes.mData[2] = v + vd + vd;
        volumes.mData[3] = v + vd + vd + vd;
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
                __m128 f = _mm_load_ps(&aBuffer[c]);
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
                _mm_store_ps(&aDestBuffer.mData[d], f);
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
        volumes.mData[0] = v;
        volumes.mData[1] = v + vd;
        volumes.mData[2] = v + vd + vd;
        volumes.mData[3] = v + vd + vd + vd;
        vd *= 4;
        __m128 vdelta = _mm_load_ps1(&vd);
        c             = 0;
        d             = 0;
        for (size_t j = 0; j < m_channels; ++j)
        {
            __m128 vol = _mm_load_ps(volumes.data());
            for (i = 0; i < samplequads; ++i)
            {
                // float f1 = aBuffer.mData[c] * v; ++c; v += vd;
                __m128 f = _mm_load_ps(&aBuffer[c]);
                c += 4;
                f   = _mm_mul_ps(f, vol);
                vol = _mm_add_ps(vol, vdelta);

                // f1 = (f1 <= -1) ? -1 : (f1 >= 1) ? 1 : f1;
                f = _mm_max_ps(f, negbound);
                f = _mm_min_ps(f, posbound);

                // aDestBuffer.mData[d] = f1 * mPostClipScaler; d++;
                f = _mm_mul_ps(f, postscale);
                _mm_store_ps(&aDestBuffer[d], f);
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

                f1 = f1 <= -1.65f  ? -0.9862875f
                     : f1 >= 1.65f ? 0.9862875f
                                   : (0.87f * f1) - 0.1f * f1 * f1 * f1;
                f2 = f2 <= -1.65f  ? -0.9862875f
                     : f2 >= 1.65f ? 0.9862875f
                                   : (0.87f * f2) - 0.1f * f2 * f2 * f2;
                f3 = f3 <= -1.65f  ? -0.9862875f
                     : f3 >= 1.65f ? 0.9862875f
                                   : (0.87f * f3) - 0.1f * f3 * f3 * f3;
                f4 = f4 <= -1.65f  ? -0.9862875f
                     : f4 >= 1.65f ? 0.9862875f
                                   : (0.87f * f4) - 0.1f * f4 * f4 * f4;

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

static constexpr auto FIXPOINT_FRAC_BITS = 20;
static constexpr auto FIXPOINT_FRAC_MUL  = 1 << FIXPOINT_FRAC_BITS;
static constexpr auto FIXPOINT_FRAC_MASK = (1 << FIXPOINT_FRAC_BITS) - 1;

static float catmull_rom(float t, float p0, float p1, float p2, float p3)
{
    return 0.5f * (2 * p1 + (-p0 + p2) * t + (2 * p0 - 5 * p1 + 4 * p2 - p3) * t * t +
                   (-p0 + 3 * p1 - 3 * p2 + p3) * t * t * t);
}

static void resample_catmullrom(const float* aSrc,
                                const float* aSrc1,
                                float*       aDst,
                                int          aSrcOffset,
                                int          aDstSampleCount,
                                int          aStepFixed)
{
    int pos = aSrcOffset;

    for (int i = 0; i < aDstSampleCount; ++i, pos += aStepFixed)
    {
        const int p = pos >> FIXPOINT_FRAC_BITS;
        const int f = pos & FIXPOINT_FRAC_MASK;

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

        aDst[i] = catmull_rom(f / float(FIXPOINT_FRAC_MUL), s3, s2, s1, s0);
    }
}

static void resample_linear(const float* aSrc,
                            const float* aSrc1,
                            float*       aDst,
                            int          aSrcOffset,
                            int          aDstSampleCount,
                            int          aStepFixed)
{
    int pos = aSrcOffset;

    for (int i = 0; i < aDstSampleCount; ++i, pos += aStepFixed)
    {
        const int   p  = pos >> FIXPOINT_FRAC_BITS;
        const int   f  = pos & FIXPOINT_FRAC_MASK;
        float       s1 = aSrc1[sample_granularity - 1];
        const float s2 = aSrc[p];
        if (p != 0)
        {
            s1 = aSrc[p - 1];
        }
        aDst[i] = s1 + (s2 - s1) * f * (1 / (float)FIXPOINT_FRAC_MUL);
    }
}

static void resample_point(const float* aSrc,
                           const float* aSrc1,
                           float*       aDst,
                           int          aSrcOffset,
                           int          aDstSampleCount,
                           int          aStepFixed)
{
    int pos = aSrcOffset;

    for (int i = 0; i < aDstSampleCount; ++i, pos += aStepFixed)
    {
        const int p = pos >> FIXPOINT_FRAC_BITS;

        aDst[i] = aSrc[p];
    }
}

void panAndExpand(std::shared_ptr<AudioSourceInstance>& aVoice,
                  float*                                aBuffer,
                  size_t                                aSamplesToRead,
                  size_t                                aBufferSize,
                  float*                                aScratch,
                  size_t                                aChannels)
{
#ifdef SOLOUD_SSE_INTRINSICS
    assert(((size_t)aBuffer & 0xf) == 0);
    assert(((size_t)aScratch & 0xf) == 0);
    assert(((size_t)aBufferSize & 0xf) == 0);
#endif

    float                           pan[max_channels]; // current speaker volume
    std::array<float, max_channels> pand{}; // destination speaker volume
    std::array<float, max_channels> pani{}; // speaker volume increment per sample

    for (size_t k = 0; k < aChannels; k++)
    {
        pan[k]  = aVoice->current_channel_volume[k];
        pand[k] = aVoice->channel_volume[k] * aVoice->overall_volume;
        pani[k] =
            (pand[k] - pan[k]) /
            aSamplesToRead; // TODO: this is a bit inconsistent.. but it's a hack to begin with
    }

    switch (aChannels)
    {
        case 1: // Target is mono. Sum everything. (1->1, 2->1, 4->1, 6->1, 8->1)
            for (size_t j = 0, ofs = 0; j < aVoice->channel_count; ++j, ofs += aBufferSize)
            {
                pan[0] = aVoice->current_channel_volume[0];
                for (size_t k = 0; k < aSamplesToRead; k++)
                {
                    pan[0] += pani[0];
                    aBuffer[k] += aScratch[ofs + k] * pan[0];
                }
            }
            break;
        case 2:
            switch (aVoice->channel_count)
            {
                case 8: // 8->2, just sum lefties and righties, add a bit of center and sub?
                    for (size_t j = 0; j < aSamplesToRead; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        float s1 = aScratch[j];
                        float s2 = aScratch[aBufferSize + j];
                        float s3 = aScratch[aBufferSize * 2 + j];
                        float s4 = aScratch[aBufferSize * 3 + j];
                        float s5 = aScratch[aBufferSize * 4 + j];
                        float s6 = aScratch[aBufferSize * 5 + j];
                        float s7 = aScratch[aBufferSize * 6 + j];
                        float s8 = aScratch[aBufferSize * 7 + j];
                        aBuffer[j + 0] += 0.2f * (s1 + s3 + s4 + s5 + s7) * pan[0];
                        aBuffer[j + aBufferSize] += 0.2f * (s2 + s3 + s4 + s6 + s8) * pan[1];
                    }
                    break;
                case 6: // 6->2, just sum lefties and righties, add a bit of center and sub?
                    for (size_t j = 0; j < aSamplesToRead; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        float s1 = aScratch[j];
                        float s2 = aScratch[aBufferSize + j];
                        float s3 = aScratch[aBufferSize * 2 + j];
                        float s4 = aScratch[aBufferSize * 3 + j];
                        float s5 = aScratch[aBufferSize * 4 + j];
                        float s6 = aScratch[aBufferSize * 5 + j];
                        aBuffer[j + 0] += 0.3f * (s1 + s3 + s4 + s5) * pan[0];
                        aBuffer[j + aBufferSize] += 0.3f * (s2 + s3 + s4 + s6) * pan[1];
                    }
                    break;
                case 4: // 4->2, just sum lefties and righties
                    for (size_t j = 0; j < aSamplesToRead; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        float s1 = aScratch[j];
                        float s2 = aScratch[aBufferSize + j];
                        float s3 = aScratch[aBufferSize * 2 + j];
                        float s4 = aScratch[aBufferSize * 3 + j];
                        aBuffer[j + 0] += 0.5f * (s1 + s3) * pan[0];
                        aBuffer[j + aBufferSize] += 0.5f * (s2 + s4) * pan[1];
                    }
                    break;
                case 2: // 2->2
#if defined(SOLOUD_SSE_INTRINSICS)
                {
                    int c = 0;
                    // if ((aBufferSize & 3) == 0)
                    {
                        size_t                 samplequads = aSamplesToRead / 4; // rounded down
                        TinyAlignedFloatBuffer pan0;
                        pan0.mData[0] = pan[0] + pani[0];
                        pan0.mData[1] = pan[0] + pani[0] * 2;
                        pan0.mData[2] = pan[0] + pani[0] * 3;
                        pan0.mData[3] = pan[0] + pani[0] * 4;
                        TinyAlignedFloatBuffer pan1;
                        pan1.mData[0] = pan[1] + pani[1];
                        pan1.mData[1] = pan[1] + pani[1] * 2;
                        pan1.mData[2] = pan[1] + pani[1] * 3;
                        pan1.mData[3] = pan[1] + pani[1] * 4;
                        pani[0] *= 4;
                        pani[1] *= 4;
                        __m128 pan0delta = _mm_load_ps1(&pani[0]);
                        __m128 pan1delta = _mm_load_ps1(&pani[1]);
                        __m128 p0        = _mm_load_ps(pan0.mData);
                        __m128 p1        = _mm_load_ps(pan1.mData);

                        for (size_t j = 0; j < samplequads; ++j)
                        {
                            __m128 f0 = _mm_load_ps(aScratch + c);
                            __m128 c0 = _mm_mul_ps(f0, p0);
                            __m128 f1 = _mm_load_ps(aScratch + c + aBufferSize);
                            __m128 c1 = _mm_mul_ps(f1, p1);
                            __m128 o0 = _mm_load_ps(aBuffer + c);
                            __m128 o1 = _mm_load_ps(aBuffer + c + aBufferSize);
                            c0        = _mm_add_ps(c0, o0);
                            c1        = _mm_add_ps(c1, o1);
                            _mm_store_ps(aBuffer + c, c0);
                            _mm_store_ps(aBuffer + c + aBufferSize, c1);
                            p0 = _mm_add_ps(p0, pan0delta);
                            p1 = _mm_add_ps(p1, pan1delta);
                            c += 4;
                        }
                    }

                    // If buffer size or samples to read are not divisible by 4, handle leftovers
                    for (size_t j = c; j < aSamplesToRead; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        float s1 = aScratch[j];
                        float s2 = aScratch[aBufferSize + j];
                        aBuffer[j + 0] += s1 * pan[0];
                        aBuffer[j + aBufferSize] += s2 * pan[1];
                    }
                }
#else // fallback
                    for (size_t j = 0; j < aSamplesToRead; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        float s1 = aScratch[j];
                        float s2 = aScratch[aBufferSize + j];
                        aBuffer[j + 0] += s1 * pan[0];
                        aBuffer[j + aBufferSize] += s2 * pan[1];
                    }
#endif
                break;
                case 1: // 1->2
#if defined(SOLOUD_SSE_INTRINSICS)
                {
                    int c = 0;
                    // if ((aBufferSize & 3) == 0)
                    {
                        size_t                 samplequads = aSamplesToRead / 4; // rounded down
                        TinyAlignedFloatBuffer pan0;
                        pan0.mData[0] = pan[0] + pani[0];
                        pan0.mData[1] = pan[0] + pani[0] * 2;
                        pan0.mData[2] = pan[0] + pani[0] * 3;
                        pan0.mData[3] = pan[0] + pani[0] * 4;
                        TinyAlignedFloatBuffer pan1;
                        pan1.mData[0] = pan[1] + pani[1];
                        pan1.mData[1] = pan[1] + pani[1] * 2;
                        pan1.mData[2] = pan[1] + pani[1] * 3;
                        pan1.mData[3] = pan[1] + pani[1] * 4;
                        pani[0] *= 4;
                        pani[1] *= 4;
                        __m128 pan0delta = _mm_load_ps1(&pani[0]);
                        __m128 pan1delta = _mm_load_ps1(&pani[1]);
                        __m128 p0        = _mm_load_ps(pan0.mData);
                        __m128 p1        = _mm_load_ps(pan1.mData);

                        for (size_t j = 0; j < samplequads; ++j)
                        {
                            __m128 f  = _mm_load_ps(aScratch + c);
                            __m128 c0 = _mm_mul_ps(f, p0);
                            __m128 c1 = _mm_mul_ps(f, p1);
                            __m128 o0 = _mm_load_ps(aBuffer + c);
                            __m128 o1 = _mm_load_ps(aBuffer + c + aBufferSize);
                            c0        = _mm_add_ps(c0, o0);
                            c1        = _mm_add_ps(c1, o1);
                            _mm_store_ps(aBuffer + c, c0);
                            _mm_store_ps(aBuffer + c + aBufferSize, c1);
                            p0 = _mm_add_ps(p0, pan0delta);
                            p1 = _mm_add_ps(p1, pan1delta);
                            c += 4;
                        }
                    }
                    // If buffer size or samples to read are not divisible by 4, handle leftovers
                    for (size_t j = c; j < aSamplesToRead; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        float s = aScratch[j];
                        aBuffer[j + 0] += s * pan[0];
                        aBuffer[j + aBufferSize] += s * pan[1];
                    }
                }
#else // fallback
                    for (size_t j = 0; j < aSamplesToRead; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        float s = aScratch[j];
                        aBuffer[j + 0] += s * pan[0];
                        aBuffer[j + aBufferSize] += s * pan[1];
                    }
#endif
                break;
            }
            break;
        case 4:
            switch (aVoice->channel_count)
            {
                case 8: // 8->4, add a bit of center, sub?
                    for (size_t j = 0; j < aSamplesToRead; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        pan[2] += pani[2];
                        pan[3] += pani[3];
                        float s1 = aScratch[j];
                        float s2 = aScratch[aBufferSize + j];
                        float s3 = aScratch[aBufferSize * 2 + j];
                        float s4 = aScratch[aBufferSize * 3 + j];
                        float s5 = aScratch[aBufferSize * 4 + j];
                        float s6 = aScratch[aBufferSize * 5 + j];
                        float s7 = aScratch[aBufferSize * 6 + j];
                        float s8 = aScratch[aBufferSize * 7 + j];
                        float c  = (s3 + s4) * 0.7f;
                        aBuffer[j + 0] += s1 * pan[0] + c;
                        aBuffer[j + aBufferSize] += s2 * pan[1] + c;
                        aBuffer[j + aBufferSize * 2] += 0.5f * (s5 + s7) * pan[2];
                        aBuffer[j + aBufferSize * 3] += 0.5f * (s6 + s8) * pan[3];
                    }
                    break;
                case 6: // 6->4, add a bit of center, sub?
                    for (size_t j = 0; j < aSamplesToRead; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        pan[2] += pani[2];
                        pan[3] += pani[3];
                        float s1 = aScratch[j];
                        float s2 = aScratch[aBufferSize + j];
                        float s3 = aScratch[aBufferSize * 2 + j];
                        float s4 = aScratch[aBufferSize * 3 + j];
                        float s5 = aScratch[aBufferSize * 4 + j];
                        float s6 = aScratch[aBufferSize * 5 + j];
                        float c  = (s3 + s4) * 0.7f;
                        aBuffer[j + 0] += s1 * pan[0] + c;
                        aBuffer[j + aBufferSize] += s2 * pan[1] + c;
                        aBuffer[j + aBufferSize * 2] += s5 * pan[2];
                        aBuffer[j + aBufferSize * 3] += s6 * pan[3];
                    }
                    break;
                case 4: // 4->4
                    for (size_t j = 0; j < aSamplesToRead; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        pan[2] += pani[2];
                        pan[3] += pani[3];
                        float s1 = aScratch[j];
                        float s2 = aScratch[aBufferSize + j];
                        float s3 = aScratch[aBufferSize * 2 + j];
                        float s4 = aScratch[aBufferSize * 3 + j];
                        aBuffer[j + 0] += s1 * pan[0];
                        aBuffer[j + aBufferSize] += s2 * pan[1];
                        aBuffer[j + aBufferSize * 2] += s3 * pan[2];
                        aBuffer[j + aBufferSize * 3] += s4 * pan[3];
                    }
                    break;
                case 2: // 2->4
                    for (size_t j = 0; j < aSamplesToRead; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        pan[2] += pani[2];
                        pan[3] += pani[3];
                        float s1 = aScratch[j];
                        float s2 = aScratch[aBufferSize + j];
                        aBuffer[j + 0] += s1 * pan[0];
                        aBuffer[j + aBufferSize] += s2 * pan[1];
                        aBuffer[j + aBufferSize * 2] += s1 * pan[2];
                        aBuffer[j + aBufferSize * 3] += s2 * pan[3];
                    }
                    break;
                case 1: // 1->4
                    for (size_t j = 0; j < aSamplesToRead; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        pan[2] += pani[2];
                        pan[3] += pani[3];
                        float s = aScratch[j];
                        aBuffer[j + 0] += s * pan[0];
                        aBuffer[j + aBufferSize] += s * pan[1];
                        aBuffer[j + aBufferSize * 2] += s * pan[2];
                        aBuffer[j + aBufferSize * 3] += s * pan[3];
                    }
                    break;
            }
            break;
        case 6:
            switch (aVoice->channel_count)
            {
                case 8: // 8->6
                    for (size_t j = 0; j < aSamplesToRead; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        pan[2] += pani[2];
                        pan[3] += pani[3];
                        pan[4] += pani[4];
                        pan[5] += pani[5];
                        float s1 = aScratch[j];
                        float s2 = aScratch[aBufferSize + j];
                        float s3 = aScratch[aBufferSize * 2 + j];
                        float s4 = aScratch[aBufferSize * 3 + j];
                        float s5 = aScratch[aBufferSize * 4 + j];
                        float s6 = aScratch[aBufferSize * 5 + j];
                        float s7 = aScratch[aBufferSize * 6 + j];
                        float s8 = aScratch[aBufferSize * 7 + j];
                        aBuffer[j + 0] += s1 * pan[0];
                        aBuffer[j + aBufferSize] += s2 * pan[1];
                        aBuffer[j + aBufferSize * 2] += s3 * pan[2];
                        aBuffer[j + aBufferSize * 3] += s4 * pan[3];
                        aBuffer[j + aBufferSize * 4] += 0.5f * (s5 + s7) * pan[4];
                        aBuffer[j + aBufferSize * 5] += 0.5f * (s6 + s8) * pan[5];
                    }
                    break;
                case 6: // 6->6
                    for (size_t j = 0; j < aSamplesToRead; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        pan[2] += pani[2];
                        pan[3] += pani[3];
                        pan[4] += pani[4];
                        pan[5] += pani[5];
                        float s1 = aScratch[j];
                        float s2 = aScratch[aBufferSize + j];
                        float s3 = aScratch[aBufferSize * 2 + j];
                        float s4 = aScratch[aBufferSize * 3 + j];
                        float s5 = aScratch[aBufferSize * 4 + j];
                        float s6 = aScratch[aBufferSize * 5 + j];
                        aBuffer[j + 0] += s1 * pan[0];
                        aBuffer[j + aBufferSize] += s2 * pan[1];
                        aBuffer[j + aBufferSize * 2] += s3 * pan[2];
                        aBuffer[j + aBufferSize * 3] += s4 * pan[3];
                        aBuffer[j + aBufferSize * 4] += s5 * pan[4];
                        aBuffer[j + aBufferSize * 5] += s6 * pan[5];
                    }
                    break;
                case 4: // 4->6
                    for (size_t j = 0; j < aSamplesToRead; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        pan[2] += pani[2];
                        pan[3] += pani[3];
                        pan[4] += pani[4];
                        pan[5] += pani[5];
                        float s1 = aScratch[j];
                        float s2 = aScratch[aBufferSize + j];
                        float s3 = aScratch[aBufferSize * 2 + j];
                        float s4 = aScratch[aBufferSize * 3 + j];
                        aBuffer[j + 0] += s1 * pan[0];
                        aBuffer[j + aBufferSize] += s2 * pan[1];
                        aBuffer[j + aBufferSize * 2] += 0.5f * (s1 + s2) * pan[2];
                        aBuffer[j + aBufferSize * 3] += 0.25f * (s1 + s2 + s3 + s4) * pan[3];
                        aBuffer[j + aBufferSize * 4] += s3 * pan[4];
                        aBuffer[j + aBufferSize * 5] += s4 * pan[5];
                    }
                    break;
                case 2: // 2->6
                    for (size_t j = 0; j < aSamplesToRead; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        pan[2] += pani[2];
                        pan[3] += pani[3];
                        pan[4] += pani[4];
                        pan[5] += pani[5];
                        float s1 = aScratch[j];
                        float s2 = aScratch[aBufferSize + j];
                        aBuffer[j + 0] += s1 * pan[0];
                        aBuffer[j + aBufferSize] += s2 * pan[1];
                        aBuffer[j + aBufferSize * 2] += 0.5f * (s1 + s2) * pan[2];
                        aBuffer[j + aBufferSize * 3] += 0.5f * (s1 + s2) * pan[3];
                        aBuffer[j + aBufferSize * 4] += s1 * pan[4];
                        aBuffer[j + aBufferSize * 5] += s2 * pan[5];
                    }
                    break;
                case 1: // 1->6
                    for (size_t j = 0; j < aSamplesToRead; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        pan[2] += pani[2];
                        pan[3] += pani[3];
                        pan[4] += pani[4];
                        pan[5] += pani[5];
                        float s = aScratch[j];
                        aBuffer[j + 0] += s * pan[0];
                        aBuffer[j + aBufferSize] += s * pan[1];
                        aBuffer[j + aBufferSize * 2] += s * pan[2];
                        aBuffer[j + aBufferSize * 3] += s * pan[3];
                        aBuffer[j + aBufferSize * 4] += s * pan[4];
                        aBuffer[j + aBufferSize * 5] += s * pan[5];
                    }
                    break;
            }
            break;
        case 8:
            switch (aVoice->channel_count)
            {
                case 8: // 8->8
                    for (size_t j = 0; j < aSamplesToRead; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        pan[2] += pani[2];
                        pan[3] += pani[3];
                        pan[4] += pani[4];
                        pan[5] += pani[5];
                        pan[6] += pani[6];
                        pan[7] += pani[7];
                        float s1 = aScratch[j];
                        float s2 = aScratch[aBufferSize + j];
                        float s3 = aScratch[aBufferSize * 2 + j];
                        float s4 = aScratch[aBufferSize * 3 + j];
                        float s5 = aScratch[aBufferSize * 4 + j];
                        float s6 = aScratch[aBufferSize * 5 + j];
                        float s7 = aScratch[aBufferSize * 6 + j];
                        float s8 = aScratch[aBufferSize * 7 + j];
                        aBuffer[j + 0] += s1 * pan[0];
                        aBuffer[j + aBufferSize] += s2 * pan[1];
                        aBuffer[j + aBufferSize * 2] += s3 * pan[2];
                        aBuffer[j + aBufferSize * 3] += s4 * pan[3];
                        aBuffer[j + aBufferSize * 4] += s5 * pan[4];
                        aBuffer[j + aBufferSize * 5] += s6 * pan[5];
                        aBuffer[j + aBufferSize * 6] += s7 * pan[6];
                        aBuffer[j + aBufferSize * 7] += s8 * pan[7];
                    }
                    break;
                case 6: // 6->8
                    for (size_t j = 0; j < aSamplesToRead; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        pan[2] += pani[2];
                        pan[3] += pani[3];
                        pan[4] += pani[4];
                        pan[5] += pani[5];
                        pan[6] += pani[6];
                        pan[7] += pani[7];
                        float s1 = aScratch[j];
                        float s2 = aScratch[aBufferSize + j];
                        float s3 = aScratch[aBufferSize * 2 + j];
                        float s4 = aScratch[aBufferSize * 3 + j];
                        float s5 = aScratch[aBufferSize * 4 + j];
                        float s6 = aScratch[aBufferSize * 5 + j];
                        aBuffer[j + 0] += s1 * pan[0];
                        aBuffer[j + aBufferSize] += s2 * pan[1];
                        aBuffer[j + aBufferSize * 2] += s3 * pan[2];
                        aBuffer[j + aBufferSize * 3] += s4 * pan[3];
                        aBuffer[j + aBufferSize * 4] += 0.5f * (s5 + s1) * pan[4];
                        aBuffer[j + aBufferSize * 5] += 0.5f * (s6 + s2) * pan[5];
                        aBuffer[j + aBufferSize * 6] += s5 * pan[6];
                        aBuffer[j + aBufferSize * 7] += s6 * pan[7];
                    }
                    break;
                case 4: // 4->8
                    for (size_t j = 0; j < aSamplesToRead; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        pan[2] += pani[2];
                        pan[3] += pani[3];
                        pan[4] += pani[4];
                        pan[5] += pani[5];
                        pan[6] += pani[6];
                        pan[7] += pani[7];
                        float s1 = aScratch[j];
                        float s2 = aScratch[aBufferSize + j];
                        float s3 = aScratch[aBufferSize * 2 + j];
                        float s4 = aScratch[aBufferSize * 3 + j];
                        aBuffer[j + 0] += s1 * pan[0];
                        aBuffer[j + aBufferSize] += s2 * pan[1];
                        aBuffer[j + aBufferSize * 2] += 0.5f * (s1 + s2) * pan[2];
                        aBuffer[j + aBufferSize * 3] += 0.25f * (s1 + s2 + s3 + s4) * pan[3];
                        aBuffer[j + aBufferSize * 4] += 0.5f * (s1 + s3) * pan[4];
                        aBuffer[j + aBufferSize * 5] += 0.5f * (s2 + s4) * pan[5];
                        aBuffer[j + aBufferSize * 6] += s3 * pan[4];
                        aBuffer[j + aBufferSize * 7] += s4 * pan[5];
                    }
                    break;
                case 2: // 2->8
                    for (size_t j = 0; j < aSamplesToRead; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        pan[2] += pani[2];
                        pan[3] += pani[3];
                        pan[4] += pani[4];
                        pan[5] += pani[5];
                        pan[6] += pani[6];
                        pan[7] += pani[7];
                        float s1 = aScratch[j];
                        float s2 = aScratch[aBufferSize + j];
                        aBuffer[j + 0] += s1 * pan[0];
                        aBuffer[j + aBufferSize] += s2 * pan[1];
                        aBuffer[j + aBufferSize * 2] += 0.5f * (s1 + s2) * pan[2];
                        aBuffer[j + aBufferSize * 3] += 0.5f * (s1 + s2) * pan[3];
                        aBuffer[j + aBufferSize * 4] += s1 * pan[4];
                        aBuffer[j + aBufferSize * 5] += s2 * pan[5];
                        aBuffer[j + aBufferSize * 6] += s1 * pan[6];
                        aBuffer[j + aBufferSize * 7] += s2 * pan[7];
                    }
                    break;
                case 1: // 1->8
                    for (size_t j = 0; j < aSamplesToRead; ++j)
                    {
                        pan[0] += pani[0];
                        pan[1] += pani[1];
                        pan[2] += pani[2];
                        pan[3] += pani[3];
                        pan[4] += pani[4];
                        pan[5] += pani[5];
                        pan[6] += pani[6];
                        pan[7] += pani[7];
                        float s = aScratch[j];
                        aBuffer[j + 0] += s * pan[0];
                        aBuffer[j + aBufferSize] += s * pan[1];
                        aBuffer[j + aBufferSize * 2] += s * pan[2];
                        aBuffer[j + aBufferSize * 3] += s * pan[3];
                        aBuffer[j + aBufferSize * 4] += s * pan[4];
                        aBuffer[j + aBufferSize * 5] += s * pan[5];
                        aBuffer[j + aBufferSize * 6] += s * pan[6];
                        aBuffer[j + aBufferSize * 7] += s * pan[7];
                    }
                    break;
            }
            break;
    }

    for (size_t k = 0; k < aChannels; k++)
    {
        aVoice->current_channel_volume[k] = pand[k];
    }
}

void AudioDevice::mixBus_internal(float*    aBuffer,
                                  size_t    aSamplesToRead,
                                  size_t    aBufferSize,
                                  float*    aScratch,
                                  size_t    aBus,
                                  float     aSamplerate,
                                  size_t    aChannels,
                                  Resampler aResampler)
{
    // Clear accumulation buffer
    for (size_t i = 0; i < aSamplesToRead; ++i)
    {
        for (size_t j = 0; j < aChannels; ++j)
        {
            aBuffer[i + j * aBufferSize] = 0;
        }
    }

    // Accumulate sound sources
    for (size_t i = 0; i < m_active_voice_count; ++i)
    {
        if (auto& voice = m_voice[m_active_voice[i]];
            voice != nullptr && voice->bus_handle == aBus && !voice->flags.Paused &&
            !voice->flags.Inaudible)
        {
            float step = voice->sample_rate / aSamplerate;

            // avoid step overflow
            if (step > (1 << (32 - FIXPOINT_FRAC_BITS)))
            {
                step = 0;
            }

            auto step_fixed = int(floor(step * FIXPOINT_FRAC_MUL));
            auto outofs     = size_t(0);

            if (voice->delay_samples)
            {
                if (voice->delay_samples > aSamplesToRead)
                {
                    outofs = aSamplesToRead;
                    voice->delay_samples -= aSamplesToRead;
                }
                else
                {
                    outofs               = voice->delay_samples;
                    voice->delay_samples = 0;
                }

                // Clear scratch where we're skipping
                for (size_t k = 0; k < voice->channel_count; k++)
                {
                    memset(aScratch + k * aBufferSize, 0, sizeof(float) * outofs);
                }
            }

            while (step_fixed != 0 && outofs < aSamplesToRead)
            {
                if (voice->leftover_samples == 0)
                {
                    // Swap resample buffers (ping-pong)
                    float* t                = voice->resample_data[0];
                    voice->resample_data[0] = voice->resample_data[1];
                    voice->resample_data[1] = t;

                    // Get a block of source data

                    auto read_count = size_t(0);

                    if (!voice->has_ended() || voice->flags.Looping)
                    {
                        read_count = voice->audio(voice->resample_data[0],
                                                  sample_granularity,
                                                  sample_granularity);
                        if (read_count < sample_granularity)
                        {
                            if (voice->flags.Looping)
                            {
                                while (read_count < sample_granularity &&
                                       voice->seek(voice->loop_point,
                                                   m_scratch.data(),
                                                   m_scratch_size))
                                {
                                    voice->loop_count++;

                                    const int inc =
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
                    if (voice->src_offset < sample_granularity * FIXPOINT_FRAC_MUL)
                    {
                        voice->src_offset = 0;
                    }
                    else
                    {
                        // We have new block of data, move pointer backwards
                        voice->src_offset -= sample_granularity * FIXPOINT_FRAC_MUL;
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

                if (voice->src_offset < sample_granularity * FIXPOINT_FRAC_MUL)
                {
                    writesamples = ((sample_granularity * FIXPOINT_FRAC_MUL) - voice->src_offset) /
                                       step_fixed +
                                   1;

                    // avoid reading past the current buffer..
                    if (((writesamples * step_fixed + voice->src_offset) >> FIXPOINT_FRAC_BITS) >=
                        sample_granularity)
                        writesamples--;
                }


                // If this is too much for our output buffer, don't write that many:
                if (writesamples + outofs > aSamplesToRead)
                {
                    voice->leftover_samples = (writesamples + outofs) - aSamplesToRead;
                    writesamples            = aSamplesToRead - outofs;
                }

                // Call resampler to generate the samples, once per channel
                if (writesamples)
                {
                    for (size_t j = 0; j < voice->channel_count; ++j)
                    {
                        switch (aResampler)
                        {
                            case Resampler::Point:
                                resample_point(voice->resample_data[0] + sample_granularity * j,
                                               voice->resample_data[1] + sample_granularity * j,
                                               aScratch + aBufferSize * j + outofs,
                                               voice->src_offset,
                                               writesamples,
                                               /*voice->mSamplerate,
                                               aSamplerate,*/
                                               step_fixed);
                                break;
                            case Resampler::CatmullRom:
                                resample_catmullrom(
                                    voice->resample_data[0] + sample_granularity * j,
                                    voice->resample_data[1] + sample_granularity * j,
                                    aScratch + aBufferSize * j + outofs,
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
                                                aScratch + aBufferSize * j + outofs,
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
            panAndExpand(voice, aBuffer, aSamplesToRead, aBufferSize, aScratch, aChannels);

            // clear voice if the sound is over
            // TODO: check this condition some day
            if (!voice->flags.Looping && !voice->flags.DisableAutostop && voice->has_ended())
            {
                stopVoice_internal(m_active_voice[i]);
            }
        }
        else if (voice && voice->bus_handle == aBus && !voice->flags.Paused &&
                 voice->flags.Inaudible && voice->flags.InaudibleTick)
        {
            // Inaudible but needs ticking. Do minimal work (keep counters up to date and ask
            // audiosource for data)
            auto step       = voice->sample_rate / aSamplerate;
            auto step_fixed = int(floor(step * FIXPOINT_FRAC_MUL));
            auto outofs     = size_t(0);

            if (voice->delay_samples)
            {
                if (voice->delay_samples > aSamplesToRead)
                {
                    outofs = aSamplesToRead;
                    voice->delay_samples -= aSamplesToRead;
                }
                else
                {
                    outofs               = voice->delay_samples;
                    voice->delay_samples = 0;
                }
            }

            while (step_fixed != 0 && outofs < aSamplesToRead)
            {
                if (voice->leftover_samples == 0)
                {
                    // Swap resample buffers (ping-pong)
                    float* t                = voice->resample_data[0];
                    voice->resample_data[0] = voice->resample_data[1];
                    voice->resample_data[1] = t;

                    // Get a block of source data

                    if (!voice->has_ended() || voice->flags.Looping)
                    {
                        auto readcount = voice->audio(voice->resample_data[0],
                                                      sample_granularity,
                                                      sample_granularity);
                        if (readcount < sample_granularity)
                        {
                            if (voice->flags.Looping)
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
                    if (voice->src_offset < sample_granularity * FIXPOINT_FRAC_MUL)
                    {
                        voice->src_offset = 0;
                    }
                    else
                    {
                        // We have new block of data, move pointer backwards
                        voice->src_offset -= sample_granularity * FIXPOINT_FRAC_MUL;
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

                if (voice->src_offset < sample_granularity * FIXPOINT_FRAC_MUL)
                {
                    writesamples = ((sample_granularity * FIXPOINT_FRAC_MUL) - voice->src_offset) /
                                       step_fixed +
                                   1;

                    // avoid reading past the current buffer..
                    if (((writesamples * step_fixed + voice->src_offset) >> FIXPOINT_FRAC_BITS) >=
                        sample_granularity)
                        writesamples--;
                }


                // If this is too much for our output buffer, don't write that many:
                if (writesamples + outofs > aSamplesToRead)
                {
                    voice->leftover_samples = (writesamples + outofs) - aSamplesToRead;
                    writesamples            = aSamplesToRead - outofs;
                }

                // Skip resampler

                // Keep track of how many samples we've written so far
                outofs += writesamples;

                // Move source pointer onwards (writesamples may be zero)
                voice->src_offset += writesamples * step_fixed;
            }

            // clear voice if the sound is over
            // TODO: check this condition some day
            if (!voice->flags.Looping && !voice->flags.DisableAutostop && voice->has_ended())
            {
                stopVoice_internal(m_active_voice[i]);
            }
        }
    }
}

void AudioDevice::mapResampleBuffers_internal()
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
        if (!(live[i] & 1) && m_resample_data_owner[i]) // For all dead channels with owners..
        {
            m_resample_data_owner[i]->resample_data[0] = nullptr;
            m_resample_data_owner[i]->resample_data[1] = nullptr;
            m_resample_data_owner[i]                   = nullptr;
        }
    }

    auto latestfree = 0;

    for (size_t i = 0; i < m_active_voice_count; ++i)
    {
        if (!(live[i] & 2) && m_voice[m_active_voice[i]]) // For all live voices with no channel..
        {
            int found = -1;

            for (size_t j = latestfree; found == -1 && j < m_max_active_voices; ++j)
            {
                if (m_resample_data_owner[j] == nullptr)
                {
                    found = j;
                }
            }

            assert(found != -1);
            m_resample_data_owner[found]                   = m_voice[m_active_voice[i]];
            m_resample_data_owner[found]->resample_data[0] = m_resample_data[found * 2 + 0];
            m_resample_data_owner[found]->resample_data[1] = m_resample_data[found * 2 + 1];

            memset(m_resample_data_owner[found]->resample_data[0],
                   0,
                   sizeof(float) * sample_granularity * max_channels);

            memset(m_resample_data_owner[found]->resample_data[1],
                   0,
                   sizeof(float) * sample_granularity * max_channels);

            latestfree = found + 1;
        }
    }
}

void AudioDevice::calcActiveVoices_internal()
{
    // TODO: consider whether we need to re-evaluate the active voices all the time.
    // It is a must when new voices are started, but otherwise we could get away
    // with postponing it sometimes..

    m_active_voice_dirty = false;

    // Populate
    size_t candidates = 0;
    size_t mustlive   = 0;

    for (size_t i = 0; i < m_highest_voice; ++i)
    {
        const auto voice = m_voice[i];
        if (voice == nullptr)
        {
            continue;
        }

        // TODO: check this some day
        if ((!voice->flags.Inaudible && !voice->flags.Paused) || voice->flags.InaudibleTick)
        {
            m_active_voice[candidates] = i;
            candidates++;
            if (m_voice[i]->flags.InaudibleTick)
            {
                m_active_voice[candidates - 1] = m_active_voice[mustlive];
                m_active_voice[mustlive]       = i;
                mustlive++;
            }
        }
    }

    // Check for early out
    if (candidates <= m_max_active_voices)
    {
        // everything is audible, early out
        m_active_voice_count = candidates;
        mapResampleBuffers_internal();
        return;
    }

    m_active_voice_count = m_max_active_voices;

    if (mustlive >= m_max_active_voices)
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
    int       left = 0, stack[24], pos = 0;
    int       len  = candidates - mustlive;
    size_t*   data = m_active_voice.data() + mustlive;
    const int k    = m_active_voice_count;
    while (true)
    {
        for (; left + 1 < len; len++)
        {
            if (pos == 24)
            {
                len = stack[pos = 0];
            }

            const int   pivot    = data[left];
            const float pivotvol = m_voice[pivot]->overall_volume;
            stack[pos++]         = len;

            for (int right = left - 1;;)
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
    mapResampleBuffers_internal();
}

void AudioDevice::mix_internal(size_t aSamples, size_t aStride)
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

    const auto buffertime   = aSamples / float(m_samplerate);
    auto       globalVolume = std::array<float, 2>{};

    m_stream_time += buffertime;
    m_last_clocked_time = 0;

    globalVolume[0] = m_global_volume;

    if (m_global_volume_fader.mActive)
    {
        m_global_volume = m_global_volume_fader.get(m_stream_time);
    }

    globalVolume[1] = m_global_volume;

    lockAudioMutex_internal();

    // Process faders. May change scratch size.
    for (size_t i = 0; i < m_highest_voice; ++i)
    {
        if (m_voice[i] != nullptr && !m_voice[i]->flags.Paused)
        {
            auto volume = std::array<float, 2>{};

            m_voice[i]->active_fader = 0;

            if (m_global_volume_fader.mActive > 0)
            {
                m_voice[i]->active_fader = 1;
            }

            m_voice[i]->stream_time += buffertime;
            m_voice[i]->stream_position +=
                double(buffertime) * double(m_voice[i]->overall_relative_play_speed);

            // TODO: this is actually unstable, because mStreamTime depends on the relative play
            // speed.
            if (m_voice[i]->relative_play_speed_fader.mActive > 0)
            {
                float speed = m_voice[i]->relative_play_speed_fader.get(m_voice[i]->stream_time);
                setVoiceRelativePlaySpeed_internal(i, speed);
            }

            volume[0] = m_voice[i]->overall_volume;

            if (m_voice[i]->volume_fader.mActive > 0)
            {
                m_voice[i]->set_volume   = m_voice[i]->volume_fader.get(m_voice[i]->stream_time);
                m_voice[i]->active_fader = 1;
                updateVoiceVolume_internal(i);
                m_active_voice_dirty = true;
            }

            volume[1] = m_voice[i]->overall_volume;

            if (m_voice[i]->pan_fader.mActive > 0)
            {
                float pan = m_voice[i]->pan_fader.get(m_voice[i]->stream_time);
                setVoicePan_internal(i, pan);
                m_voice[i]->active_fader = 1;
            }

            if (m_voice[i]->pause_scheduler.mActive)
            {
                m_voice[i]->pause_scheduler.get(m_voice[i]->stream_time);
                if (m_voice[i]->pause_scheduler.mActive == -1)
                {
                    m_voice[i]->pause_scheduler.mActive = 0;
                    setVoicePause_internal(i, 1);
                }
            }

            if (m_voice[i]->stop_scheduler.mActive)
            {
                m_voice[i]->stop_scheduler.get(m_voice[i]->stream_time);
                if (m_voice[i]->stop_scheduler.mActive == -1)
                {
                    m_voice[i]->stop_scheduler.mActive = 0;
                    stopVoice_internal(i);
                }
            }
        }
    }

    if (m_active_voice_dirty)
    {
        calcActiveVoices_internal();
    }

    mixBus_internal(m_output_scratch.data(),
                    aSamples,
                    aStride,
                    m_scratch.data(),
                    0,
                    float(m_samplerate),
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
            .samples     = aSamples,
            .buffer_size = aStride,
            .channels    = m_channels,
            .sample_rate = float(m_samplerate),
            .time        = m_stream_time,
        });
    }

    unlockAudioMutex_internal();

    // Note: clipping channels*aStride, not channels*aSamples, so we're possibly clipping some
    // unused data. The buffers should be large enough for it, we just may do a few bytes of
    // unnecessary work.
    clip_internal(m_output_scratch, m_scratch, aStride, globalVolume[0], globalVolume[1]);

    if (m_flags.enable_visualization)
    {
        for (size_t i = 0; i < max_channels; ++i)
        {
            m_visualization_channel_volume[i] = 0;
        }

        if (aSamples > 255)
        {
            for (size_t i = 0; i < 256; ++i)
            {
                m_visualization_wave_data[i] = 0;
                for (size_t j = 0; j < m_channels; ++j)
                {
                    const auto sample = m_scratch[i + (j * aStride)];
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
                    const auto sample = m_scratch[(i % aSamples) + (j * aStride)];
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

void AudioDevice::mix(float* aBuffer, size_t aSamples)
{
    size_t stride = (aSamples + 15) & ~0xf;
    mix_internal(aSamples, stride);
    interlace_samples_float(m_scratch.data(), aBuffer, aSamples, m_channels, stride);
}

void AudioDevice::mixSigned16(short* aBuffer, size_t aSamples)
{
    size_t stride = (aSamples + 15) & ~0xf;
    mix_internal(aSamples, stride);
    interlace_samples_s16(m_scratch.data(), aBuffer, aSamples, m_channels, stride);
}

void AudioDevice::lockAudioMutex_internal()
{
    if (m_audio_thread_mutex)
    {
        thread::lock_mutex(m_audio_thread_mutex);
    }
    assert(!m_inside_audio_thread_mutex);
    m_inside_audio_thread_mutex = true;
}

void AudioDevice::unlockAudioMutex_internal()
{
    assert(m_inside_audio_thread_mutex);
    m_inside_audio_thread_mutex = false;
    if (m_audio_thread_mutex)
    {
        thread::unlock_mutex(m_audio_thread_mutex);
    }
}

float AudioDevice::post_clip_scaler() const
{
    return m_post_clip_scaler;
}

Resampler AudioDevice::main_resampler() const
{
    return m_resampler;
}

float AudioDevice::global_volume() const
{
    return m_global_volume;
}

SoundHandle AudioDevice::getHandleFromVoice_internal(size_t aVoice) const
{
    if (m_voice[aVoice] == nullptr)
    {
        return 0;
    }

    return (aVoice + 1) | (m_voice[aVoice]->play_index << 12);
}

int AudioDevice::getVoiceFromHandle_internal(SoundHandle voice_handle) const
{
    // If this is a voice group handle, pick the first handle from the group
    if (const auto* h = voiceGroupHandleToArray_internal(voice_handle); h != nullptr)
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

size_t AudioDevice::max_active_voice_count() const
{
    return m_max_active_voices;
}

size_t AudioDevice::active_voice_count()
{
    lockAudioMutex_internal();
    if (m_active_voice_dirty)
    {
        calcActiveVoices_internal();
    }
    const size_t c = m_active_voice_count;
    unlockAudioMutex_internal();

    return c;
}

size_t AudioDevice::voice_count()
{
    lockAudioMutex_internal();
    int c = 0;
    for (size_t i = 0; i < m_highest_voice; ++i)
    {
        if (m_voice[i])
        {
            ++c;
        }
    }
    unlockAudioMutex_internal();
    return c;
}

bool AudioDevice::is_valid_voice_handle(SoundHandle voice_handle)
{
    // voice groups are not valid voice handles
    if ((voice_handle & 0xfffff000) == 0xfffff000)
    {
        return false;
    }

    lockAudioMutex_internal();
    if (getVoiceFromHandle_internal(voice_handle) != -1)
    {
        unlockAudioMutex_internal();
        return true;
    }
    unlockAudioMutex_internal();
    return false;
}


SoundTime AudioDevice::getLoopPoint(SoundHandle voice_handle)
{
    lockAudioMutex_internal();
    const int ch = getVoiceFromHandle_internal(voice_handle);
    if (ch == -1)
    {
        unlockAudioMutex_internal();
        return 0;
    }
    const SoundTime v = m_voice[ch]->loop_point;
    unlockAudioMutex_internal();
    return v;
}

bool AudioDevice::is_voice_looping(SoundHandle voice_handle)
{
    lockAudioMutex_internal();
    const int ch = getVoiceFromHandle_internal(voice_handle);
    if (ch == -1)
    {
        unlockAudioMutex_internal();
        return 0;
    }
    const bool v = m_voice[ch]->flags.Looping;
    unlockAudioMutex_internal();
    return v;
}

bool AudioDevice::getAutoStop(SoundHandle voice_handle)
{
    lockAudioMutex_internal();
    const int ch = getVoiceFromHandle_internal(voice_handle);
    if (ch == -1)
    {
        unlockAudioMutex_internal();
        return false;
    }
    const auto v = m_voice[ch]->flags.DisableAutostop;
    unlockAudioMutex_internal();
    return !v;
}

float AudioDevice::getInfo(SoundHandle voice_handle, size_t mInfoKey)
{
    lockAudioMutex_internal();
    const int ch = getVoiceFromHandle_internal(voice_handle);
    if (ch == -1)
    {
        unlockAudioMutex_internal();
        return 0;
    }
    const float v = m_voice[ch]->getInfo(mInfoKey);
    unlockAudioMutex_internal();
    return v;
}

float AudioDevice::volume(SoundHandle voice_handle)
{
    lockAudioMutex_internal();
    const int ch = getVoiceFromHandle_internal(voice_handle);
    if (ch == -1)
    {
        unlockAudioMutex_internal();
        return 0;
    }
    const float v = m_voice[ch]->set_volume;
    unlockAudioMutex_internal();
    return v;
}

float AudioDevice::overall_volume(SoundHandle voice_handle)
{
    lockAudioMutex_internal();
    const int ch = getVoiceFromHandle_internal(voice_handle);
    if (ch == -1)
    {
        unlockAudioMutex_internal();
        return 0;
    }
    const float v = m_voice[ch]->overall_volume;
    unlockAudioMutex_internal();
    return v;
}

float AudioDevice::pan(SoundHandle voice_handle)
{
    lockAudioMutex_internal();
    const int ch = getVoiceFromHandle_internal(voice_handle);
    if (ch == -1)
    {
        unlockAudioMutex_internal();
        return 0;
    }
    const float v = m_voice[ch]->pan;
    unlockAudioMutex_internal();
    return v;
}

SoundTime AudioDevice::stream_time(SoundHandle voice_handle)
{
    lockAudioMutex_internal();
    const int ch = getVoiceFromHandle_internal(voice_handle);
    if (ch == -1)
    {
        unlockAudioMutex_internal();
        return 0;
    }
    const double v = m_voice[ch]->stream_time;
    unlockAudioMutex_internal();
    return v;
}

SoundTime AudioDevice::stream_position(SoundHandle voice_handle)
{
    lockAudioMutex_internal();
    const int ch = getVoiceFromHandle_internal(voice_handle);
    if (ch == -1)
    {
        unlockAudioMutex_internal();
        return 0;
    }
    const double v = m_voice[ch]->stream_position;
    unlockAudioMutex_internal();
    return v;
}

float AudioDevice::relative_play_speed(SoundHandle voice_handle)
{
    lockAudioMutex_internal();
    const int ch = getVoiceFromHandle_internal(voice_handle);
    if (ch == -1)
    {
        unlockAudioMutex_internal();
        return 1;
    }
    const float v = m_voice[ch]->set_relative_play_speed;
    unlockAudioMutex_internal();
    return v;
}

float AudioDevice::sample_rate(SoundHandle voice_handle)
{
    lockAudioMutex_internal();
    const int ch = getVoiceFromHandle_internal(voice_handle);
    if (ch == -1)
    {
        unlockAudioMutex_internal();
        return 0;
    }
    const float v = m_voice[ch]->base_sample_rate;
    unlockAudioMutex_internal();
    return v;
}

bool AudioDevice::pause(SoundHandle voice_handle)
{
    lockAudioMutex_internal();
    const int ch = getVoiceFromHandle_internal(voice_handle);
    if (ch == -1)
    {
        unlockAudioMutex_internal();
        return false;
    }
    const auto v = m_voice[ch]->flags.Paused;
    unlockAudioMutex_internal();
    return v;
}

bool AudioDevice::is_voice_protected(SoundHandle voice_handle)
{
    lockAudioMutex_internal();
    const int ch = getVoiceFromHandle_internal(voice_handle);
    if (ch == -1)
    {
        unlockAudioMutex_internal();
        return false;
    }
    const auto v = m_voice[ch]->flags.Protected;
    unlockAudioMutex_internal();
    return v;
}

int AudioDevice::findFreeVoice_internal()
{
    size_t lowest_play_index_value = 0xffffffff;
    int    lowest_play_index       = -1;

    // (slowly) drag the highest active voice index down
    if (m_highest_voice > 0 && m_voice[m_highest_voice - 1] == nullptr)
    {
        m_highest_voice--;
    }

    for (size_t i = 0; i < cer::voice_count; ++i)
    {
        if (m_voice[i] == nullptr)
        {
            if (i + 1 > m_highest_voice)
            {
                m_highest_voice = i + 1;
            }
            return i;
        }

        if (!m_voice[i]->flags.Protected && m_voice[i]->play_index < lowest_play_index_value)
        {
            lowest_play_index_value = m_voice[i]->play_index;
            lowest_play_index       = i;
        }
    }
    stopVoice_internal(lowest_play_index);
    return lowest_play_index;
}

size_t AudioDevice::getLoopCount(SoundHandle voice_handle)
{
    lockAudioMutex_internal();
    const int ch = getVoiceFromHandle_internal(voice_handle);
    if (ch == -1)
    {
        unlockAudioMutex_internal();
        return 0;
    }
    const int v = m_voice[ch]->loop_count;
    unlockAudioMutex_internal();
    return v;
}

// Returns current backend channel count (1 mono, 2 stereo, etc)
size_t AudioDevice::backend_channels() const
{
    return m_channels;
}

// Returns current backend sample rate
size_t AudioDevice::backend_sample_rate() const
{
    return m_samplerate;
}

// Returns current backend buffer size
size_t AudioDevice::backend_buffer_size() const
{
    return m_buffer_size;
}

// Get speaker position in 3d space
Vector3 AudioDevice::speaker_position(size_t channel) const
{
    return m_3d_speaker_position.at(channel);
}

void AudioDevice::setPostClipScaler(float aScaler)
{
    m_post_clip_scaler = aScaler;
}

void AudioDevice::setMainResampler(Resampler aResampler)
{
    m_resampler = aResampler;
}

void AudioDevice::setGlobalVolume(float aVolume)
{
    m_global_volume_fader.mActive = 0;
    m_global_volume               = aVolume;
}

void AudioDevice::setRelativePlaySpeed(SoundHandle voice_handle, float aSpeed)
{
    FOR_ALL_VOICES_PRE
    m_voice[ch]->relative_play_speed_fader.mActive = 0;
    setVoiceRelativePlaySpeed_internal(ch, aSpeed);
    FOR_ALL_VOICES_POST
}

void AudioDevice::setSamplerate(SoundHandle voice_handle, float aSamplerate)
{
    FOR_ALL_VOICES_PRE
    m_voice[ch]->base_sample_rate = aSamplerate;
    updateVoiceRelativePlaySpeed_internal(ch);
    FOR_ALL_VOICES_POST
}

void AudioDevice::setPause(SoundHandle voice_handle, bool aPause)
{
    FOR_ALL_VOICES_PRE
    setVoicePause_internal(ch, aPause);
    FOR_ALL_VOICES_POST
}

void AudioDevice::setMaxActiveVoiceCount(size_t aVoiceCount)
{
    assert(aVoiceCount > 0);
    assert(aVoiceCount <= cer::voice_count);

    lockAudioMutex_internal();
    m_max_active_voices = aVoiceCount;

    m_resample_data.resize(aVoiceCount * 2);
    m_resample_data_owner.resize(aVoiceCount);

    m_resample_data_buffer =
        AlignedFloatBuffer{sample_granularity * max_channels * aVoiceCount * 2};

    for (size_t i = 0; i < aVoiceCount * 2; ++i)
        m_resample_data[i] =
            m_resample_data_buffer.data() + (sample_granularity * max_channels * i);

    for (size_t i = 0; i < aVoiceCount; ++i)
        m_resample_data_owner[i] = nullptr;

    m_active_voice_dirty = true;
    unlockAudioMutex_internal();
}

void AudioDevice::setPauseAll(bool aPause)
{
    lockAudioMutex_internal();
    for (size_t ch = 0; ch < m_highest_voice; ++ch)
    {
        setVoicePause_internal(ch, aPause);
    }
    unlockAudioMutex_internal();
}

void AudioDevice::setProtectVoice(SoundHandle voice_handle, bool protect)
{
    FOR_ALL_VOICES_PRE
    m_voice[ch]->flags.Protected = protect;
    FOR_ALL_VOICES_POST
}

void AudioDevice::setPan(SoundHandle voice_handle, float pan)
{
    FOR_ALL_VOICES_PRE
    setVoicePan_internal(ch, pan);
    FOR_ALL_VOICES_POST
}

void AudioDevice::setChannelVolume(SoundHandle voice_handle, size_t channel, float volume)
{
    FOR_ALL_VOICES_PRE
    if (m_voice[ch]->channel_count > channel)
    {
        m_voice[ch]->channel_volume[channel] = volume;
    }
    FOR_ALL_VOICES_POST
}

void AudioDevice::setPanAbsolute(SoundHandle voice_handle, float aLVolume, float aRVolume)
{
    FOR_ALL_VOICES_PRE
    m_voice[ch]->pan_fader.mActive = 0;
    m_voice[ch]->channel_volume[0] = aLVolume;
    m_voice[ch]->channel_volume[1] = aRVolume;
    if (m_voice[ch]->channel_count == 4)
    {
        m_voice[ch]->channel_volume[2] = aLVolume;
        m_voice[ch]->channel_volume[3] = aRVolume;
    }
    if (m_voice[ch]->channel_count == 6)
    {
        m_voice[ch]->channel_volume[2] = (aLVolume + aRVolume) * 0.5f;
        m_voice[ch]->channel_volume[3] = (aLVolume + aRVolume) * 0.5f;
        m_voice[ch]->channel_volume[4] = aLVolume;
        m_voice[ch]->channel_volume[5] = aRVolume;
    }
    if (m_voice[ch]->channel_count == 8)
    {
        m_voice[ch]->channel_volume[2] = (aLVolume + aRVolume) * 0.5f;
        m_voice[ch]->channel_volume[3] = (aLVolume + aRVolume) * 0.5f;
        m_voice[ch]->channel_volume[4] = aLVolume;
        m_voice[ch]->channel_volume[5] = aRVolume;
        m_voice[ch]->channel_volume[6] = aLVolume;
        m_voice[ch]->channel_volume[7] = aRVolume;
    }
    FOR_ALL_VOICES_POST
}

void AudioDevice::setInaudibleBehavior(SoundHandle voice_handle, bool aMustTick, bool aKill)
{
    FOR_ALL_VOICES_PRE
    m_voice[ch]->flags.InaudibleKill = aKill;
    m_voice[ch]->flags.InaudibleTick = aMustTick;
    FOR_ALL_VOICES_POST
}

void AudioDevice::setLoopPoint(SoundHandle voice_handle, SoundTime aLoopPoint)
{
    FOR_ALL_VOICES_PRE
    m_voice[ch]->loop_point = aLoopPoint;
    FOR_ALL_VOICES_POST
}

void AudioDevice::setLooping(SoundHandle voice_handle, bool aLooping)
{
    FOR_ALL_VOICES_PRE
    m_voice[ch]->flags.Looping = aLooping;
    FOR_ALL_VOICES_POST
}

void AudioDevice::setAutoStop(SoundHandle voice_handle, bool aAutoStop)
{
    FOR_ALL_VOICES_PRE
    m_voice[ch]->flags.DisableAutostop = !aAutoStop;
    FOR_ALL_VOICES_POST
}

void AudioDevice::setVolume(SoundHandle voice_handle, float aVolume)
{
    FOR_ALL_VOICES_PRE
    m_voice[ch]->volume_fader.mActive = 0;
    setVoiceVolume_internal(ch, aVolume);
    FOR_ALL_VOICES_POST
}

void AudioDevice::setDelaySamples(SoundHandle voice_handle, size_t aSamples)
{
    FOR_ALL_VOICES_PRE
    m_voice[ch]->delay_samples = aSamples;
    FOR_ALL_VOICES_POST
}

void AudioDevice::setVisualizationEnable(bool aEnable)
{
    m_flags.enable_visualization = aEnable;
}

void AudioDevice::speaker_position(size_t channel, Vector3 value)
{
    m_3d_speaker_position.at(channel) = value;
}

struct mat3 : std::array<Vector3, 3>
{
};

static Vector3 operator*(const mat3& m, const Vector3& a)
{
    return {
        m[0].x * a.x + m[0].y * a.y + m[0].z * a.z,
        m[1].x * a.x + m[1].y * a.y + m[1].z * a.z,
        m[2].x * a.x + m[2].y * a.y + m[2].z * a.z,
    };
}

static mat3 lookatRH(const Vector3& at, Vector3 up)
{
    const auto z = normalize(at);
    const auto x = normalize(cross(up, z));
    const auto y = cross(z, x);

    return {x, y, z};
}

#ifndef MIN
#define MIN(a, b) ((a) < (b)) ? (a) : (b)
#endif

#ifndef MAX
#define MAX(a, b) ((a) > (b)) ? (a) : (b)
#endif

float doppler(Vector3        aDeltaPos,
              const Vector3& aSrcVel,
              const Vector3& aDstVel,
              float          aFactor,
              float          aSoundSpeed)
{
    float deltamag = length(aDeltaPos);
    if (deltamag == 0)
        return 1.0f;
    float vls      = dot(aDeltaPos, aDstVel) / deltamag;
    float vss      = dot(aDeltaPos, aSrcVel) / deltamag;
    float maxspeed = aSoundSpeed / aFactor;
    vss            = MIN(vss, maxspeed);
    vls            = MIN(vls, maxspeed);
    return (aSoundSpeed - aFactor * vls) / (aSoundSpeed - aFactor * vss);
}

float attenuateInvDistance(float aDistance,
                           float aMinDistance,
                           float aMaxDistance,
                           float aRolloffFactor)
{
    float distance = MAX(aDistance, aMinDistance);
    distance       = MIN(distance, aMaxDistance);
    return aMinDistance / (aMinDistance + aRolloffFactor * (distance - aMinDistance));
}

float attenuateLinearDistance(float aDistance,
                              float aMinDistance,
                              float aMaxDistance,
                              float aRolloffFactor)
{
    float distance = MAX(aDistance, aMinDistance);
    distance       = MIN(distance, aMaxDistance);
    return 1 - aRolloffFactor * (distance - aMinDistance) / (aMaxDistance - aMinDistance);
}

float attenuateExponentialDistance(float aDistance,
                                   float aMinDistance,
                                   float aMaxDistance,
                                   float aRolloffFactor)
{
    float distance = MAX(aDistance, aMinDistance);
    distance       = MIN(distance, aMaxDistance);
    return pow(distance / aMinDistance, -aRolloffFactor);
}

void AudioDevice::update3dVoices_internal(std::span<const size_t> voiceList)
{
    auto speaker = std::array<Vector3, max_channels>{};

    for (size_t i = 0; i < m_channels; ++i)
        speaker[i] = normalize(m_3d_speaker_position[i]);

    const auto lpos = m_3d_position;
    const auto lvel = m_3d_velocity;
    const auto at   = m_3d_at;
    const auto up   = m_3d_up;
    const auto m    = lookatRH(at, up);

    for (const size_t voice_id : voiceList)
    {
        auto& v = m_3d_data[voice_id];

        auto vol = v.collider != nullptr ? v.collider->collide(this, v, v.collider_data) : 1.0f;

        auto       pos = v.position_3d;
        const auto vel = v.velocity_3d;

        if (!v.flags.ListenerRelative)
        {
            pos = pos - lpos;
        }

        const float dist = length(pos);

        // attenuation

        if (v.attenuator != nullptr)
        {
            vol *= v.attenuator->attenuate(dist,
                                           v.min_distance_3d,
                                           v.max_distance_3d,
                                           v.attenuation_rolloff_3d);
        }
        else
        {
            switch (v.attenuation_model_3d)
            {
                case AttenuationModel::InverseDistance:
                    vol *= attenuateInvDistance(dist,
                                                v.min_distance_3d,
                                                v.max_distance_3d,
                                                v.attenuation_rolloff_3d);
                    break;
                case AttenuationModel::LinearDistance:
                    vol *= attenuateLinearDistance(dist,
                                                   v.min_distance_3d,
                                                   v.max_distance_3d,
                                                   v.attenuation_rolloff_3d);
                    break;
                case AttenuationModel::ExponentialDistance:
                    vol *= attenuateExponentialDistance(dist,
                                                        v.min_distance_3d,
                                                        v.max_distance_3d,
                                                        v.attenuation_rolloff_3d);
                    break;
                default:
                    // case AudioSource::NO_ATTENUATION:
                    break;
            }
        }

        // cone
        // (todo) vol *= conev;

        // doppler
        v.doppler_value = doppler(pos, vel, lvel, v.doppler_factor_3d, m_3d_sound_speed);

        // panning
        pos = normalize(m * pos);

        v.channel_volume = {};

        // Apply volume to channels based on speaker vectors
        for (size_t j = 0; j < m_channels; ++j)
        {
            float speakervol = (dot(speaker[j], pos) + 1) / 2;
            if (is_zero(speaker[j]))
                speakervol = 1;
            // Different speaker "focus" calculations to try, if the default "bleeds" too much..
            // speakervol = (speakervol * speakervol + speakervol) / 2;
            // speakervol = speakervol * speakervol;
            v.channel_volume[j] = vol * speakervol;
        }

        v.volume_3d = vol;
    }
}

void AudioDevice::update3dAudio()
{
    size_t voicecount = 0;
    size_t voices[cer::voice_count];

    // Step 1 - find voices that need 3d processing
    lockAudioMutex_internal();
    for (size_t i = 0; i < m_highest_voice; ++i)
    {
        if (m_voice[i] && m_voice[i]->flags.Process3D)
        {
            voices[voicecount] = i;
            voicecount++;
            m_3d_data[i].flags = m_voice[i]->flags;
        }
    }
    unlockAudioMutex_internal();

    // Step 2 - do 3d processing

    update3dVoices_internal({voices, voicecount});

    // Step 3 - update SoLoud voices

    lockAudioMutex_internal();
    for (size_t i = 0; i < voicecount; ++i)
    {
        auto& v = m_3d_data[voices[i]];

        if (const auto& vi = m_voice[voices[i]])
        {
            updateVoiceRelativePlaySpeed_internal(voices[i]);
            updateVoiceVolume_internal(voices[i]);
            for (size_t j = 0; j < max_channels; ++j)
            {
                vi->channel_volume[j] = v.channel_volume[j];
            }

            if (vi->overall_volume < 0.001f)
            {
                // Inaudible.
                vi->flags.Inaudible = true;

                if (vi->flags.InaudibleKill)
                {
                    stopVoice_internal(voices[i]);
                }
            }
            else
            {
                vi->flags.Inaudible = false;
            }
        }
    }

    m_active_voice_dirty = true;
    unlockAudioMutex_internal();
}


SoundHandle AudioDevice::play3d(
    AudioSource& sound, Vector3 pos, Vector3 vel, float volume, bool paused, size_t bus)
{
    const SoundHandle h = play(sound, volume, 0, true, bus);
    lockAudioMutex_internal();
    auto v = getVoiceFromHandle_internal(h);

    if (v < 0)
    {
        unlockAudioMutex_internal();
        return h;
    }

    m_3d_data[v].handle         = h;
    m_voice[v]->flags.Process3D = true;

    set3dSourceParameters(h, pos, vel);

    int samples = 0;
    if (sound.distance_delay)
    {
        const auto corrected_pos = m_voice[v]->flags.ListenerRelative ? pos : pos - m_3d_position;

        const float dist = length(corrected_pos);
        samples += int(floor(dist / m_3d_sound_speed * float(m_samplerate)));
    }

    update3dVoices_internal({reinterpret_cast<size_t*>(&v), 1});
    updateVoiceRelativePlaySpeed_internal(v);

    for (size_t j = 0; j < max_channels; ++j)
    {
        m_voice[v]->channel_volume[j] = m_3d_data[v].channel_volume[j];
    }

    updateVoiceVolume_internal(v);

    // Fix initial voice volume ramp up
    for (size_t i = 0; i < max_channels; ++i)
    {
        m_voice[v]->current_channel_volume[i] =
            m_voice[v]->channel_volume[i] * m_voice[v]->overall_volume;
    }

    if (m_voice[v]->overall_volume < 0.01f)
    {
        // Inaudible.
        m_voice[v]->flags.Inaudible = true;

        if (m_voice[v]->flags.InaudibleKill)
        {
            stopVoice_internal(v);
        }
    }
    else
    {
        m_voice[v]->flags.Inaudible = false;
    }

    m_active_voice_dirty = true;

    unlockAudioMutex_internal();
    setDelaySamples(h, samples);
    setPause(h, paused);

    return h;
}

SoundHandle AudioDevice::play3d_clocked(
    SoundTime sound_time, AudioSource& sound, Vector3 pos, Vector3 vel, float volume, size_t bus)
{
    const SoundHandle h = play(sound, volume, 0, 1, bus);
    lockAudioMutex_internal();
    int v = getVoiceFromHandle_internal(h);
    if (v < 0)
    {
        unlockAudioMutex_internal();
        return h;
    }
    m_3d_data[v].handle         = h;
    m_voice[v]->flags.Process3D = true;
    set3dSourceParameters(h, pos, vel);
    SoundTime lasttime = m_last_clocked_time;
    if (lasttime == 0)
    {
        lasttime            = sound_time;
        m_last_clocked_time = sound_time;
    }
    unlockAudioMutex_internal();

    auto samples = int(floor((sound_time - lasttime) * m_samplerate));

    // Make sure we don't delay too much (or overflow)
    if (samples < 0 || samples > 2048)
    {
        samples = 0;
    }

    if (sound.distance_delay)
    {
        const float dist = length(pos);
        samples += int(floor((dist / m_3d_sound_speed) * m_samplerate));
    }

    update3dVoices_internal({reinterpret_cast<size_t*>(&v), 1});
    lockAudioMutex_internal();
    updateVoiceRelativePlaySpeed_internal(v);

    for (size_t j = 0; j < max_channels; ++j)
    {
        m_voice[v]->channel_volume[j] = m_3d_data[v].channel_volume[j];
    }

    updateVoiceVolume_internal(v);

    // Fix initial voice volume ramp up
    for (size_t i = 0; i < max_channels; ++i)
    {
        m_voice[v]->current_channel_volume[i] =
            m_voice[v]->channel_volume[i] * m_voice[v]->overall_volume;
    }

    if (m_voice[v]->overall_volume < 0.01f)
    {
        // Inaudible.
        m_voice[v]->flags.Inaudible = true;

        if (m_voice[v]->flags.InaudibleKill)
        {
            stopVoice_internal(v);
        }
    }
    else
    {
        m_voice[v]->flags.Inaudible = false;
    }

    m_active_voice_dirty = true;
    unlockAudioMutex_internal();

    setDelaySamples(h, samples);
    setPause(h, false);

    return h;
}


void AudioDevice::set3dSoundSpeed(float aSpeed)
{
    assert(aSpeed > 0.0f);
    m_3d_sound_speed = aSpeed;
}


float AudioDevice::get3dSoundSpeed() const
{
    return m_3d_sound_speed;
}


void AudioDevice::set3dListenerParameters(Vector3 pos, Vector3 at, Vector3 up, Vector3 velocity)
{
    m_3d_position = pos;
    m_3d_at       = at;
    m_3d_up       = up;
    m_3d_velocity = velocity;
}


void AudioDevice::set3dListenerPosition(Vector3 value)
{
    m_3d_position = value;
}


void AudioDevice::set3dListenerAt(Vector3 value)
{
    m_3d_at = value;
}


void AudioDevice::set3dListenerUp(Vector3 value)
{
    m_3d_up = value;
}


void AudioDevice::set3dListenerVelocity(Vector3 value)
{
    m_3d_velocity = value;
}


void AudioDevice::set3dSourceParameters(SoundHandle voice_handle, Vector3 aPos, Vector3 aVelocity)
{
    FOR_ALL_VOICES_PRE_3D
    m_3d_data[ch].position_3d = aPos;
    m_3d_data[ch].velocity_3d = aVelocity;
    FOR_ALL_VOICES_POST_3D
}


void AudioDevice::set3dSourcePosition(SoundHandle voice_handle, Vector3 value)
{
    FOR_ALL_VOICES_PRE_3D
    m_3d_data[ch].position_3d = value;
    FOR_ALL_VOICES_POST_3D
}


void AudioDevice::set3dSourceVelocity(SoundHandle voice_handle, Vector3 velocity)
{
    FOR_ALL_VOICES_PRE_3D
    m_3d_data[ch].velocity_3d = velocity;
    FOR_ALL_VOICES_POST_3D
}


void AudioDevice::set3dSourceMinMaxDistance(SoundHandle voice_handle,
                                            float       aMinDistance,
                                            float       aMaxDistance)
{
    FOR_ALL_VOICES_PRE_3D
    m_3d_data[ch].min_distance_3d = aMinDistance;
    m_3d_data[ch].max_distance_3d = aMaxDistance;
    FOR_ALL_VOICES_POST_3D
}


void AudioDevice::set3dSourceAttenuation(SoundHandle      voice_handle,
                                         AttenuationModel aAttenuationModel,
                                         float            aAttenuationRolloffFactor)
{
    FOR_ALL_VOICES_PRE_3D
    m_3d_data[ch].attenuation_model_3d   = aAttenuationModel;
    m_3d_data[ch].attenuation_rolloff_3d = aAttenuationRolloffFactor;
    FOR_ALL_VOICES_POST_3D
}


void AudioDevice::set3dSourceDopplerFactor(SoundHandle voice_handle, float aDopplerFactor)
{
    FOR_ALL_VOICES_PRE_3D
    m_3d_data[ch].doppler_factor_3d = aDopplerFactor;
    FOR_ALL_VOICES_POST_3D
}

void AudioDevice::setGlobalFilter(size_t aFilterId, Filter* aFilter)
{
    if (aFilterId >= filters_per_stream)
    {
        return;
    }

    lockAudioMutex_internal();

    m_filter[aFilterId] = aFilter;
    if (aFilter)
    {
        m_filter_instance[aFilterId] = m_filter[aFilterId]->create_instance();
    }

    unlockAudioMutex_internal();
}

std::optional<float> AudioDevice::filter_parameter(SoundHandle voice_handle,
                                                   size_t      filter_id,
                                                   size_t      attribute_id)
{
    if (filter_id >= filters_per_stream)
    {
        return {};
    }

    auto ret = std::optional<float>{};

    if (voice_handle == 0)
    {
        lockAudioMutex_internal();
        if (m_filter_instance[filter_id])
        {
            ret = m_filter_instance[filter_id]->filter_parameter(attribute_id);
        }
        unlockAudioMutex_internal();
        return ret;
    }

    const int ch = getVoiceFromHandle_internal(voice_handle);
    if (ch == -1)
    {
        return ret;
    }

    lockAudioMutex_internal();
    if (m_voice[ch] && m_voice[ch]->filter[filter_id])
    {
        ret = m_voice[ch]->filter[filter_id]->filter_parameter(attribute_id);
    }
    unlockAudioMutex_internal();

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
        lockAudioMutex_internal();
        if (m_filter_instance[filter_id])
        {
            m_filter_instance[filter_id]->set_filter_parameter(attribute_id, value);
        }
        unlockAudioMutex_internal();
        return;
    }

    FOR_ALL_VOICES_PRE
    if (m_voice[ch] && m_voice[ch]->filter[filter_id])
    {
        m_voice[ch]->filter[filter_id]->set_filter_parameter(attribute_id, value);
    }
    FOR_ALL_VOICES_POST
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
        lockAudioMutex_internal();
        if (m_filter_instance[aFilterId])
        {
            m_filter_instance[aFilterId]->fade_filter_parameter(attribute_id,
                                                                to,
                                                                time,
                                                                m_stream_time);
        }
        unlockAudioMutex_internal();
        return;
    }

    FOR_ALL_VOICES_PRE
    if (m_voice[ch] && m_voice[ch]->filter[aFilterId])
    {
        m_voice[ch]->filter[aFilterId]->fade_filter_parameter(attribute_id,
                                                              to,
                                                              time,
                                                              m_stream_time);
    }
    FOR_ALL_VOICES_POST
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
        lockAudioMutex_internal();
        if (m_filter_instance[filter_id])
        {
            m_filter_instance[filter_id]->oscillate_filter_parameter(attribute_id,
                                                                     from,
                                                                     to,
                                                                     time,
                                                                     m_stream_time);
        }
        unlockAudioMutex_internal();
        return;
    }

    FOR_ALL_VOICES_PRE
    if (m_voice[ch] && m_voice[ch]->filter[filter_id])
    {
        m_voice[ch]->filter[filter_id]->oscillate_filter_parameter(attribute_id,
                                                                   from,
                                                                   to,
                                                                   time,
                                                                   m_stream_time);
    }
    FOR_ALL_VOICES_POST
}

// Create a voice group. Returns 0 if unable (out of voice groups / out of memory)
SoundHandle AudioDevice::createVoiceGroup()
{
    lockAudioMutex_internal();

    size_t i;
    // Check if there's any deleted voice groups and re-use if found
    for (i = 0; i < m_voice_group_count; ++i)
    {
        if (m_voice_group[i] == nullptr)
        {
            m_voice_group[i] = new size_t[17];
            if (m_voice_group[i] == nullptr)
            {
                unlockAudioMutex_internal();
                return 0;
            }
            m_voice_group[i][0] = 16;
            m_voice_group[i][1] = 0;
            unlockAudioMutex_internal();
            return 0xfffff000 | i;
        }
    }
    if (m_voice_group_count == 4096)
    {
        unlockAudioMutex_internal();
        return 0;
    }
    size_t oldcount = m_voice_group_count;
    if (m_voice_group_count == 0)
    {
        m_voice_group_count = 4;
    }
    m_voice_group_count *= 2;
    size_t** vg = new size_t*[m_voice_group_count];
    if (vg == nullptr)
    {
        m_voice_group_count = oldcount;
        unlockAudioMutex_internal();
        return 0;
    }
    for (i = 0; i < oldcount; ++i)
    {
        vg[i] = m_voice_group[i];
    }

    for (; i < m_voice_group_count; ++i)
    {
        vg[i] = nullptr;
    }

    delete[] m_voice_group;
    m_voice_group    = vg;
    i                = oldcount;
    m_voice_group[i] = new size_t[17];
    if (m_voice_group[i] == nullptr)
    {
        unlockAudioMutex_internal();
        return 0;
    }
    m_voice_group[i][0] = 16;
    m_voice_group[i][1] = 0;
    unlockAudioMutex_internal();
    return 0xfffff000 | i;
}

// Destroy a voice group.
void AudioDevice::destroyVoiceGroup(SoundHandle aVoiceGroupHandle)
{
    if (!isVoiceGroup(aVoiceGroupHandle))
        return;

    const int c = aVoiceGroupHandle & 0xfff;

    lockAudioMutex_internal();
    delete[] m_voice_group[c];
    m_voice_group[c] = nullptr;
    unlockAudioMutex_internal();
}

// Add a voice handle to a voice group
void AudioDevice::addVoiceToGroup(SoundHandle aVoiceGroupHandle, SoundHandle voice_handle)
{
    if (!isVoiceGroup(aVoiceGroupHandle))
        return;

    // Don't consider adding invalid voice handles as an error, since the voice may just have ended.
    if (!is_valid_voice_handle(voice_handle))
        return;

    trimVoiceGroup_internal(aVoiceGroupHandle);

    const int c = aVoiceGroupHandle & 0xfff;

    lockAudioMutex_internal();

    for (size_t i = 1; i < m_voice_group[c][0]; ++i)
    {
        if (m_voice_group[c][i] == voice_handle)
        {
            unlockAudioMutex_internal();
            return; // already there
        }

        if (m_voice_group[c][i] == 0)
        {
            m_voice_group[c][i]     = voice_handle;
            m_voice_group[c][i + 1] = 0;

            unlockAudioMutex_internal();
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
    unlockAudioMutex_internal();
}

// Is this handle a valid voice group?
bool AudioDevice::isVoiceGroup(SoundHandle aVoiceGroupHandle)
{
    if ((aVoiceGroupHandle & 0xfffff000) != 0xfffff000)
    {
        return false;
    }

    const size_t c = aVoiceGroupHandle & 0xfff;
    if (c >= m_voice_group_count)
    {
        return false;
    }

    lockAudioMutex_internal();
    const bool res = m_voice_group[c] != nullptr;
    unlockAudioMutex_internal();

    return res;
}

// Is this voice group empty?
bool AudioDevice::isVoiceGroupEmpty(SoundHandle aVoiceGroupHandle)
{
    // If not a voice group, yeah, we're empty alright..
    if (!isVoiceGroup(aVoiceGroupHandle))
    {
        return true;
    }

    trimVoiceGroup_internal(aVoiceGroupHandle);
    const int c = aVoiceGroupHandle & 0xfff;

    lockAudioMutex_internal();
    const bool res = m_voice_group[c][1] == 0;
    unlockAudioMutex_internal();

    return res;
}

// Remove all non-active voices from group
void AudioDevice::trimVoiceGroup_internal(SoundHandle aVoiceGroupHandle)
{
    if (!isVoiceGroup(aVoiceGroupHandle))
    {
        return;
    }

    const int c = aVoiceGroupHandle & 0xfff;

    lockAudioMutex_internal();

    // empty group
    if (m_voice_group[c][1] == 0)
    {
        unlockAudioMutex_internal();
        return;
    }

    // first item in voice group is number of allocated indices
    for (size_t i = 1; i < m_voice_group[c][0]; ++i)
    {
        // If we hit a voice in the group that's not set, we're done
        if (m_voice_group[c][i] == 0)
        {
            unlockAudioMutex_internal();
            return;
        }

        unlockAudioMutex_internal();
        while (!is_valid_voice_handle(
            m_voice_group[c][i])) // function locks mutex, so we need to unlock it before the call
        {
            lockAudioMutex_internal();
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
                unlockAudioMutex_internal();
                return;
            }
            unlockAudioMutex_internal();
        }
        lockAudioMutex_internal();
    }
    unlockAudioMutex_internal();
}

SoundHandle* AudioDevice::voiceGroupHandleToArray_internal(SoundHandle aVoiceGroupHandle) const
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

void AudioDevice::setVoiceRelativePlaySpeed_internal(size_t aVoice, float aSpeed)
{
    assert(aVoice < cer::voice_count);
    assert(m_inside_audio_thread_mutex);
    assert(aSpeed > 0.0f);

    if (m_voice[aVoice])
    {
        m_voice[aVoice]->set_relative_play_speed = aSpeed;
        updateVoiceRelativePlaySpeed_internal(aVoice);
    }
}

void AudioDevice::setVoicePause_internal(size_t aVoice, int aPause)
{
    assert(aVoice < cer::voice_count);
    assert(m_inside_audio_thread_mutex);
    m_active_voice_dirty = true;

    if (m_voice[aVoice])
    {
        m_voice[aVoice]->pause_scheduler.mActive = 0;
        m_voice[aVoice]->flags.Paused            = aPause;
    }
}

void AudioDevice::setVoicePan_internal(size_t aVoice, float aPan)
{
    assert(aVoice < cer::voice_count);
    assert(m_inside_audio_thread_mutex);
    if (m_voice[aVoice])
    {
        m_voice[aVoice]->pan               = aPan;
        const auto l                       = float(std::cos((aPan + 1) * M_PI / 4));
        const auto r                       = float(std::sin((aPan + 1) * M_PI / 4));
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

void AudioDevice::setVoiceVolume_internal(size_t aVoice, float aVolume)
{
    assert(aVoice < cer::voice_count);
    assert(m_inside_audio_thread_mutex);
    m_active_voice_dirty = true;
    if (m_voice[aVoice])
    {
        m_voice[aVoice]->set_volume = aVolume;
        updateVoiceVolume_internal(aVoice);
    }
}

void AudioDevice::stopVoice_internal(size_t aVoice)
{
    assert(aVoice < cer::voice_count);
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

void AudioDevice::updateVoiceRelativePlaySpeed_internal(size_t aVoice)
{
    assert(aVoice < cer::voice_count);
    assert(m_inside_audio_thread_mutex);
    m_voice[aVoice]->overall_relative_play_speed =
        m_3d_data[aVoice].doppler_value * m_voice[aVoice]->set_relative_play_speed;
    m_voice[aVoice]->sample_rate =
        m_voice[aVoice]->base_sample_rate * m_voice[aVoice]->overall_relative_play_speed;
}

void AudioDevice::updateVoiceVolume_internal(size_t aVoice)
{
    assert(aVoice < cer::voice_count);
    assert(m_inside_audio_thread_mutex);
    m_voice[aVoice]->overall_volume = m_voice[aVoice]->set_volume * m_3d_data[aVoice].volume_3d;
    if (m_voice[aVoice]->flags.Paused)
    {
        for (size_t i = 0; i < max_channels; ++i)
        {
            m_voice[aVoice]->current_channel_volume[i] =
                m_voice[aVoice]->channel_volume[i] * m_voice[aVoice]->overall_volume;
        }
    }
}
} // namespace cer
