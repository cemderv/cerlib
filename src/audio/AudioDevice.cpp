// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "AudioDevice.hpp"

#include "SoundChannelImpl.hpp"
#include "SoundImpl.hpp"
#include "cerlib/Logging.hpp"
#include "cerlib/SoundChannel.hpp"
#include "soloud_fft.hpp"
#include "soloud_internal.hpp"
#include "soloud_thread.hpp"
#include "soloud_misc.hpp"
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
auto to_soloud_time(const SoundTime& seconds) -> cer::time_t
{
    return seconds.count();
}

// AudioDevice::AudioDevice(bool& success)
// {
//     try
//     {
//         m_soloud                       = std::make_unique<Engine>();
//         m_was_initialized_successfully = true;
//         success                        = true;
//     }
//     catch (const std::exception& ex)
//     {
//         log_info("Failed to initialize the internal audio engine. Reason: {}", ex.what());
//     }
// }

AudioDevice::AudioDevice(EngineFlags           flags,
                         std::optional<size_t> sample_rate,
                         std::optional<size_t> buffer_size,
                         size_t                channels)
    : mFlags(flags)
{
    assert(channels != 3 && channels != 5 && channels != 7);
    assert(channels <= max_channels);

    mAudioThreadMutex = Thread::createMutex();

    int samplerate = sample_rate.value_or(44100);
    int buffersize = buffer_size.value_or(2048);

#if defined(WITH_SDL2_STATIC)
    {
        if (!aBufferSize.has_value())
            buffersize = 2048;

        sdl2static_init(this, flags, samplerate, buffersize, aChannels);
    }
#endif

#if defined(WITH_XAUDIO2)
    {
        if (!aBufferSize.has_value())
            buffersize = 4096;

        xaudio2_init(this, flags, samplerate, buffersize, aChannels);
    }
#endif

#if defined(WITH_WINMM)
    {
        if (!aBufferSize.has_value())
            buffersize = 4096;

        winmm_init(this, flags, samplerate, buffersize, aChannels);
    }
#endif

#if defined(WITH_WASAPI)
    {
        if (!aBufferSize.has_value())
            buffersize = 4096;

        if (!aSamplerate.has_value())
            samplerate = 48000;

        wasapi_init(this, flags, samplerate, buffersize, aChannels);
    }
#endif

#if defined(WITH_ALSA)
    {
        if (!aBufferSize.has_value())
            buffersize = 2048;

        alsa_init(this, flags, samplerate, buffersize, aChannels);
    }
#endif

#if defined(WITH_COREAUDIO)
    {
        if (!buffer_size.has_value())
            buffersize = 2048;

        coreaudio_init(this, flags, samplerate, buffersize, channels);
    }
#endif

#if defined(WITH_OPENSLES)
    {
        if (!aBufferSize.has_value())
            buffersize = 4096;

        opensles_init(this, flags, samplerate, buffersize, aChannels);
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
    stopAll();

    // Make sure no audio operation is currently pending
    lockAudioMutex_internal();
    unlockAudioMutex_internal();
    assert(!mInsideAudioThreadMutex);
    stopAll();
    if (mBackendCleanupFunc)
        mBackendCleanupFunc(this);
    mBackendCleanupFunc = 0;
    if (mAudioThreadMutex)
        Thread::destroyMutex(mAudioThreadMutex);
    mAudioThreadMutex = nullptr;

    for (size_t i = 0; i < filters_per_stream; ++i)
    {
        mFilterInstance[i].reset();
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
        delay
            ? play_clocked(to_soloud_time(*delay),
                           sound.impl()->soloud_audio_source(),
                           volume,
                           pan)
            : play(sound.impl()->soloud_audio_source(), volume, pan, start_paused);

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
        play_clocked(to_soloud_time(*delay),
                     sound.impl()->soloud_audio_source(),
                     volume,
                     pan);
    }
    else
    {
        play(sound.impl()->soloud_audio_source(), volume, pan, false);
    }

    m_playing_sounds.insert(sound);
}

auto AudioDevice::play_sound_in_background(const Sound& sound,
                                           float        volume,
                                           bool         start_paused)
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
    stopAll();
}

void AudioDevice::pause_all_sounds()
{
    setPauseAll(true);
}

void AudioDevice::resume_all_sounds()
{
    setPauseAll(false);
}

auto AudioDevice::global_volume() const -> float
{
    return getGlobalVolume();
}

void AudioDevice::set_global_volume(float value)
{
    setGlobalVolume(value);
}

void AudioDevice::fade_global_volume(float to_volume, SoundTime fade_duration)
{
    fadeGlobalVolume(to_volume, to_soloud_time(fade_duration));
}

void AudioDevice::purge_sounds()
{
    std::erase_if(m_playing_sounds,
                  [this](const Sound& sound) {
                      return countAudioSource(sound.impl()->soloud_audio_source()) == 0;
                  });
}

auto AudioDevice::SoundHash::operator()(const Sound& sound) const -> size_t
{
    return reinterpret_cast<size_t>(sound.impl());
}

handle AudioDevice::play(AudioSource& sound, float volume, float pan, bool paused, size_t bus)
{
    if (sound.single_instance)
    {
        // Only one instance allowed, stop others
        sound.stop();
    }

    // Creation of an audio instance may take significant amount of time,
    // so let's not do it inside the audio thread mutex.
    sound.engine  = this;
    auto instance = sound.createInstance();

    lockAudioMutex_internal();
    int ch = findFreeVoice_internal();
    if (ch < 0)
    {
        unlockAudioMutex_internal();
        return 7; // TODO: this was "UNKNOWN_ERROR"
    }
    if (!sound.audio_source_id)
    {
        sound.audio_source_id = mAudioSourceID;
        mAudioSourceID++;
    }
    mVoice[ch]                 = instance;
    mVoice[ch]->mAudioSourceID = sound.audio_source_id;
    mVoice[ch]->mBusHandle     = bus;
    mVoice[ch]->init(sound, mPlayIndex);
    m3dData[ch] = AudioSourceInstance3dData{sound};

    mPlayIndex++;

    // 20 bits, skip the last one (top bits full = voice group)
    if (mPlayIndex == 0xfffff)
    {
        mPlayIndex = 0;
    }

    if (paused)
    {
        mVoice[ch]->mFlags.Paused = true;
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
        mVoice[ch]->mCurrentChannelVolume[i] =
            mVoice[ch]->mChannelVolume[i] * mVoice[ch]->mOverallVolume;
    }

    setVoiceRelativePlaySpeed_internal(ch, 1);

    for (size_t i = 0; i < filters_per_stream; ++i)
    {
        if (sound.filter[i])
        {
            mVoice[ch]->mFilter[i] = sound.filter[i]->createInstance();
        }
    }

    mActiveVoiceDirty = true;

    unlockAudioMutex_internal();

    return getHandleFromVoice_internal(ch);
}

handle AudioDevice::play_clocked(
    time_t       aSoundTime,
    AudioSource& aSound,
    float        aVolume,
    float        aPan,
    size_t       aBus)
{
    const handle h = play(aSound, aVolume, aPan, 1, aBus);
    lockAudioMutex_internal();
    // mLastClockedTime is cleared to zero at start of every output buffer
    time_t lasttime = mLastClockedTime;
    if (lasttime == 0)
    {
        mLastClockedTime = aSoundTime;
        lasttime         = aSoundTime;
    }
    unlockAudioMutex_internal();
    int samples = (int)floor((aSoundTime - lasttime) * mSamplerate);
    // Make sure we don't delay too much (or overflow)
    if (samples < 0 || samples > 2048)
        samples = 0;
    setDelaySamples(h, samples);
    setPause(h, false);
    return h;
}

handle AudioDevice::playBackground(AudioSource& sound, float volume, bool paused, size_t bus)
{
    const handle h = play(sound, volume, 0.0f, paused, bus);
    setPanAbsolute(h, 1.0f, 1.0f);
    return h;
}

bool AudioDevice::seek(handle aVoiceHandle, time_t aSeconds)
{
    bool res = true;


    FOR_ALL_VOICES_PRE
        const auto singleres = mVoice[ch]->seek(aSeconds, mScratch.mData, mScratchSize);
        if (!singleres)
            res = singleres;
    FOR_ALL_VOICES_POST
    return res;
}


void AudioDevice::stop(handle aVoiceHandle)
{
    FOR_ALL_VOICES_PRE
        stopVoice_internal(ch);
    FOR_ALL_VOICES_POST
}

void AudioDevice::stopAudioSource(AudioSource& aSound)
{
    if (aSound.audio_source_id)
    {
        lockAudioMutex_internal();

        for (size_t i = 0; i < mHighestVoice; ++i)
        {
            if (mVoice[i] && mVoice[i]->mAudioSourceID == aSound.audio_source_id)
            {
                stopVoice_internal(i);
            }
        }
        unlockAudioMutex_internal();
    }
}

void AudioDevice::stopAll()
{
    lockAudioMutex_internal();
    for (size_t i = 0; i < mHighestVoice; ++i)
    {
        stopVoice_internal(i);
    }
    unlockAudioMutex_internal();
}

int AudioDevice::countAudioSource(AudioSource& aSound)
{
    int count = 0;
    if (aSound.audio_source_id)
    {
        lockAudioMutex_internal();

        for (size_t i = 0; i < mHighestVoice; ++i)
        {
            if (mVoice[i] && mVoice[i]->mAudioSourceID == aSound.audio_source_id)
            {
                count++;
            }
        }
        unlockAudioMutex_internal();
    }
    return count;
}

void AudioDevice::schedulePause(handle aVoiceHandle, time_t aTime)
{
    if (aTime <= 0)
    {
        setPause(aVoiceHandle, 1);
        return;
    }
    FOR_ALL_VOICES_PRE
        mVoice[ch]
            ->
            mPauseScheduler.set(1, 0, aTime, mVoice[ch]->mStreamTime);
    FOR_ALL_VOICES_POST
}

void AudioDevice::scheduleStop(handle aVoiceHandle, time_t aTime)
{
    if (aTime <= 0)
    {
        stop(aVoiceHandle);
        return;
    }
    FOR_ALL_VOICES_PRE
        mVoice[ch]
            ->
            mStopScheduler.set(1, 0, aTime, mVoice[ch]->mStreamTime);
    FOR_ALL_VOICES_POST
}

void AudioDevice::fadeVolume(handle aVoiceHandle, float aTo, time_t aTime)
{
    float from = getVolume(aVoiceHandle);
    if (aTime <= 0 || aTo == from)
    {
        setVolume(aVoiceHandle, aTo);
        return;
    }

    FOR_ALL_VOICES_PRE
        mVoice[ch]
            ->
            mVolumeFader.set(from, aTo, aTime, mVoice[ch]->mStreamTime);
    FOR_ALL_VOICES_POST
}

void AudioDevice::fadePan(handle aVoiceHandle, float aTo, time_t aTime)
{
    const float from = getPan(aVoiceHandle);

    if (aTime <= 0 || aTo == from)
    {
        setPan(aVoiceHandle, aTo);
        return;
    }

    FOR_ALL_VOICES_PRE
        mVoice[ch]
            ->
            mPanFader.set(from, aTo, aTime, mVoice[ch]->mStreamTime);
    FOR_ALL_VOICES_POST
}

