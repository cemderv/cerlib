/*
SoLoud audio engine
Copyright (c) 2013-2014 Jari Komppa

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

#include "audio/soloud_audiosource.hpp"
#include "audio/AudioDevice.hpp"
#include <algorithm>
#include <ranges>

namespace cer
{
AudioSourceInstance3dData::AudioSourceInstance3dData(const AudioSource& source)
{
    m3dAttenuationModel   = source.attenuation_model_3d;
    m3dAttenuationRolloff = source.attenuation_rolloff_3d;
    m3dDopplerFactor      = source.doppler_factor_3d;
    m3dMaxDistance        = source.max_distance_3d;
    m3dMinDistance        = source.min_distance_3d;
    mCollider             = source.collider;
    mColliderData         = source.collider_data;
    mAttenuator           = source.attenuator;
    m3dVolume             = 1.0f;
    mDopplerValue         = 1.0f;
}

AudioSourceInstance::AudioSourceInstance()
{
    // Default all volumes to 1.0 so sound behind N mix busses isn't super quiet.
    std::ranges::fill(mChannelVolume, 1.0f);
}

void AudioSourceInstance::init(const AudioSource& source, int aPlayIndex)
{
    mPlayIndex      = aPlayIndex;
    mBaseSamplerate = source.base_sample_rate;
    mSamplerate     = mBaseSamplerate;
    mChannels       = source.channel_count;
    mStreamTime     = 0.0f;
    mStreamPosition = 0.0f;
    mLoopPoint      = source.loop_point;

    if (source.should_loop)
    {
        mFlags.Looping = true;
    }
    if (source.process_3d)
    {
        mFlags.Process3D = true;
    }
    if (source.listener_relative)
    {
        mFlags.ListenerRelative = true;
    }
    if (source.inaudible_kill)
    {
        mFlags.InaudibleKill = true;
    }
    if (source.inaudible_tick)
    {
        mFlags.InaudibleTick = true;
    }
    if (source.disable_autostop)
    {
        mFlags.DisableAutostop = true;
    }
}

auto AudioSourceInstance::rewind() -> bool
{
    return false;
}

auto AudioSourceInstance::seek(double aSeconds, float* mScratch, size_t mScratchSize) -> bool
{
    double offset = aSeconds - mStreamPosition;
    if (offset <= 0)
    {
        if (!rewind())
        {
            // can't do generic seek backwards unless we can rewind.
            return false;
        }
        offset = aSeconds;
    }

    auto samples_to_discard = int(floor(mSamplerate * offset));

    while (samples_to_discard != 0)
    {
        int samples = mScratchSize / mChannels;
        if (samples > samples_to_discard)
            samples = samples_to_discard;
        getAudio(mScratch, samples, samples);
        samples_to_discard -= samples;
    }

    mStreamPosition = aSeconds;

    return true;
}

AudioSource::~AudioSource() noexcept
{
    stop();
}

void AudioSource::setFilter(size_t aFilterId, Filter* aFilter)
{
    if (aFilterId >= filters_per_stream)
        return;

    filter[aFilterId] = aFilter;
}

void AudioSource::stop()
{
    if (engine)
    {
        engine->stop_audio_source(*this);
    }
}

float AudioSourceInstance::getInfo(size_t /*aInfoKey*/)
{
    return 0;
}
}; // namespace cer
