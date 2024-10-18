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

#include "audio/AudioSource.hpp"
#include "audio/AudioDevice.hpp"
#include <algorithm>
#include <ranges>

namespace cer
{
AudioSourceInstance3dData::AudioSourceInstance3dData(const AudioSource& source)
{
    attenuation_model_3d   = source.attenuation_model_3d;
    attenuation_rolloff_3d = source.attenuation_rolloff_3d;
    doppler_factor_3d      = source.doppler_factor_3d;
    max_distance_3d        = source.max_distance_3d;
    min_distance_3d        = source.min_distance_3d;
    collider               = source.collider;
    collider_data          = source.collider_data;
    attenuator             = source.attenuator;
    volume_3d              = 1.0f;
    doppler_value          = 1.0f;
}

AudioSourceInstance::AudioSourceInstance()
{
    // Default all volumes to 1.0 so sound behind N mix busses isn't super quiet.
    std::ranges::fill(channel_volume, 1.0f);
}

void AudioSourceInstance::init(const AudioSource& source, size_t play_index)
{
    this->play_index = play_index;
    base_sample_rate = source.base_sample_rate;
    sample_rate      = base_sample_rate;
    channel_count    = source.channel_count;
    stream_time      = 0.0f;
    stream_position  = 0.0f;
    loop_point       = source.loop_point;

    if (source.should_loop)
    {
        flags.Looping = true;
    }
    if (source.process_3d)
    {
        flags.Process3D = true;
    }
    if (source.listener_relative)
    {
        flags.ListenerRelative = true;
    }
    if (source.inaudible_kill)
    {
        flags.InaudibleKill = true;
    }
    if (source.inaudible_tick)
    {
        flags.InaudibleTick = true;
    }
    if (source.disable_autostop)
    {
        flags.DisableAutostop = true;
    }
}

auto AudioSourceInstance::rewind() -> bool
{
    return false;
}

auto AudioSourceInstance::seek(double seconds, float* scratch, size_t scratch_size) -> bool
{
    double offset = seconds - stream_position;
    if (offset <= 0)
    {
        if (!rewind())
        {
            // can't do generic seek backwards unless we can rewind.
            return false;
        }
        offset = seconds;
    }

    auto samples_to_discard = size_t(floor(sample_rate * offset));

    while (samples_to_discard != 0)
    {
        auto samples = scratch_size / channel_count;
        if (samples > samples_to_discard)
            samples = samples_to_discard;
        audio(scratch, samples, samples);
        samples_to_discard -= samples;
    }

    stream_position = seconds;

    return true;
}

AudioSource::~AudioSource() noexcept
{
    stop();
}

void AudioSource::set_filter(size_t filter_id, Filter* filter)
{
    if (filter_id >= filters_per_stream)
        return;

    this->filter[filter_id] = filter;
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