void AudioDevice::fadeRelativePlaySpeed(handle aVoiceHandle, float aTo, time_t aTime)
{
    const float from = getRelativePlaySpeed(aVoiceHandle);
    if (aTime <= 0 || aTo == from)
    {
        setRelativePlaySpeed(aVoiceHandle, aTo);
        return;
    }
    FOR_ALL_VOICES_PRE
        mVoice[ch]
            ->
            mRelativePlaySpeedFader.set(from, aTo, aTime, mVoice[ch]->mStreamTime);
    FOR_ALL_VOICES_POST
}

void AudioDevice::fadeGlobalVolume(float aTo, time_t aTime)
{
    const float from = getGlobalVolume();
    if (aTime <= 0 || aTo == from)
    {
        setGlobalVolume(aTo);
        return;
    }
    mGlobalVolumeFader.set(from, aTo, aTime, mStreamTime);
}


void AudioDevice::oscillateVolume(handle aVoiceHandle, float aFrom, float aTo, time_t aTime)
{
    if (aTime <= 0 || aTo == aFrom)
    {
        setVolume(aVoiceHandle, aTo);
        return;
    }

    FOR_ALL_VOICES_PRE
        mVoice[ch]
            ->
            mVolumeFader.setLFO(aFrom, aTo, aTime, mVoice[ch]->mStreamTime);
    FOR_ALL_VOICES_POST
}

void AudioDevice::oscillatePan(handle aVoiceHandle, float aFrom, float aTo, time_t aTime)
{
    if (aTime <= 0 || aTo == aFrom)
    {
        setPan(aVoiceHandle, aTo);
        return;
    }

    FOR_ALL_VOICES_PRE
        mVoice[ch]
            ->
            mPanFader.setLFO(aFrom, aTo, aTime, mVoice[ch]->mStreamTime);
    FOR_ALL_VOICES_POST
}

void AudioDevice::oscillateRelativePlaySpeed(handle aVoiceHandle,
                                             float  aFrom,
                                             float  aTo,
                                             time_t aTime)
{
    if (aTime <= 0 || aTo == aFrom)
    {
        setRelativePlaySpeed(aVoiceHandle, aTo);
        return;
    }

    FOR_ALL_VOICES_PRE
        mVoice[ch]
            ->
            mRelativePlaySpeedFader.setLFO(aFrom, aTo, aTime, mVoice[ch]->mStreamTime);
    FOR_ALL_VOICES_POST
}

void AudioDevice::oscillateGlobalVolume(float aFrom, float aTo, time_t aTime)
{
    if (aTime <= 0 || aTo == aFrom)
    {
        setGlobalVolume(aTo);
        return;
    }

    mGlobalVolumeFader.setLFO(aFrom, aTo, aTime, mStreamTime);
}

AlignedFloatBuffer::AlignedFloatBuffer(size_t floats)
    : mFloats(floats)
{
#ifndef SOLOUD_SSE_INTRINSICS
    mBasePtr = std::make_unique<unsigned char[]>(mFloats * sizeof(float));
    mData    = reinterpret_cast<float*>(mBasePtr.get());
#else
    mBasePtr = std::make_unique<unsigned char[]>(aFloats * sizeof(float) + 16);
    mData    = (float*)(((size_t)mBasePtr.get() + 15) & ~15);
#endif
}

void AlignedFloatBuffer::clear()
{
    std::fill_n(mData, mFloats, 0.0f);
}

TinyAlignedFloatBuffer::TinyAlignedFloatBuffer()
{
    unsigned char* basePtr = &mActualData[0];
    mData                  = reinterpret_cast<float*>(size_t(basePtr) + 15 & ~15);
}

void AudioDevice::pause()
{
    if (mBackendPauseFunc != nullptr)
    {
        mBackendPauseFunc(this);
    }
}

void AudioDevice::resume()
{
    if (mBackendResumeFunc != nullptr)
    {
        mBackendResumeFunc(this);
    }
}


