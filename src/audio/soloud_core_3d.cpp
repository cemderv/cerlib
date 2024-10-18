/*
SoLoud audio engine
Copyright (c) 2013-2015 Jari Komppa

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

#include "cerlib/Vector3.hpp"
#include "soloud_internal.hpp"
#include <array>
#include <cmath>

// 3d audio operations

namespace cer
{
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

void Engine::update3dVoices_internal(std::span<const size_t> voiceList)
{
    auto speaker = std::array<Vector3, MAX_CHANNELS>{};

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

void Engine::update3dAudio()
{
    size_t voicecount = 0;
    size_t voices[VOICE_COUNT];

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
            for (size_t j = 0; j < MAX_CHANNELS; ++j)
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


handle Engine::play3d(
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

    for (size_t j = 0; j < MAX_CHANNELS; ++j)
    {
        mVoice[v]->mChannelVolume[j] = m3dData[v].mChannelVolume[j];
    }

    updateVoiceVolume_internal(v);

    // Fix initial voice volume ramp up
    for (size_t i = 0; i < MAX_CHANNELS; ++i)
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

handle Engine::play3dClocked(
    time_t       aSoundTime,
    AudioSource& aSound,
    Vector3      aPos,
    Vector3      aVel,
    float        aVolume,
    size_t       aBus)
{
    const handle h = play(aSound, aVolume, 0, 1, aBus);
    lockAudioMutex_internal();
    int v = getVoiceFromHandle_internal(h);
    if (v < 0)
    {
        unlockAudioMutex_internal();
        return h;
    }
    m3dData[v].mHandle          = h;
    mVoice[v]->mFlags.Process3D = true;
    set3dSourceParameters(h, aPos, aVel);
    time_t lasttime = mLastClockedTime;
    if (lasttime == 0)
    {
        lasttime         = aSoundTime;
        mLastClockedTime = aSoundTime;
    }
    unlockAudioMutex_internal();

    auto samples = int(floor((aSoundTime - lasttime) * mSamplerate));

    // Make sure we don't delay too much (or overflow)
    if (samples < 0 || samples > 2048)
    {
        samples = 0;
    }

    if (aSound.distance_delay)
    {
        const float dist = length(aPos);
        samples += int(floor((dist / m3dSoundSpeed) * mSamplerate));
    }

    update3dVoices_internal({reinterpret_cast<size_t*>(&v), 1});
    lockAudioMutex_internal();
    updateVoiceRelativePlaySpeed_internal(v);

    for (size_t j = 0; j < MAX_CHANNELS; ++j)
    {
        mVoice[v]->mChannelVolume[j] = m3dData[v].mChannelVolume[j];
    }

    updateVoiceVolume_internal(v);

    // Fix initial voice volume ramp up
    for (size_t i = 0; i < MAX_CHANNELS; ++i)
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


void Engine::set3dSoundSpeed(float aSpeed)
{
    assert(aSpeed > 0.0f);
    m3dSoundSpeed = aSpeed;
}


float Engine::get3dSoundSpeed() const
{
    return m3dSoundSpeed;
}


void Engine::set3dListenerParameters(Vector3 pos, Vector3 at, Vector3 up, Vector3 velocity)
{
    m3dPosition = pos;
    m3dAt       = at;
    m3dUp       = up;
    m3dVelocity = velocity;
}


void Engine::set3dListenerPosition(Vector3 value)
{
    m3dPosition = value;
}


void Engine::set3dListenerAt(Vector3 value)
{
    m3dAt = value;
}


void Engine::set3dListenerUp(Vector3 value)
{
    m3dUp = value;
}


void Engine::set3dListenerVelocity(Vector3 value)
{
    m3dVelocity = value;
}


void Engine::set3dSourceParameters(handle aVoiceHandle, Vector3 aPos, Vector3 aVelocity)
{
    FOR_ALL_VOICES_PRE_3D
        m3dData[ch].m3dPosition = aPos;
        m3dData[ch].m3dVelocity = aVelocity;
    FOR_ALL_VOICES_POST_3D
}


void Engine::set3dSourcePosition(handle aVoiceHandle, Vector3 value)
{
    FOR_ALL_VOICES_PRE_3D
        m3dData[ch].m3dPosition = value;
    FOR_ALL_VOICES_POST_3D
}


void Engine::set3dSourceVelocity(handle aVoiceHandle, Vector3 velocity)
{
    FOR_ALL_VOICES_PRE_3D
        m3dData[ch].m3dVelocity = velocity;
    FOR_ALL_VOICES_POST_3D
}


void Engine::set3dSourceMinMaxDistance(handle aVoiceHandle, float aMinDistance, float aMaxDistance)
{
    FOR_ALL_VOICES_PRE_3D
        m3dData[ch].m3dMinDistance = aMinDistance;
        m3dData[ch].m3dMaxDistance = aMaxDistance;
    FOR_ALL_VOICES_POST_3D
}


void Engine::set3dSourceAttenuation(handle           aVoiceHandle,
                                    AttenuationModel aAttenuationModel,
                                    float            aAttenuationRolloffFactor)
{
    FOR_ALL_VOICES_PRE_3D
        m3dData[ch].m3dAttenuationModel   = aAttenuationModel;
        m3dData[ch].m3dAttenuationRolloff = aAttenuationRolloffFactor;
    FOR_ALL_VOICES_POST_3D
}


void Engine::set3dSourceDopplerFactor(handle aVoiceHandle, float aDopplerFactor)
{
    FOR_ALL_VOICES_PRE_3D
        m3dData[ch].m3dDopplerFactor = aDopplerFactor;
    FOR_ALL_VOICES_POST_3D
}
}; // namespace cer
