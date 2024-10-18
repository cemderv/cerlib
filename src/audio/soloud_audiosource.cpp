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
#include "audio/soloud_engine.hpp"
#include <algorithm>
#include <ranges>

namespace cer
{
AudioSourceInstance3dData::AudioSourceInstance3dData(AudioSource& aSource)
{
    m3dAttenuationModel   = aSource.attenuation_model_3d;
    m3dAttenuationRolloff = aSource.attenuation_rolloff_3d;
    m3dDopplerFactor      = aSource.doppler_factor_3d;
    m3dMaxDistance        = aSource.max_distance_3d;
    m3dMinDistance        = aSource.min_distance_3d;
    mCollider             = aSource.collider;
    mColliderData         = aSource.collider_data;
    mAttenuator           = aSource.attenuator;
    m3dVolume             = 1.0f;
    mDopplerValue         = 1.0f;
}

AudioSourceInstance::AudioSourceInstance()
{
    // Default all volumes to 1.0 so sound behind N mix busses isn't super quiet.
    std::ranges::fill(mChannelVolume, 1.0f);
}

void AudioSourceInstance::init(AudioSource& aSource, int aPlayIndex)
{
    mPlayIndex      = aPlayIndex;
    mBaseSamplerate = aSource.base_sample_rate;
    mSamplerate     = mBaseSamplerate;
    mChannels       = aSource.channel_count;
    mStreamTime     = 0.0f;
    mStreamPosition = 0.0f;
    mLoopPoint      = aSource.loop_point;

    if (aSource.should_loop)
    {
        mFlags.Looping = true;
    }
    if (aSource.process_3d)
    {
        mFlags.Process3D = true;
    }
    if (aSource.listener_relative)
    {
        mFlags.ListenerRelative = true;
    }
    if (aSource.inaudible_kill)
    {
        mFlags.InaudibleKill = true;
    }
    if (aSource.inaudible_tick)
    {
        mFlags.InaudibleTick = true;
    }
    if (aSource.disable_autostop)
    {
        mFlags.DisableAutostop = true;
    }
}

bool AudioSourceInstance::rewind()
{
    return false;
}

bool AudioSourceInstance::seek(double aSeconds, float* mScratch, size_t mScratchSize)
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
    int samples_to_discard = (int)floor(mSamplerate * offset);

    while (samples_to_discard)
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
    if (aFilterId >= FILTERS_PER_STREAM)
        return;

    filter[aFilterId] = aFilter;
}

void AudioSource::stop()
{
    if (engine)
    {
        engine->stopAudioSource(*this);
    }
}

float AudioSourceInstance::getInfo(size_t /*aInfoKey*/)
{
    return 0;
}
}; // namespace cer