void AudioDevice::postinit_internal(size_t      sample_rate,
                                    size_t      buffer_size,
                                    EngineFlags flags,
                                    size_t      channels)
{
    mGlobalVolume = 1;
    mChannels     = channels;
    mSamplerate   = sample_rate;
    mBufferSize   = buffer_size;
    mScratchSize  = (buffer_size + 15) & (~0xf); // round to the next div by 16

    mScratchSize = std::max(mScratchSize, sample_granularity * 2);
    mScratchSize = std::max(mScratchSize, size_t(4096));

    mScratch       = AlignedFloatBuffer{mScratchSize * max_channels};
    mOutputScratch = AlignedFloatBuffer{mScratchSize * max_channels};

    mResampleData.resize(mMaxActiveVoices * 2);
    mResampleDataOwner.resize(mMaxActiveVoices);

    mResampleDataBuffer =
        AlignedFloatBuffer{mMaxActiveVoices * 2 * sample_granularity * max_channels};

    for (size_t i = 0; i < mMaxActiveVoices * 2; ++i)
    {
        mResampleData[i] = mResampleDataBuffer.mData + (sample_granularity * max_channels * i);
    }

    mFlags          = flags;
    mPostClipScaler = 0.95f;

    switch (mChannels)
    {
        case 1: {
            m3dSpeakerPosition[0] = {0, 0, 1};
            break;
        }
        case 2: {
            m3dSpeakerPosition[0] = {2, 0, 1};
            m3dSpeakerPosition[1] = {-2, 0, 1};
            break;
        }
        case 4: {
            m3dSpeakerPosition[0] = {2, 0, 1};
            m3dSpeakerPosition[1] = {-2, 0, 1};

            // I suppose technically the second pair should be straight left & right,
            // but I prefer moving them a bit back to mirror the front speakers.
            m3dSpeakerPosition[2] = {2, 0, -1};
            m3dSpeakerPosition[3] = {-2, 0, -1};

            break;
        }
        case 6: {
            m3dSpeakerPosition[0] = {2, 0, 1};
            m3dSpeakerPosition[1] = {-2, 0, 1};

            // center and subwoofer.
            m3dSpeakerPosition[2] = {0, 0, 1};

            // Sub should be "mix of everything". We'll handle it as a special case and make it a
            // null vector.
            m3dSpeakerPosition[3] = {0, 0, 0};

            // I suppose technically the second pair should be straight left & right,
            // but I prefer moving them a bit back to mirror the front speakers.
            m3dSpeakerPosition[4] = {2, 0, -1};
            m3dSpeakerPosition[5] = {-2, 0, -1};

            break;
        }
        case 8: {
            m3dSpeakerPosition[0] = {2, 0, 1};
            m3dSpeakerPosition[1] = {-2, 0, 1};

            // center and subwoofer.
            m3dSpeakerPosition[2] = {0, 0, 1};

            // Sub should be "mix of everything". We'll handle it as a special case and make it a
            // null vector.
            m3dSpeakerPosition[3] = {0, 0, 0};

            // side
            m3dSpeakerPosition[4] = {2, 0, 0};
            m3dSpeakerPosition[5] = {-2, 0, 0};

            // back
            m3dSpeakerPosition[6] = {2, 0, -1};
            m3dSpeakerPosition[7] = {-2, 0, -1};

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
        mWaveData[i] = mVisualizationWaveData[i];
    }

    unlockAudioMutex_internal();
    return mWaveData.data();
}

auto AudioDevice::getApproximateVolume(size_t aChannel) -> float
{
    if (aChannel > mChannels)
    {
        return 0;
    }

    float vol = 0;

    lockAudioMutex_internal();
    vol = mVisualizationChannelVolume[aChannel];
    unlockAudioMutex_internal();

    return vol;
}


auto AudioDevice::calcFFT() -> float*
{
    lockAudioMutex_internal();
    auto temp = std::array<float, 1024>{};

    for (size_t i = 0; i < 256; ++i)
    {
        temp[i * 2]     = mVisualizationWaveData[i];
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

        mFFTData[i] = std::sqrt(real * real + imag * imag);
    }

    return mFFTData.data();
}

#if defined(SOLOUD_SSE_INTRINSICS)
void AudioDevice::clip_internal(const AlignedFloatBuffer& aBuffer,
                           const AlignedFloatBuffer& aDestBuffer,
                           size_t                    aSamples,
                           float                     aVolume0,
                           float                     aVolume1)
{
    float  vd = (aVolume1 - aVolume0) / aSamples;
    float  v  = aVolume0;
    size_t i, c, d;
    size_t samplequads = (aSamples + 3) / 4; // rounded up

    // Clip
    if (mFlags.ClipRoundoff)
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
        __m128                 postscale   = _mm_load_ps1(&mPostClipScaler);
        TinyAlignedFloatBuffer volumes;
        volumes.mData[0] = v;
        volumes.mData[1] = v + vd;
        volumes.mData[2] = v + vd + vd;
        volumes.mData[3] = v + vd + vd + vd;
        vd *= 4;
        __m128 vdelta = _mm_load_ps1(&vd);
        c             = 0;
        d             = 0;
        for (size_t j = 0; j < mChannels; ++j)
        {
            __m128 vol = _mm_load_ps(volumes.mData);

            for (i = 0; i < samplequads; ++i)
            {
                // float f1 = origdata[c] * v;	++c; v += vd;
                __m128 f = _mm_load_ps(&aBuffer.mData[c]);
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
        __m128                 postscale = _mm_load_ps1(&mPostClipScaler);
        TinyAlignedFloatBuffer volumes;
        volumes.mData[0] = v;
        volumes.mData[1] = v + vd;
        volumes.mData[2] = v + vd + vd;
        volumes.mData[3] = v + vd + vd + vd;
        vd *= 4;
        __m128 vdelta = _mm_load_ps1(&vd);
        c             = 0;
        d             = 0;
        for (size_t j = 0; j < mChannels; ++j)
        {
            __m128 vol = _mm_load_ps(volumes.mData);
            for (i = 0; i < samplequads; ++i)
            {
                // float f1 = aBuffer.mData[c] * v; ++c; v += vd;
                __m128 f = _mm_load_ps(&aBuffer.mData[c]);
                c += 4;
                f   = _mm_mul_ps(f, vol);
                vol = _mm_add_ps(vol, vdelta);

                // f1 = (f1 <= -1) ? -1 : (f1 >= 1) ? 1 : f1;
                f = _mm_max_ps(f, negbound);
                f = _mm_min_ps(f, posbound);

                // aDestBuffer.mData[d] = f1 * mPostClipScaler; d++;
                f = _mm_mul_ps(f, postscale);
                _mm_store_ps(&aDestBuffer.mData[d], f);
                d += 4;
            }
        }
    }
}
#else // fallback code
void AudioDevice::clip_internal(const AlignedFloatBuffer& aBuffer,
                                const AlignedFloatBuffer& aDestBuffer,
                                size_t                    aSamples,
                                float                     aVolume0,
                                float                     aVolume1)
{
    const float vd          = (aVolume1 - aVolume0) / aSamples;
    const auto  samplequads = (aSamples + 3) / 4; // rounded up

    // Clip
    if (mFlags.ClipRoundoff)
    {
        auto c = size_t(0);
        auto d = size_t(0);

        for (size_t j = 0; j < mChannels; ++j)
        {
            auto v = aVolume0;
            for (size_t i = 0; i < samplequads; ++i)
            {
                float f1 = aBuffer.mData[c] * v;
                ++c;
                v += vd;

                float f2 = aBuffer.mData[c] * v;
                ++c;
                v += vd;

                float f3 = aBuffer.mData[c] * v;
                ++c;
                v += vd;

                float f4 = aBuffer.mData[c] * v;
                ++c;
                v += vd;

                f1 = f1 <= -1.65f
                         ? -0.9862875f
                         : f1 >= 1.65f
                         ? 0.9862875f
                         : 0.87f * f1 - 0.1f * f1 * f1 * f1;
                f2 = f2 <= -1.65f
                         ? -0.9862875f
                         : f2 >= 1.65f
                         ? 0.9862875f
                         : 0.87f * f2 - 0.1f * f2 * f2 * f2;
                f3 = f3 <= -1.65f
                         ? -0.9862875f
                         : f3 >= 1.65f
                         ? 0.9862875f
                         : 0.87f * f3 - 0.1f * f3 * f3 * f3;
                f4 = f4 <= -1.65f
                         ? -0.9862875f
                         : f4 >= 1.65f
                         ? 0.9862875f
                         : 0.87f * f4 - 0.1f * f4 * f4 * f4;

                aDestBuffer.mData[d] = f1 * mPostClipScaler;
                d++;

                aDestBuffer.mData[d] = f2 * mPostClipScaler;
                d++;

                aDestBuffer.mData[d] = f3 * mPostClipScaler;
                d++;

                aDestBuffer.mData[d] = f4 * mPostClipScaler;
                d++;
            }
        }
    }
    else
    {
        auto c = size_t(0);
        auto d = size_t(0);

        for (size_t j = 0; j < mChannels; ++j)
        {
            auto v = aVolume0;

            for (size_t i = 0; i < samplequads; ++i)
            {
                float f1 = aBuffer.mData[c] * v;
                ++c;
                v += vd;

                float f2 = aBuffer.mData[c] * v;
                ++c;
                v += vd;

                float f3 = aBuffer.mData[c] * v;
                ++c;
                v += vd;

                float f4 = aBuffer.mData[c] * v;
                ++c;
                v += vd;

                f1 = f1 <= -1 ? -1 : f1 >= 1 ? 1 : f1;
                f2 = f2 <= -1 ? -1 : f2 >= 1 ? 1 : f2;
                f3 = f3 <= -1 ? -1 : f3 >= 1 ? 1 : f3;
                f4 = f4 <= -1 ? -1 : f4 >= 1 ? 1 : f4;

                aDestBuffer.mData[d] = f1 * mPostClipScaler;
                d++;

                aDestBuffer.mData[d] = f2 * mPostClipScaler;
                d++;

                aDestBuffer.mData[d] = f3 * mPostClipScaler;
                d++;

                aDestBuffer.mData[d] = f4 * mPostClipScaler;
                d++;
            }
        }
    }
}
#endif

static constexpr auto FIXPOINT_FRAC_BITS = 20;
static constexpr auto FIXPOINT_FRAC_MUL  = 1 << FIXPOINT_FRAC_BITS;
static constexpr auto FIXPOINT_FRAC_MASK = (1 << FIXPOINT_FRAC_BITS) - 1;

static float catmullrom(float t, float p0, float p1, float p2, float p3)
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

        aDst[i] = catmullrom(f / float(FIXPOINT_FRAC_MUL), s3, s2, s1, s0);
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
        pan[k]  = aVoice->mCurrentChannelVolume[k];
        pand[k] = aVoice->mChannelVolume[k] * aVoice->mOverallVolume;
        pani[k] =
            (pand[k] - pan[k]) /
            aSamplesToRead; // TODO: this is a bit inconsistent.. but it's a hack to begin with
    }

    switch (aChannels)
    {
        case 1: // Target is mono. Sum everything. (1->1, 2->1, 4->1, 6->1, 8->1)
            for (size_t j = 0, ofs = 0; j < aVoice->mChannels; ++j, ofs += aBufferSize)
            {
                pan[0] = aVoice->mCurrentChannelVolume[0];
                for (size_t k = 0; k < aSamplesToRead; k++)
                {
                    pan[0] += pani[0];
                    aBuffer[k] += aScratch[ofs + k] * pan[0];
                }
            }
            break;
        case 2: switch (aVoice->mChannels)
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
        case 4: switch (aVoice->mChannels)
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
        case 6: switch (aVoice->mChannels)
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
        case 8: switch (aVoice->mChannels)
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
        aVoice->mCurrentChannelVolume[k] = pand[k];
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
    for (size_t i = 0; i < mActiveVoiceCount; ++i)
    {
        auto& voice = mVoice[mActiveVoice[i]];

        if (voice != nullptr && voice->mBusHandle == aBus && !voice->mFlags.Paused &&
            !voice->mFlags.Inaudible)
        {
            float step = voice->mSamplerate / aSamplerate;

            // avoid step overflow
            if (step > (1 << (32 - FIXPOINT_FRAC_BITS)))
            {
                step = 0;
            }

            size_t step_fixed = (int)floor(step * FIXPOINT_FRAC_MUL);
            size_t outofs     = 0;

            if (voice->mDelaySamples)
            {
                if (voice->mDelaySamples > aSamplesToRead)
                {
                    outofs = aSamplesToRead;
                    voice->mDelaySamples -= aSamplesToRead;
                }
                else
                {
                    outofs               = voice->mDelaySamples;
                    voice->mDelaySamples = 0;
                }

                // Clear scratch where we're skipping
                for (size_t k = 0; k < voice->mChannels; k++)
                {
                    memset(aScratch + k * aBufferSize, 0, sizeof(float) * outofs);
                }
            }

            while (step_fixed != 0 && outofs < aSamplesToRead)
            {
                if (voice->mLeftoverSamples == 0)
                {
                    // Swap resample buffers (ping-pong)
                    float* t                = voice->mResampleData[0];
                    voice->mResampleData[0] = voice->mResampleData[1];
                    voice->mResampleData[1] = t;

                    // Get a block of source data

                    size_t readcount = 0;
                    if (!voice->hasEnded() || voice->mFlags.Looping)
                    {
                        readcount = voice->getAudio(voice->mResampleData[0],
                                                    sample_granularity,
                                                    sample_granularity);
                        if (readcount < sample_granularity)
                        {
                            if (voice->mFlags.Looping)
                            {
                                while (readcount < sample_granularity &&
                                       voice->seek(voice->mLoopPoint, mScratch.mData, mScratchSize))
                                {
                                    voice->mLoopCount++;

                                    const int inc =
                                        voice->getAudio(voice->mResampleData[0] + readcount,
                                                        sample_granularity - readcount,
                                                        sample_granularity);

                                    readcount += inc;
                                    if (inc == 0)
                                        break;
                                }
                            }
                        }
                    }

                    // Clear remaining of the resample data if the full scratch wasn't used
                    if (readcount < sample_granularity)
                    {
                        for (size_t k = 0; k < voice->mChannels; k++)
                        {
                            memset(voice->mResampleData[0] + readcount + sample_granularity * k,
                                   0,
                                   sizeof(float) * (sample_granularity - readcount));
                        }
                    }

                    // If we go past zero, crop to zero (a bit of a kludge)
                    if (voice->mSrcOffset < sample_granularity * FIXPOINT_FRAC_MUL)
                    {
                        voice->mSrcOffset = 0;
                    }
                    else
                    {
                        // We have new block of data, move pointer backwards
                        voice->mSrcOffset -= sample_granularity *
                            FIXPOINT_FRAC_MUL;
                    }


                    // Run the per-stream filters to get our source data

                    for (size_t j = 0; j < filters_per_stream;
                         ++j
                    )
                    {
                        if (voice->mFilter[j])
                        {
                            voice->mFilter[j]->filter(voice->mResampleData[0],
                                                      sample_granularity,
                                                      sample_granularity,
                                                      voice->mChannels,
                                                      voice->mSamplerate,
                                                      mStreamTime);
                        }
                    }
                }
                else
                {
                    voice->mLeftoverSamples = 0;
                }

                // Figure out how many samples we can generate from this source data.
                // The value may be zero.

                size_t writesamples = 0;

                if (voice->mSrcOffset < sample_granularity * FIXPOINT_FRAC_MUL)
                {
                    writesamples =
                        ((sample_granularity * FIXPOINT_FRAC_MUL) - voice->
                         mSrcOffset) /
                        step_fixed +
                        1;

                    // avoid reading past the current buffer..
                    if (((writesamples * step_fixed + voice->mSrcOffset) >> FIXPOINT_FRAC_BITS) >=
                        sample_granularity)
                        writesamples--;
                }


                // If this is too much for our output buffer, don't write that many:
                if (writesamples + outofs > aSamplesToRead)
                {
                    voice->mLeftoverSamples = (writesamples + outofs) - aSamplesToRead;
                    writesamples            = aSamplesToRead - outofs;
                }

                // Call resampler to generate the samples, once per channel
                if (writesamples)
                {
                    for (size_t j = 0; j < voice->mChannels; ++j)
                    {
                        switch (aResampler)
                        {
                            case Resampler::Point: resample_point(
                                    voice->mResampleData[0] + sample_granularity * j,
                                    voice->mResampleData[1] + sample_granularity * j,
                                    aScratch + aBufferSize * j + outofs,
                                    voice->mSrcOffset,
                                    writesamples,
                                    /*voice->mSamplerate,
                                    aSamplerate,*/
                                    step_fixed);
                                break;
                            case Resampler::CatmullRom: resample_catmullrom(
                                    voice->mResampleData[0] + sample_granularity * j,
                                    voice->mResampleData[1] + sample_granularity * j,
                                    aScratch + aBufferSize * j + outofs,
                                    voice->mSrcOffset,
                                    writesamples,
                                    /*voice->mSamplerate,
                                    aSamplerate,*/
                                    step_fixed);
                                break;
                            default:
                                // case RESAMPLER_LINEAR:
                                resample_linear(voice->mResampleData[0] + sample_granularity * j,
                                                voice->mResampleData[1] + sample_granularity * j,
                                                aScratch + aBufferSize * j + outofs,
                                                voice->mSrcOffset,
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
                voice->mSrcOffset += writesamples * step_fixed;
            }

            // Handle panning and channel expansion (and/or shrinking)
            panAndExpand(voice, aBuffer, aSamplesToRead, aBufferSize, aScratch, aChannels);

            // clear voice if the sound is over
            // TODO: check this condition some day
            if (!voice->mFlags.Looping && !voice->mFlags.DisableAutostop && voice->hasEnded())
            {
                stopVoice_internal(mActiveVoice[i]);
            }
        }
        else if (voice && voice->mBusHandle == aBus && !voice->mFlags.Paused &&
                 voice->mFlags.Inaudible && voice->mFlags.InaudibleTick)
        {
            // Inaudible but needs ticking. Do minimal work (keep counters up to date and ask
            // audiosource for data)
            auto step       = voice->mSamplerate / aSamplerate;
            auto step_fixed = int(floor(step * FIXPOINT_FRAC_MUL));
            auto outofs     = size_t(0);

            if (voice->mDelaySamples)
            {
                if (voice->mDelaySamples > aSamplesToRead)
                {
                    outofs = aSamplesToRead;
                    voice->mDelaySamples -= aSamplesToRead;
                }
                else
                {
                    outofs               = voice->mDelaySamples;
                    voice->mDelaySamples = 0;
                }
            }

            while (step_fixed != 0 && outofs < aSamplesToRead)
            {
                if (voice->mLeftoverSamples == 0)
                {
                    // Swap resample buffers (ping-pong)
                    float* t                = voice->mResampleData[0];
                    voice->mResampleData[0] = voice->mResampleData[1];
                    voice->mResampleData[1] = t;

                    // Get a block of source data

                    if (!voice->hasEnded() || voice->mFlags.Looping)
                    {
                        auto readcount = voice->getAudio(voice->mResampleData[0],
                                                         sample_granularity,
                                                         sample_granularity);
                        if (readcount < sample_granularity)
                        {
                            if (voice->mFlags.Looping)
                            {
                                while (readcount < sample_granularity &&
                                       voice->seek(voice->mLoopPoint, mScratch.mData, mScratchSize))
                                {
                                    voice->mLoopCount++;
                                    readcount +=
                                        voice->getAudio(voice->mResampleData[0] + readcount,
                                                        sample_granularity - readcount,
                                                        sample_granularity);
                                }
                            }
                        }
                    }

                    // If we go past zero, crop to zero (a bit of a kludge)
                    if (voice->mSrcOffset < sample_granularity * FIXPOINT_FRAC_MUL)
                    {
                        voice->mSrcOffset = 0;
                    }
                    else
                    {
                        // We have new block of data, move pointer backwards
                        voice->mSrcOffset -= sample_granularity *
                            FIXPOINT_FRAC_MUL;
                    }

                    // Skip filters


                }
                else
                {
                    voice->mLeftoverSamples = 0;
                }

                // Figure out how many samples we can generate from this source data.
                // The value may be zero.

                auto writesamples = size_t(0);

                if (voice->mSrcOffset < sample_granularity * FIXPOINT_FRAC_MUL)
                {
                    writesamples =
                        ((sample_granularity * FIXPOINT_FRAC_MUL) - voice->
                         mSrcOffset) /
                        step_fixed +
                        1;

                    // avoid reading past the current buffer..
                    if (((writesamples * step_fixed + voice->mSrcOffset) >> FIXPOINT_FRAC_BITS) >=
                        sample_granularity)
                        writesamples--;
                }


                // If this is too much for our output buffer, don't write that many:
                if (writesamples + outofs > aSamplesToRead)
                {
                    voice->mLeftoverSamples = (writesamples + outofs) - aSamplesToRead;
                    writesamples            = aSamplesToRead - outofs;
                }

                // Skip resampler

                // Keep track of how many samples we've written so far
                outofs += writesamples;

                // Move source pointer onwards (writesamples may be zero)
                voice->mSrcOffset += writesamples * step_fixed;
            }

            // clear voice if the sound is over
            // TODO: check this condition some day
            if (!voice->mFlags.Looping && !voice->mFlags.DisableAutostop && voice->hasEnded())
            {
                stopVoice_internal(mActiveVoice[i]);
            }
        }
    }
}

void AudioDevice::mapResampleBuffers_internal()
{
    assert(mMaxActiveVoices < 256);
    auto live = std::array<char, 256>{};

    for (size_t i = 0; i < mMaxActiveVoices; ++i)
    {
        for (size_t j = 0; j < mMaxActiveVoices; ++j)
        {
            if (mResampleDataOwner[i] &&
                mResampleDataOwner[i].get() == mVoice[mActiveVoice[j]].get())
            {
                live[i] |= 1; // Live channel
                live[j] |= 2; // Live voice
            }
        }
    }

    for (size_t i = 0; i < mMaxActiveVoices; ++i)
    {
        if (!(live[i] & 1) && mResampleDataOwner[i]) // For all dead channels with owners..
        {
            mResampleDataOwner[i]->mResampleData[0] = nullptr;
            mResampleDataOwner[i]->mResampleData[1] = nullptr;
            mResampleDataOwner[i]                   = nullptr;
        }
    }

    auto latestfree = 0;

    for (size_t i = 0; i < mActiveVoiceCount; ++i)
    {
        if (!(live[i] & 2) && mVoice[mActiveVoice[i]]) // For all live voices with no channel..
        {
            int found = -1;

            for (size_t j = latestfree; found == -1 && j < mMaxActiveVoices; ++j)
            {
                if (mResampleDataOwner[j] == nullptr)
                {
                    found = j;
                }
            }

            assert(found != -1);
            mResampleDataOwner[found]                   = mVoice[mActiveVoice[i]];
            mResampleDataOwner[found]->mResampleData[0] = mResampleData[found * 2 + 0];
            mResampleDataOwner[found]->mResampleData[1] = mResampleData[found * 2 + 1];

            memset(mResampleDataOwner[found]->mResampleData[0],
                   0,
                   sizeof(float) * sample_granularity * max_channels);

            memset(mResampleDataOwner[found]->mResampleData[1],
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

    mActiveVoiceDirty = false;

    // Populate
    size_t candidates = 0;
    size_t mustlive   = 0;

    for (size_t i = 0; i < mHighestVoice; ++i)
    {
        const auto voice = mVoice[i];
        if (voice == nullptr)
        {
            continue;
        }

        // TODO: check this some day
        if ((!voice->mFlags.Inaudible && !voice->mFlags.Paused) || voice->mFlags.InaudibleTick)
        {
            mActiveVoice[candidates] = i;
            candidates++;
            if (mVoice[i]->mFlags.InaudibleTick)
            {
                mActiveVoice[candidates - 1] = mActiveVoice[mustlive];
                mActiveVoice[mustlive]       = i;
                mustlive++;
            }
        }
    }

    // Check for early out
    if (candidates <= mMaxActiveVoices)
    {
        // everything is audible, early out
        mActiveVoiceCount = candidates;
        mapResampleBuffers_internal();
        return;
    }

    mActiveVoiceCount = mMaxActiveVoices;

    if (mustlive >= mMaxActiveVoices)
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
    size_t*   data = mActiveVoice.data() + mustlive;
    const int k    = mActiveVoiceCount;
    while (true)
    {
        for (; left + 1 < len; len++)
        {
            if (pos == 24)
            {
                len = stack[pos = 0];
            }

            const int   pivot    = data[left];
            const float pivotvol = mVoice[pivot]->mOverallVolume;
            stack[pos++]         = len;

            for (int right = left - 1;;)
            {
                do
                {
                    ++right;
                } while (mVoice[data[right]]->mOverallVolume > pivotvol);

                do
                {
                    --len;
                } while (pivotvol > mVoice[data[len]]->mOverallVolume);

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
            if (!mFlags.NoFpuRegisterChange)
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
            if (!mFlags.NoFpuRegisterChange)
            {
                _mm_setcsr(_mm_getcsr() | 0x8040);
            }
        }
    }
#endif

    const auto buffertime   = aSamples / float(mSamplerate);
    auto       globalVolume = std::array<float, 2>{};

    mStreamTime += buffertime;
    mLastClockedTime = 0;

    globalVolume[0] = mGlobalVolume;

    if (mGlobalVolumeFader.mActive)
    {
        mGlobalVolume = mGlobalVolumeFader.get(mStreamTime);
    }

    globalVolume[1] = mGlobalVolume;

    lockAudioMutex_internal();

    // Process faders. May change scratch size.
    for (size_t i = 0; i < mHighestVoice; ++i)
    {
        if (mVoice[i] != nullptr && !mVoice[i]->mFlags.Paused)
        {
            auto volume = std::array<float, 2>{};

            mVoice[i]->mActiveFader = 0;

            if (mGlobalVolumeFader.mActive > 0)
            {
                mVoice[i]->mActiveFader = 1;
            }

            mVoice[i]->mStreamTime += buffertime;
            mVoice[i]->mStreamPosition +=
                double(buffertime) * double(mVoice[i]->mOverallRelativePlaySpeed);

            // TODO: this is actually unstable, because mStreamTime depends on the relative play
            // speed.
            if (mVoice[i]->mRelativePlaySpeedFader.mActive > 0)
            {
                float speed = mVoice[i]->mRelativePlaySpeedFader.get(mVoice[i]->mStreamTime);
                setVoiceRelativePlaySpeed_internal(i, speed);
            }

            volume[0] = mVoice[i]->mOverallVolume;

            if (mVoice[i]->mVolumeFader.mActive > 0)
            {
                mVoice[i]->mSetVolume   = mVoice[i]->mVolumeFader.get(mVoice[i]->mStreamTime);
                mVoice[i]->mActiveFader = 1;
                updateVoiceVolume_internal(i);
                mActiveVoiceDirty = true;
            }

            volume[1] = mVoice[i]->mOverallVolume;

            if (mVoice[i]->mPanFader.mActive > 0)
            {
                float pan = mVoice[i]->mPanFader.get(mVoice[i]->mStreamTime);
                setVoicePan_internal(i, pan);
                mVoice[i]->mActiveFader = 1;
            }

            if (mVoice[i]->mPauseScheduler.mActive)
            {
                mVoice[i]->mPauseScheduler.get(mVoice[i]->mStreamTime);
                if (mVoice[i]->mPauseScheduler.mActive == -1)
                {
                    mVoice[i]->mPauseScheduler.mActive = 0;
                    setVoicePause_internal(i, 1);
                }
            }

            if (mVoice[i]->mStopScheduler.mActive)
            {
                mVoice[i]->mStopScheduler.get(mVoice[i]->mStreamTime);
                if (mVoice[i]->mStopScheduler.mActive == -1)
                {
                    mVoice[i]->mStopScheduler.mActive = 0;
                    stopVoice_internal(i);
                }
            }
        }
    }

    if (mActiveVoiceDirty)
    {
        calcActiveVoices_internal();
    }

    mixBus_internal(mOutputScratch.mData,
                    aSamples,
                    aStride,
                    mScratch.mData,
                    0,
                    float(mSamplerate),
                    mChannels,
                    mResampler);

    for (size_t i = 0; i < filters_per_stream; ++i)
    {
        if (mFilterInstance[i])
        {
            mFilterInstance[i]->filter(mOutputScratch.mData,
                                       aSamples,
                                       aStride,
                                       mChannels,
                                       float(mSamplerate),
                                       mStreamTime);
        }
    }

    unlockAudioMutex_internal();

    // Note: clipping channels*aStride, not channels*aSamples, so we're possibly clipping some
    // unused data. The buffers should be large enough for it, we just may do a few bytes of
    // unneccessary work.
    clip_internal(mOutputScratch, mScratch, aStride, globalVolume[0], globalVolume[1]);

    if (mFlags.EnableVisualization)
    {
        for (size_t i = 0; i < max_channels; ++i)
        {
            mVisualizationChannelVolume[i] = 0;
        }

        if (aSamples > 255)
        {
            for (size_t i = 0; i < 256; ++i)
            {
                mVisualizationWaveData[i] = 0;
                for (size_t j = 0; j < mChannels; ++j)
                {
                    const auto sample = mScratch.mData[i + j * aStride];
                    const auto absvol = fabs(sample);

                    if (mVisualizationChannelVolume[j] < absvol)
                    {
                        mVisualizationChannelVolume[j] = absvol;
                    }

                    mVisualizationWaveData[i] += sample;
                }
            }
        }
        else
        {
            // Very unlikely failsafe branch
            for (size_t i = 0; i < 256; ++i)
            {
                mVisualizationWaveData[i] = 0;

                for (size_t j = 0; j < mChannels; ++j)
                {
                    const auto sample = mScratch.mData[(i % aSamples) + j * aStride];
                    const auto absvol = fabs(sample);

                    if (mVisualizationChannelVolume[j] < absvol)
                    {
                        mVisualizationChannelVolume[j] = absvol;
                    }

                    mVisualizationWaveData[i] += sample;
                }
            }
        }
    }
}

void interlace_samples_float(const float* aSourceBuffer,
                             float*       aDestBuffer,
                             size_t       aSamples,
                             size_t       aChannels,
                             size_t       aStride)
{
    // 111222 -> 121212
    for (size_t j = 0; j < aChannels; ++j)
    {
        auto c = j * aStride;
        for (auto i = j; i < aSamples * aChannels; i += aChannels)
        {
            aDestBuffer[i] = aSourceBuffer[c];
            ++c;
        }
    }
}

void interlace_samples_s16(const float* aSourceBuffer,
                           short*       aDestBuffer,
                           size_t       aSamples,
                           size_t       aChannels,
                           size_t       aStride)
{
    // 111222 -> 121212
    for (size_t j = 0; j < aChannels; ++j)
    {
        size_t c = j * aStride;

        for (size_t i = j; i < aSamples * aChannels; i += aChannels)
        {
            aDestBuffer[i] = short(aSourceBuffer[c] * 0x7fff);
            ++c;
        }
    }
}

void AudioDevice::mix(float* aBuffer, size_t aSamples)
{
    size_t stride = (aSamples + 15) & ~0xf;
    mix_internal(aSamples, stride);
    interlace_samples_float(mScratch.mData, aBuffer, aSamples, mChannels, stride);
}

void AudioDevice::mixSigned16(short* aBuffer, size_t aSamples)
{
    size_t stride = (aSamples + 15) & ~0xf;
    mix_internal(aSamples, stride);
    interlace_samples_s16(mScratch.mData, aBuffer, aSamples, mChannels, stride);
}

void AudioDevice::lockAudioMutex_internal()
{
    if (mAudioThreadMutex)
    {
        Thread::lockMutex(mAudioThreadMutex);
    }
    assert(!mInsideAudioThreadMutex);
    mInsideAudioThreadMutex = true;
}

void AudioDevice::unlockAudioMutex_internal()
{
    assert(mInsideAudioThreadMutex);
    mInsideAudioThreadMutex = false;
    if (mAudioThreadMutex)
    {
        Thread::unlockMutex(mAudioThreadMutex);
    }
}

float AudioDevice::getPostClipScaler() const
{
    return mPostClipScaler;
}

Resampler AudioDevice::getMainResampler() const
{
    return mResampler;
}

float AudioDevice::getGlobalVolume() const
{
    return mGlobalVolume;
}

handle AudioDevice::getHandleFromVoice_internal(size_t aVoice) const
{
    if (mVoice[aVoice] == nullptr)
    {
        return 0;
    }

    return (aVoice + 1) | (mVoice[aVoice]->mPlayIndex << 12);
}

int AudioDevice::getVoiceFromHandle_internal(handle aVoiceHandle) const
{
    // If this is a voice group handle, pick the first handle from the group
    if (const auto* h = voiceGroupHandleToArray_internal(aVoiceHandle); h != nullptr)
    {
        aVoiceHandle = *h;
    }

    if (aVoiceHandle == 0)
    {
        return -1;
    }

    const int    ch  = (aVoiceHandle & 0xfff) - 1;
    const size_t idx = aVoiceHandle >> 12;

    if (mVoice[ch] != nullptr && (mVoice[ch]->mPlayIndex & 0xfffff) == idx)
    {
        return ch;
    }

    return -1;
}

size_t AudioDevice::getMaxActiveVoiceCount() const
{
    return mMaxActiveVoices;
}

size_t AudioDevice::getActiveVoiceCount()
{
    lockAudioMutex_internal();
    if (mActiveVoiceDirty)
    {
        calcActiveVoices_internal();
    }
    const size_t c = mActiveVoiceCount;
    unlockAudioMutex_internal();

    return c;
}

size_t AudioDevice::getVoiceCount()
{
    lockAudioMutex_internal();
    int c = 0;
    for (size_t i = 0; i < mHighestVoice; ++i)
    {
        if (mVoice[i])
        {
            ++c;
        }
    }
    unlockAudioMutex_internal();
    return c;
}

bool AudioDevice::isValidVoiceHandle(handle aVoiceHandle)
{
    // voice groups are not valid voice handles
    if ((aVoiceHandle & 0xfffff000) == 0xfffff000)
    {
        return false;
    }

    lockAudioMutex_internal();
    if (getVoiceFromHandle_internal(aVoiceHandle) != -1)
    {
        unlockAudioMutex_internal();
        return true;
    }
    unlockAudioMutex_internal();
    return false;
}


time_t AudioDevice::getLoopPoint(handle aVoiceHandle)
{
    lockAudioMutex_internal();
    const int ch = getVoiceFromHandle_internal(aVoiceHandle);
    if (ch == -1)
    {
        unlockAudioMutex_internal();
        return 0;
    }
    const time_t v = mVoice[ch]->mLoopPoint;
    unlockAudioMutex_internal();
    return v;
}

bool AudioDevice::getLooping(handle aVoiceHandle)
{
    lockAudioMutex_internal();
    const int ch = getVoiceFromHandle_internal(aVoiceHandle);
    if (ch == -1)
    {
        unlockAudioMutex_internal();
        return 0;
    }
    const bool v = mVoice[ch]->mFlags.Looping;
    unlockAudioMutex_internal();
    return v;
}

bool AudioDevice::getAutoStop(handle aVoiceHandle)
{
    lockAudioMutex_internal();
    const int ch = getVoiceFromHandle_internal(aVoiceHandle);
    if (ch == -1)
    {
        unlockAudioMutex_internal();
        return false;
    }
    const auto v = mVoice[ch]->mFlags.DisableAutostop;
    unlockAudioMutex_internal();
    return !v;
}

float AudioDevice::getInfo(handle aVoiceHandle, size_t mInfoKey)
{
    lockAudioMutex_internal();
    const int ch = getVoiceFromHandle_internal(aVoiceHandle);
    if (ch == -1)
    {
        unlockAudioMutex_internal();
        return 0;
    }
    const float v = mVoice[ch]->getInfo(mInfoKey);
    unlockAudioMutex_internal();
    return v;
}

float AudioDevice::getVolume(handle aVoiceHandle)
{
    lockAudioMutex_internal();
    const int ch = getVoiceFromHandle_internal(aVoiceHandle);
    if (ch == -1)
    {
        unlockAudioMutex_internal();
        return 0;
    }
    const float v = mVoice[ch]->mSetVolume;
    unlockAudioMutex_internal();
    return v;
}

float AudioDevice::getOverallVolume(handle aVoiceHandle)
{
    lockAudioMutex_internal();
    const int ch = getVoiceFromHandle_internal(aVoiceHandle);
    if (ch == -1)
    {
        unlockAudioMutex_internal();
        return 0;
    }
    const float v = mVoice[ch]->mOverallVolume;
    unlockAudioMutex_internal();
    return v;
}

float AudioDevice::getPan(handle aVoiceHandle)
{
    lockAudioMutex_internal();
    const int ch = getVoiceFromHandle_internal(aVoiceHandle);
    if (ch == -1)
    {
        unlockAudioMutex_internal();
        return 0;
    }
    const float v = mVoice[ch]->mPan;
    unlockAudioMutex_internal();
    return v;
}

time_t AudioDevice::getStreamTime(handle aVoiceHandle)
{
    lockAudioMutex_internal();
    const int ch = getVoiceFromHandle_internal(aVoiceHandle);
    if (ch == -1)
    {
        unlockAudioMutex_internal();
        return 0;
    }
    const double v = mVoice[ch]->mStreamTime;
    unlockAudioMutex_internal();
    return v;
}

time_t AudioDevice::getStreamPosition(handle aVoiceHandle)
{
    lockAudioMutex_internal();
    const int ch = getVoiceFromHandle_internal(aVoiceHandle);
    if (ch == -1)
    {
        unlockAudioMutex_internal();
        return 0;
    }
    const double v = mVoice[ch]->mStreamPosition;
    unlockAudioMutex_internal();
    return v;
}

float AudioDevice::getRelativePlaySpeed(handle aVoiceHandle)
{
    lockAudioMutex_internal();
    const int ch = getVoiceFromHandle_internal(aVoiceHandle);
    if (ch == -1)
    {
        unlockAudioMutex_internal();
        return 1;
    }
    const float v = mVoice[ch]->mSetRelativePlaySpeed;
    unlockAudioMutex_internal();
    return v;
}

float AudioDevice::getSamplerate(handle aVoiceHandle)
{
    lockAudioMutex_internal();
    const int ch = getVoiceFromHandle_internal(aVoiceHandle);
    if (ch == -1)
    {
        unlockAudioMutex_internal();
        return 0;
    }
    const float v = mVoice[ch]->mBaseSamplerate;
    unlockAudioMutex_internal();
    return v;
}

bool AudioDevice::getPause(handle aVoiceHandle)
{
    lockAudioMutex_internal();
    const int ch = getVoiceFromHandle_internal(aVoiceHandle);
    if (ch == -1)
    {
        unlockAudioMutex_internal();
        return false;
    }
    const auto v = mVoice[ch]->mFlags.Paused;
    unlockAudioMutex_internal();
    return v;
}

bool AudioDevice::getProtectVoice(handle aVoiceHandle)
{
    lockAudioMutex_internal();
    const int ch = getVoiceFromHandle_internal(aVoiceHandle);
    if (ch == -1)
    {
        unlockAudioMutex_internal();
        return false;
    }
    const auto v = mVoice[ch]->mFlags.Protected;
    unlockAudioMutex_internal();
    return v;
}

int AudioDevice::findFreeVoice_internal()
{
    size_t lowest_play_index_value = 0xffffffff;
    int    lowest_play_index       = -1;

    // (slowly) drag the highest active voice index down
    if (mHighestVoice > 0 && mVoice[mHighestVoice - 1] == nullptr)
    {
        mHighestVoice--;
    }

    for (size_t i = 0; i < voice_count; ++i)
    {
        if (mVoice[i] == nullptr)
        {
            if (i + 1 > mHighestVoice)
            {
                mHighestVoice = i + 1;
            }
            return i;
        }

        if (!mVoice[i]->mFlags.Protected && mVoice[i]->mPlayIndex < lowest_play_index_value)
        {
            lowest_play_index_value = mVoice[i]->mPlayIndex;
            lowest_play_index       = i;
        }
    }
    stopVoice_internal(lowest_play_index);
    return lowest_play_index;
}

size_t AudioDevice::getLoopCount(handle aVoiceHandle)
{
    lockAudioMutex_internal();
    const int ch = getVoiceFromHandle_internal(aVoiceHandle);
    if (ch == -1)
    {
        unlockAudioMutex_internal();
        return 0;
    }
    const int v = mVoice[ch]->mLoopCount;
    unlockAudioMutex_internal();
    return v;
}

// Returns current backend channel count (1 mono, 2 stereo, etc)
size_t AudioDevice::backend_channels() const
{
    return mChannels;
}

// Returns current backend sample rate
size_t AudioDevice::backend_sample_rate() const
{
    return mSamplerate;
}

// Returns current backend buffer size
size_t AudioDevice::backend_buffer_size() const
{
    return mBufferSize;
}

// Get speaker position in 3d space
Vector3 AudioDevice::speaker_position(size_t channel) const
{
    return m3dSpeakerPosition.at(channel);
}

void AudioDevice::setPostClipScaler(float aScaler)
{
    mPostClipScaler = aScaler;
}

void AudioDevice::setMainResampler(Resampler aResampler)
{
    mResampler = aResampler;
}

void AudioDevice::setGlobalVolume(float aVolume)
{
    mGlobalVolumeFader.mActive = 0;
    mGlobalVolume              = aVolume;
}

void AudioDevice::setRelativePlaySpeed(handle aVoiceHandle, float aSpeed)
{
    FOR_ALL_VOICES_PRE
        mVoice[ch]->mRelativePlaySpeedFader.mActive = 0;
        setVoiceRelativePlaySpeed_internal(ch, aSpeed);
    FOR_ALL_VOICES_POST
}

void AudioDevice::setSamplerate(handle aVoiceHandle, float aSamplerate)
{
    FOR_ALL_VOICES_PRE
        mVoice[ch]->mBaseSamplerate = aSamplerate;
        updateVoiceRelativePlaySpeed_internal(ch);
    FOR_ALL_VOICES_POST
}

void AudioDevice::setPause(handle aVoiceHandle, bool aPause)
{
    FOR_ALL_VOICES_PRE
        setVoicePause_internal(ch, aPause);
    FOR_ALL_VOICES_POST
}

void AudioDevice::setMaxActiveVoiceCount(size_t aVoiceCount)
{
    assert(aVoiceCount > 0);
    assert(aVoiceCount <= voice_count);

    lockAudioMutex_internal();
    mMaxActiveVoices = aVoiceCount;

    mResampleData.resize(aVoiceCount * 2);
    mResampleDataOwner.resize(aVoiceCount);

    mResampleDataBuffer = AlignedFloatBuffer{sample_granularity * max_channels * aVoiceCount * 2};

    for (size_t i        = 0; i < aVoiceCount * 2; ++i)
        mResampleData[i] = mResampleDataBuffer.mData + (sample_granularity * max_channels * i);

    for (size_t i             = 0; i < aVoiceCount; ++i)
        mResampleDataOwner[i] = nullptr;

    mActiveVoiceDirty = true;
    unlockAudioMutex_internal();
}

void AudioDevice::setPauseAll(bool aPause)
{
    lockAudioMutex_internal();
    for (size_t ch = 0; ch < mHighestVoice; ++ch)
    {
        setVoicePause_internal(ch, aPause);
    }
    unlockAudioMutex_internal();
}

void AudioDevice::setProtectVoice(handle aVoiceHandle, bool aProtect)
{
    FOR_ALL_VOICES_PRE
        mVoice[ch]->mFlags.Protected = aProtect;
    FOR_ALL_VOICES_POST
}

void AudioDevice::setPan(handle aVoiceHandle, float aPan)
{
    FOR_ALL_VOICES_PRE
        setVoicePan_internal(ch, aPan);
    FOR_ALL_VOICES_POST
}

void AudioDevice::setChannelVolume(handle aVoiceHandle, size_t aChannel, float aVolume)
{
    FOR_ALL_VOICES_PRE
        if (mVoice[ch]->mChannels > aChannel)
        {
            mVoice[ch]->mChannelVolume[aChannel] = aVolume;
        }
    FOR_ALL_VOICES_POST
}

void AudioDevice::setPanAbsolute(handle aVoiceHandle, float aLVolume, float aRVolume)
{
    FOR_ALL_VOICES_PRE
        mVoice[ch]->mPanFader.mActive = 0;
        mVoice[ch]->mChannelVolume[0] = aLVolume;
        mVoice[ch]->mChannelVolume[1] = aRVolume;
        if (mVoice[ch]->mChannels == 4)
        {
            mVoice[ch]->mChannelVolume[2] = aLVolume;
            mVoice[ch]->mChannelVolume[3] = aRVolume;
        }
        if (mVoice[ch]->mChannels == 6)
        {
            mVoice[ch]->mChannelVolume[2] = (aLVolume + aRVolume) * 0.5f;
            mVoice[ch]->mChannelVolume[3] = (aLVolume + aRVolume) * 0.5f;
            mVoice[ch]->mChannelVolume[4] = aLVolume;
            mVoice[ch]->mChannelVolume[5] = aRVolume;
        }
        if (mVoice[ch]->mChannels == 8)
        {
            mVoice[ch]->mChannelVolume[2] = (aLVolume + aRVolume) * 0.5f;
            mVoice[ch]->mChannelVolume[3] = (aLVolume + aRVolume) * 0.5f;
            mVoice[ch]->mChannelVolume[4] = aLVolume;
            mVoice[ch]->mChannelVolume[5] = aRVolume;
            mVoice[ch]->mChannelVolume[6] = aLVolume;
            mVoice[ch]->mChannelVolume[7] = aRVolume;
        }
    FOR_ALL_VOICES_POST
}

void AudioDevice::setInaudibleBehavior(handle aVoiceHandle, bool aMustTick, bool aKill)
{
    FOR_ALL_VOICES_PRE
        mVoice[ch]->mFlags.InaudibleKill = aKill;
        mVoice[ch]->mFlags.InaudibleTick = aMustTick;
    FOR_ALL_VOICES_POST
}

void AudioDevice::setLoopPoint(handle aVoiceHandle, time_t aLoopPoint)
{
    FOR_ALL_VOICES_PRE
        mVoice[ch]->mLoopPoint = aLoopPoint;
    FOR_ALL_VOICES_POST
}

void AudioDevice::setLooping(handle aVoiceHandle, bool aLooping)
{
    FOR_ALL_VOICES_PRE
        mVoice[ch]->mFlags.Looping = aLooping;
    FOR_ALL_VOICES_POST
}

void AudioDevice::setAutoStop(handle aVoiceHandle, bool aAutoStop)
{
    FOR_ALL_VOICES_PRE
        mVoice[ch]->mFlags.DisableAutostop = !aAutoStop;
    FOR_ALL_VOICES_POST
}

void AudioDevice::setVolume(handle aVoiceHandle, float aVolume)
{
    FOR_ALL_VOICES_PRE
        mVoice[ch]->mVolumeFader.mActive = 0;
        setVoiceVolume_internal(ch, aVolume);
    FOR_ALL_VOICES_POST
}

void AudioDevice::setDelaySamples(handle aVoiceHandle, size_t aSamples)
{
    FOR_ALL_VOICES_PRE
        mVoice[ch]->mDelaySamples = aSamples;
    FOR_ALL_VOICES_POST
}

void AudioDevice::setVisualizationEnable(bool aEnable)
{
    mFlags.EnableVisualization = aEnable;
}

void AudioDevice::speaker_position(size_t channel, Vector3 value)
{
    m3dSpeakerPosition.at(channel) = value;
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

float doppler(
    Vector3        aDeltaPos,
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

    for (size_t i  = 0; i < mChannels; ++i)
        speaker[i] = normalize(m3dSpeakerPosition[i]);

    const auto lpos = m3dPosition;
    const auto lvel = m3dVelocity;
    const auto at   = m3dAt;
    const auto up   = m3dUp;
    const auto m    = lookatRH(at, up);

    for (const size_t voice_id : voiceList)
    {
        auto& v = m3dData[voice_id];

        auto vol = v.mCollider != nullptr ? v.mCollider->collide(this, v, v.mColliderData) : 1.0f;

        auto       pos = v.m3dPosition;
        const auto vel = v.m3dVelocity;

        if (!v.mFlags.ListenerRelative)
        {
            pos = pos - lpos;
        }

        const float dist = length(pos);

        // attenuation

        if (v.mAttenuator != nullptr)
        {
            vol *= v.mAttenuator->attenuate(dist,
                                            v.m3dMinDistance,
                                            v.m3dMaxDistance,
                                            v.m3dAttenuationRolloff);
        }
        else
        {
            switch (v.m3dAttenuationModel)
            {
                case AttenuationModel::InverseDistance: vol *= attenuateInvDistance(dist,
                                                            v.m3dMinDistance,
                                                            v.m3dMaxDistance,
                                                            v.m3dAttenuationRolloff);
                    break;
                case AttenuationModel::LinearDistance: vol *= attenuateLinearDistance(dist,
                                                           v.m3dMinDistance,
                                                           v.m3dMaxDistance,
                                                           v.m3dAttenuationRolloff);
                    break;
                case AttenuationModel::ExponentialDistance: vol *= attenuateExponentialDistance(
                                                                dist,
                                                                v.m3dMinDistance,
                                                                v.m3dMaxDistance,
                                                                v.m3dAttenuationRolloff);
                    break;
                default:
                    // case AudioSource::NO_ATTENUATION:
                    break;
            }
        }

        // cone
        // (todo) vol *= conev;

        // doppler
        v.mDopplerValue = doppler(pos, vel, lvel, v.m3dDopplerFactor, m3dSoundSpeed);

        // panning
        pos = normalize(m * pos);

        v.mChannelVolume = {};

        // Apply volume to channels based on speaker vectors
        for (size_t j = 0; j < mChannels; ++j)
        {
            float speakervol = (dot(speaker[j], pos) + 1) / 2;
            if (is_zero(speaker[j]))
                speakervol = 1;
            // Different speaker "focus" calculations to try, if the default "bleeds" too much..
            // speakervol = (speakervol * speakervol + speakervol) / 2;
            // speakervol = speakervol * speakervol;
            v.mChannelVolume[j] = vol * speakervol;
        }

        v.m3dVolume = vol;
    }
}

void AudioDevice::update3dAudio()
{
    size_t voicecount = 0;
    size_t voices[voice_count];

    // Step 1 - find voices that need 3d processing
    lockAudioMutex_internal();
    for (size_t i = 0; i < mHighestVoice; ++i)
    {
        if (mVoice[i] && mVoice[i]->mFlags.Process3D)
        {
            voices[voicecount] = i;
            voicecount++;
            m3dData[i].mFlags = mVoice[i]->mFlags;
        }
    }
    unlockAudioMutex_internal();

    // Step 2 - do 3d processing

    update3dVoices_internal({voices, voicecount});

    // Step 3 - update SoLoud voices

    lockAudioMutex_internal();
    for (size_t i = 0; i < voicecount; ++i)
    {
        auto& v = m3dData[voices[i]];

        if (const auto& vi = mVoice[voices[i]])
        {
            updateVoiceRelativePlaySpeed_internal(voices[i]);
            updateVoiceVolume_internal(voices[i]);
            for (size_t j = 0; j < max_channels; ++j)
            {
                vi->mChannelVolume[j] = v.mChannelVolume[j];
            }

            if (vi->mOverallVolume < 0.001f)
            {
                // Inaudible.
                vi->mFlags.Inaudible = true;

                if (vi->mFlags.InaudibleKill)
                {
                    stopVoice_internal(voices[i]);
                }
            }
            else
            {
                vi->mFlags.Inaudible = false;
            }
        }
    }

    mActiveVoiceDirty = true;
    unlockAudioMutex_internal();
}


handle AudioDevice::play3d(
    AudioSource& aSound,
    Vector3      aPos,
    Vector3      aVel,
    float        aVolume,
    bool         aPaused,
    size_t       aBus)
{
    const handle h = play(aSound, aVolume, 0, true, aBus);
    lockAudioMutex_internal();
    auto v = getVoiceFromHandle_internal(h);

    if (v < 0)
    {
        unlockAudioMutex_internal();
        return h;
    }

    m3dData[v].mHandle          = h;
    mVoice[v]->mFlags.Process3D = true;

    set3dSourceParameters(h, aPos, aVel);

    int samples = 0;
    if (aSound.distance_delay)
    {
        const auto pos = mVoice[v]->mFlags.ListenerRelative ? aPos : aPos - m3dPosition;

        const float dist = length(pos);
        samples += int(floor(dist / m3dSoundSpeed * float(mSamplerate)));
    }

    update3dVoices_internal({reinterpret_cast<size_t*>(&v), 1});
    updateVoiceRelativePlaySpeed_internal(v);

    for (size_t j = 0; j < max_channels; ++j)
    {
        mVoice[v]->mChannelVolume[j] = m3dData[v].mChannelVolume[j];
    }

    updateVoiceVolume_internal(v);

    // Fix initial voice volume ramp up
    for (size_t i = 0; i < max_channels; ++i)
    {
        mVoice[v]->mCurrentChannelVolume[i] =
            mVoice[v]->mChannelVolume[i] * mVoice[v]->mOverallVolume;
    }

    if (mVoice[v]->mOverallVolume < 0.01f)
    {
        // Inaudible.
        mVoice[v]->mFlags.Inaudible = true;

        if (mVoice[v]->mFlags.InaudibleKill)
        {
            stopVoice_internal(v);
        }
    }
    else
    {
        mVoice[v]->mFlags.Inaudible = false;
    }

    mActiveVoiceDirty = true;

    unlockAudioMutex_internal();
    setDelaySamples(h, samples);
    setPause(h, aPaused);

    return h;
}

handle AudioDevice::play3dClocked(
    time_t       sound_time,
    AudioSource& sound,
    Vector3      pos,
    Vector3      vel,
    float        volume,
    size_t       bus)
{
    const handle h = play(sound, volume, 0, 1, bus);
    lockAudioMutex_internal();
    int v = getVoiceFromHandle_internal(h);
    if (v < 0)
    {
        unlockAudioMutex_internal();
        return h;
    }
    m3dData[v].mHandle          = h;
    mVoice[v]->mFlags.Process3D = true;
    set3dSourceParameters(h, pos, vel);
    time_t lasttime = mLastClockedTime;
    if (lasttime == 0)
    {
        lasttime         = sound_time;
        mLastClockedTime = sound_time;
    }
    unlockAudioMutex_internal();

    auto samples = int(floor((sound_time - lasttime) * mSamplerate));

    // Make sure we don't delay too much (or overflow)
    if (samples < 0 || samples > 2048)
    {
        samples = 0;
    }

    if (sound.distance_delay)
    {
        const float dist = length(pos);
        samples += int(floor((dist / m3dSoundSpeed) * mSamplerate));
    }

    update3dVoices_internal({reinterpret_cast<size_t*>(&v), 1});
    lockAudioMutex_internal();
    updateVoiceRelativePlaySpeed_internal(v);

    for (size_t j = 0; j < max_channels; ++j)
    {
        mVoice[v]->mChannelVolume[j] = m3dData[v].mChannelVolume[j];
    }

    updateVoiceVolume_internal(v);

    // Fix initial voice volume ramp up
    for (size_t i = 0; i < max_channels; ++i)
    {
        mVoice[v]->mCurrentChannelVolume[i] =
            mVoice[v]->mChannelVolume[i] * mVoice[v]->mOverallVolume;
    }

    if (mVoice[v]->mOverallVolume < 0.01f)
    {
        // Inaudible.
        mVoice[v]->mFlags.Inaudible = true;

        if (mVoice[v]->mFlags.InaudibleKill)
        {
            stopVoice_internal(v);
        }
    }
    else
    {
        mVoice[v]->mFlags.Inaudible = false;
    }

    mActiveVoiceDirty = true;
    unlockAudioMutex_internal();

    setDelaySamples(h, samples);
    setPause(h, false);

    return h;
}


void AudioDevice::set3dSoundSpeed(float aSpeed)
{
    assert(aSpeed > 0.0f);
    m3dSoundSpeed = aSpeed;
}


float AudioDevice::get3dSoundSpeed() const
{
    return m3dSoundSpeed;
}


void AudioDevice::set3dListenerParameters(Vector3 pos, Vector3 at, Vector3 up, Vector3 velocity)
{
    m3dPosition = pos;
    m3dAt       = at;
    m3dUp       = up;
    m3dVelocity = velocity;
}


void AudioDevice::set3dListenerPosition(Vector3 value)
{
    m3dPosition = value;
}


void AudioDevice::set3dListenerAt(Vector3 value)
{
    m3dAt = value;
}


void AudioDevice::set3dListenerUp(Vector3 value)
{
    m3dUp = value;
}


void AudioDevice::set3dListenerVelocity(Vector3 value)
{
    m3dVelocity = value;
}


void AudioDevice::set3dSourceParameters(handle aVoiceHandle, Vector3 aPos, Vector3 aVelocity)
{
    FOR_ALL_VOICES_PRE_3D
        m3dData[ch].m3dPosition = aPos;
        m3dData[ch].m3dVelocity = aVelocity;
    FOR_ALL_VOICES_POST_3D
}


void AudioDevice::set3dSourcePosition(handle aVoiceHandle, Vector3 value)
{
    FOR_ALL_VOICES_PRE_3D
        m3dData[ch].m3dPosition = value;
    FOR_ALL_VOICES_POST_3D
}


void AudioDevice::set3dSourceVelocity(handle aVoiceHandle, Vector3 velocity)
{
    FOR_ALL_VOICES_PRE_3D
        m3dData[ch].m3dVelocity = velocity;
    FOR_ALL_VOICES_POST_3D
}


void AudioDevice::set3dSourceMinMaxDistance(handle aVoiceHandle,
                                            float  aMinDistance,
                                            float  aMaxDistance)
{
    FOR_ALL_VOICES_PRE_3D
        m3dData[ch].m3dMinDistance = aMinDistance;
        m3dData[ch].m3dMaxDistance = aMaxDistance;
    FOR_ALL_VOICES_POST_3D
}


void AudioDevice::set3dSourceAttenuation(handle           aVoiceHandle,
                                         AttenuationModel aAttenuationModel,
                                         float            aAttenuationRolloffFactor)
{
    FOR_ALL_VOICES_PRE_3D
        m3dData[ch].m3dAttenuationModel   = aAttenuationModel;
        m3dData[ch].m3dAttenuationRolloff = aAttenuationRolloffFactor;
    FOR_ALL_VOICES_POST_3D
}


void AudioDevice::set3dSourceDopplerFactor(handle aVoiceHandle, float aDopplerFactor)
{
    FOR_ALL_VOICES_PRE_3D
        m3dData[ch].m3dDopplerFactor = aDopplerFactor;
    FOR_ALL_VOICES_POST_3D
}

void AudioDevice::setGlobalFilter(size_t aFilterId, Filter* aFilter)
{
    if (aFilterId >= filters_per_stream)
    {
        return;
    }

    lockAudioMutex_internal();

    mFilter[aFilterId] = aFilter;
    if (aFilter)
    {
        mFilterInstance[aFilterId] = mFilter[aFilterId]->createInstance();
    }

    unlockAudioMutex_internal();
}

std::optional<float> AudioDevice::getFilterParameter(handle voice_handle,
                                                     size_t filter_id,
                                                     size_t attribute_id)
{
    if (filter_id >= filters_per_stream)
    {
        return {};
    }

    auto ret = std::optional<float>{};

    if (voice_handle == 0)
    {
        lockAudioMutex_internal();
        if (mFilterInstance[filter_id])
        {
            ret = mFilterInstance[filter_id]->getFilterParameter(attribute_id);
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
    if (mVoice[ch] && mVoice[ch]->mFilter[filter_id])
    {
        ret = mVoice[ch]->mFilter[filter_id]->getFilterParameter(attribute_id);
    }
    unlockAudioMutex_internal();

    return ret;
}

void AudioDevice::setFilterParameter(handle aVoiceHandle,
                                     size_t filter_id,
                                     size_t attribute_id,
                                     float  value)
{
    if (filter_id >= filters_per_stream)
    {
        return;
    }

    if (aVoiceHandle == 0)
    {
        lockAudioMutex_internal();
        if (mFilterInstance[filter_id])
        {
            mFilterInstance[filter_id]->setFilterParameter(attribute_id, value);
        }
        unlockAudioMutex_internal();
        return;
    }

    FOR_ALL_VOICES_PRE
        if (mVoice[ch] && mVoice[ch]->mFilter[filter_id])
        {
            mVoice[ch]->mFilter[filter_id]->setFilterParameter(attribute_id, value);
        }
    FOR_ALL_VOICES_POST
}

void AudioDevice::fadeFilterParameter(
    handle aVoiceHandle,
    size_t aFilterId,
    size_t aAttributeId,
    float  aTo,
    double aTime)
{
    if (aFilterId >= filters_per_stream)
    {
        return;
    }

    if (aVoiceHandle == 0)
    {
        lockAudioMutex_internal();
        if (mFilterInstance[aFilterId])
        {
            mFilterInstance[aFilterId]->fadeFilterParameter(aAttributeId, aTo, aTime, mStreamTime);
        }
        unlockAudioMutex_internal();
        return;
    }

    FOR_ALL_VOICES_PRE
        if (mVoice[ch] && mVoice[ch]->mFilter[aFilterId])
        {
            mVoice[ch]->mFilter[aFilterId]->fadeFilterParameter(
                aAttributeId,
                aTo,
                aTime,
                mStreamTime);
        }
    FOR_ALL_VOICES_POST
}

void AudioDevice::oscillateFilterParameter(handle aVoiceHandle,
                                           size_t aFilterId,
                                           size_t aAttributeId,
                                           float  aFrom,
                                           float  aTo,
                                           double aTime)
{
    if (aFilterId >= filters_per_stream)
    {
        return;
    }

    if (aVoiceHandle == 0)
    {
        lockAudioMutex_internal();
        if (mFilterInstance[aFilterId])
        {
            mFilterInstance[aFilterId]->oscillateFilterParameter(aAttributeId,
                                                                 aFrom,
                                                                 aTo,
                                                                 aTime,
                                                                 mStreamTime);
        }
        unlockAudioMutex_internal();
        return;
    }

    FOR_ALL_VOICES_PRE
        if (mVoice[ch] && mVoice[ch]->mFilter[aFilterId])
        {
            mVoice[ch]->mFilter[aFilterId]->oscillateFilterParameter(aAttributeId,
                aFrom,
                aTo,
                aTime,
                mStreamTime);
        }
    FOR_ALL_VOICES_POST
}

// Create a voice group. Returns 0 if unable (out of voice groups / out of memory)
handle AudioDevice::createVoiceGroup()
{
    lockAudioMutex_internal();

    size_t i;
    // Check if there's any deleted voice groups and re-use if found
    for (i = 0; i < mVoiceGroupCount; ++i)
    {
        if (mVoiceGroup[i] == nullptr)
        {
            mVoiceGroup[i] = new size_t[17];
            if (mVoiceGroup[i] == nullptr)
            {
                unlockAudioMutex_internal();
                return 0;
            }
            mVoiceGroup[i][0] = 16;
            mVoiceGroup[i][1] = 0;
            unlockAudioMutex_internal();
            return 0xfffff000 | i;
        }
    }
    if (mVoiceGroupCount == 4096)
    {
        unlockAudioMutex_internal();
        return 0;
    }
    size_t oldcount = mVoiceGroupCount;
    if (mVoiceGroupCount == 0)
    {
        mVoiceGroupCount = 4;
    }
    mVoiceGroupCount *= 2;
    size_t** vg = new size_t*[mVoiceGroupCount];
    if (vg == nullptr)
    {
        mVoiceGroupCount = oldcount;
        unlockAudioMutex_internal();
        return 0;
    }
    for (i = 0; i < oldcount; ++i)
    {
        vg[i] = mVoiceGroup[i];
    }

    for (; i < mVoiceGroupCount; ++i)
    {
        vg[i] = nullptr;
    }

    delete[] mVoiceGroup;
    mVoiceGroup    = vg;
    i              = oldcount;
    mVoiceGroup[i] = new size_t[17];
    if (mVoiceGroup[i] == nullptr)
    {
        unlockAudioMutex_internal();
        return 0;
    }
    mVoiceGroup[i][0] = 16;
    mVoiceGroup[i][1] = 0;
    unlockAudioMutex_internal();
    return 0xfffff000 | i;
}

// Destroy a voice group.
void AudioDevice::destroyVoiceGroup(handle aVoiceGroupHandle)
{
    if (!isVoiceGroup(aVoiceGroupHandle))
        return;

    const int c = aVoiceGroupHandle & 0xfff;

    lockAudioMutex_internal();
    delete[] mVoiceGroup[c];
    mVoiceGroup[c] = nullptr;
    unlockAudioMutex_internal();
}

// Add a voice handle to a voice group
void AudioDevice::addVoiceToGroup(handle aVoiceGroupHandle, handle aVoiceHandle)
{
    if (!isVoiceGroup(aVoiceGroupHandle))
        return;

    // Don't consider adding invalid voice handles as an error, since the voice may just have ended.
    if (!isValidVoiceHandle(aVoiceHandle))
        return;

    trimVoiceGroup_internal(aVoiceGroupHandle);

    const int c = aVoiceGroupHandle & 0xfff;

    lockAudioMutex_internal();

    for (size_t i = 1; i < mVoiceGroup[c][0]; ++i)
    {
        if (mVoiceGroup[c][i] == aVoiceHandle)
        {
            unlockAudioMutex_internal();
            return; // already there
        }

        if (mVoiceGroup[c][i] == 0)
        {
            mVoiceGroup[c][i]     = aVoiceHandle;
            mVoiceGroup[c][i + 1] = 0;

            unlockAudioMutex_internal();
            return;
        }
    }

    // Full group, allocate more memory
    const auto n = new size_t[mVoiceGroup[c][0] * 2 + 1];

    for (size_t i = 0; i < mVoiceGroup[c][0]; ++i)
    {
        n[i] = mVoiceGroup[c][i];
    }

    n[n[0]]     = aVoiceHandle;
    n[n[0] + 1] = 0;
    n[0] *= 2;
    delete[] mVoiceGroup[c];
    mVoiceGroup[c] = n;
    unlockAudioMutex_internal();
}

// Is this handle a valid voice group?
bool AudioDevice::isVoiceGroup(handle aVoiceGroupHandle)
{
    if ((aVoiceGroupHandle & 0xfffff000) != 0xfffff000)
    {
        return false;
    }

    const size_t c = aVoiceGroupHandle & 0xfff;
    if (c >= mVoiceGroupCount)
    {
        return false;
    }

    lockAudioMutex_internal();
    const bool res = mVoiceGroup[c] != nullptr;
    unlockAudioMutex_internal();

    return res;
}

// Is this voice group empty?
bool AudioDevice::isVoiceGroupEmpty(handle aVoiceGroupHandle)
{
    // If not a voice group, yeah, we're empty alright..
    if (!isVoiceGroup(aVoiceGroupHandle))
    {
        return true;
    }

    trimVoiceGroup_internal(aVoiceGroupHandle);
    const int c = aVoiceGroupHandle & 0xfff;

    lockAudioMutex_internal();
    const bool res = mVoiceGroup[c][1] == 0;
    unlockAudioMutex_internal();

    return res;
}

// Remove all non-active voices from group
void AudioDevice::trimVoiceGroup_internal(handle aVoiceGroupHandle)
{
    if (!isVoiceGroup(aVoiceGroupHandle))
    {
        return;
    }

    const int c = aVoiceGroupHandle & 0xfff;

    lockAudioMutex_internal();

    // empty group
    if (mVoiceGroup[c][1] == 0)
    {
        unlockAudioMutex_internal();
        return;
    }

    // first item in voice group is number of allocated indices
    for (size_t i = 1; i < mVoiceGroup[c][0]; ++i)
    {
        // If we hit a voice in the group that's not set, we're done
        if (mVoiceGroup[c][i] == 0)
        {
            unlockAudioMutex_internal();
            return;
        }

        unlockAudioMutex_internal();
        while (!isValidVoiceHandle(
            mVoiceGroup[c][i])) // function locks mutex, so we need to unlock it before the call
        {
            lockAudioMutex_internal();
            // current index is an invalid handle, move all following handles backwards
            for (size_t j = i; j < mVoiceGroup[c][0] - 1; ++j)
            {
                mVoiceGroup[c][j] = mVoiceGroup[c][j + 1];
                // not a full group, we can stop copying
                if (mVoiceGroup[c][j] == 0)
                    break;
            }
            // be sure to mark the last one as unused in any case
            mVoiceGroup[c][mVoiceGroup[c][0] - 1] = 0;
            // did we end up with an empty group? we're done then
            if (mVoiceGroup[c][i] == 0)
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

handle* AudioDevice::voiceGroupHandleToArray_internal(handle aVoiceGroupHandle) const
{
    if ((aVoiceGroupHandle & 0xfffff000) != 0xfffff000)
    {
        return nullptr;
    }

    const size_t c = aVoiceGroupHandle & 0xfff;
    if (c >= mVoiceGroupCount)
    {
        return nullptr;
    }

    if (mVoiceGroup[c] == nullptr)
    {
        return nullptr;
    }

    return mVoiceGroup[c] + 1;
}

void AudioDevice::setVoiceRelativePlaySpeed_internal(size_t aVoice, float aSpeed)
{
    assert(aVoice < voice_count);
    assert(mInsideAudioThreadMutex);
    assert(aSpeed > 0.0f);

    if (mVoice[aVoice])
    {
        mVoice[aVoice]->mSetRelativePlaySpeed = aSpeed;
        updateVoiceRelativePlaySpeed_internal(aVoice);
    }
}

void AudioDevice::setVoicePause_internal(size_t aVoice, int aPause)
{
    assert(aVoice < voice_count);
    assert(mInsideAudioThreadMutex);
    mActiveVoiceDirty = true;

    if (mVoice[aVoice])
    {
        mVoice[aVoice]->mPauseScheduler.mActive = 0;
        mVoice[aVoice]->mFlags.Paused           = aPause;
    }
}

void AudioDevice::setVoicePan_internal(size_t aVoice, float aPan)
{
    assert(aVoice < voice_count);
    assert(mInsideAudioThreadMutex);
    if (mVoice[aVoice])
    {
        mVoice[aVoice]->mPan              = aPan;
        const auto l                      = float(std::cos((aPan + 1) * M_PI / 4));
        const auto r                      = float(std::sin((aPan + 1) * M_PI / 4));
        mVoice[aVoice]->mChannelVolume[0] = l;
        mVoice[aVoice]->mChannelVolume[1] = r;
        if (mVoice[aVoice]->mChannels == 4)
        {
            mVoice[aVoice]->mChannelVolume[2] = l;
            mVoice[aVoice]->mChannelVolume[3] = r;
        }
        if (mVoice[aVoice]->mChannels == 6)
        {
            mVoice[aVoice]->mChannelVolume[2] = 1.0f / std::sqrt(2.0f);
            mVoice[aVoice]->mChannelVolume[3] = 1;
            mVoice[aVoice]->mChannelVolume[4] = l;
            mVoice[aVoice]->mChannelVolume[5] = r;
        }
        if (mVoice[aVoice]->mChannels == 8)
        {
            mVoice[aVoice]->mChannelVolume[2] = 1.0f / std::sqrt(2.0f);
            mVoice[aVoice]->mChannelVolume[3] = 1;
            mVoice[aVoice]->mChannelVolume[4] = l;
            mVoice[aVoice]->mChannelVolume[5] = r;
            mVoice[aVoice]->mChannelVolume[6] = l;
            mVoice[aVoice]->mChannelVolume[7] = r;
        }
    }
}

void AudioDevice::setVoiceVolume_internal(size_t aVoice, float aVolume)
{
    assert(aVoice < voice_count);
    assert(mInsideAudioThreadMutex);
    mActiveVoiceDirty = true;
    if (mVoice[aVoice])
    {
        mVoice[aVoice]->mSetVolume = aVolume;
        updateVoiceVolume_internal(aVoice);
    }
}

void AudioDevice::stopVoice_internal(size_t aVoice)
{
    assert(aVoice < voice_count);
    assert(mInsideAudioThreadMutex);
    mActiveVoiceDirty = true;
    if (mVoice[aVoice])
    {
        // Delete via temporary variable to avoid recursion
        auto v = mVoice[aVoice];
        mVoice[aVoice].reset();

        for (size_t i = 0; i < mMaxActiveVoices; ++i)
        {
            if (mResampleDataOwner[i].get() == v.get())
            {
                mResampleDataOwner[i].reset();
            }
        }
    }
}

void AudioDevice::updateVoiceRelativePlaySpeed_internal(size_t aVoice)
{
    assert(aVoice < voice_count);
    assert(mInsideAudioThreadMutex);
    mVoice[aVoice]->mOverallRelativePlaySpeed =
        m3dData[aVoice].mDopplerValue * mVoice[aVoice]->mSetRelativePlaySpeed;
    mVoice[aVoice]->mSamplerate =
        mVoice[aVoice]->mBaseSamplerate * mVoice[aVoice]->mOverallRelativePlaySpeed;
}

void AudioDevice::updateVoiceVolume_internal(size_t aVoice)
{
    assert(aVoice < voice_count);
    assert(mInsideAudioThreadMutex);
    mVoice[aVoice]->mOverallVolume = mVoice[aVoice]->mSetVolume * m3dData[aVoice].m3dVolume;
    if (mVoice[aVoice]->mFlags.Paused)
    {
        for (size_t i = 0; i < max_channels; ++i)
        {
            mVoice[aVoice]->mCurrentChannelVolume[i] =
                mVoice[aVoice]->mChannelVolume[i] * mVoice[aVoice]->mOverallVolume;
        }
    }
}
} // namespace cer::details
