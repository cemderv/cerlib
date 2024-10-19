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

#pragma once

#include "audio/Common.hpp"

namespace cer
{
class AudioDevice;

struct AudioBackendArgs
{
    AudioDevice* engine;
    EngineFlags  flags;
    size_t       sample_rate   = 44100;
    size_t       buffer        = 2048;
    size_t       channel_count = 2;
};

void sdl2static_init(const AudioBackendArgs& args);

void sdl3static_init(const AudioBackendArgs& args);

void coreaudio_init(const AudioBackendArgs& args);

void opensles_init(const AudioBackendArgs& args);

void winmm_init(const AudioBackendArgs& args);

void xaudio2_init(const AudioBackendArgs& args);

void wasapi_init(const AudioBackendArgs& args);

void alsa_init(const AudioBackendArgs& args);

// Interlace samples in a buffer. From 11112222 to 12121212
void interlace_samples_float(
    const float* src_buffer, float* dst_buffer, size_t samples, size_t channels, size_t stride);

// Convert to 16-bit and interlace samples in a buffer. From 11112222 to 12121212
void interlace_samples_s16(
    const float* src_buffer, short* dst_buffer, size_t samples, size_t channels, size_t stride);
}; // namespace cer

#define FOR_ALL_VOICES_PRE                                                                         \
    SoundHandle* h_     = nullptr;                                                                 \
    SoundHandle  th_[2] = {voice_handle, 0};                                                       \
    lockAudioMutex_internal();                                                                     \
    h_ = voiceGroupHandleToArray_internal(voice_handle);                                           \
    if (h_ == nullptr)                                                                             \
        h_ = th_;                                                                                  \
    while (*h_)                                                                                    \
    {                                                                                              \
        int ch = getVoiceFromHandle_internal(*h_);                                                 \
        if (ch != -1)                                                                              \
        {

#define FOR_ALL_VOICES_POST                                                                        \
    }                                                                                              \
    h_++;                                                                                          \
    }                                                                                              \
    unlockAudioMutex_internal();

#define FOR_ALL_VOICES_PRE_3D                                                                      \
    SoundHandle*               h_  = nullptr;                                                      \
    std::array<SoundHandle, 2> th_ = {voice_handle, 0};                                            \
    h_                             = voiceGroupHandleToArray_internal(voice_handle);               \
    if (h_ == nullptr)                                                                             \
        h_ = th_.data();                                                                           \
    while (*h_)                                                                                    \
    {                                                                                              \
        int ch = (*h_ & 0xfff) - 1;                                                                \
        if (ch != -1 && m_3d_data[ch].handle == *h_)                                               \
        {

#define FOR_ALL_VOICES_POST_3D                                                                     \
    }                                                                                              \
    h_++;                                                                                          \
    }

#define FOR_ALL_VOICES_PRE_EXT                                                                     \
    SoundHandle* h_     = nullptr;                                                                 \
    SoundHandle  th_[2] = {voice_handle, 0};                                                       \
    engine->lockAudioMutex_internal();                                                             \
    h_ = engine->voiceGroupHandleToArray_internal(voice_handle);                                   \
    if (h_ == nullptr)                                                                             \
        h_ = th_;                                                                                  \
    while (*h_)                                                                                    \
    {                                                                                              \
        int ch = engine->getVoiceFromHandle_internal(*h_);                                         \
        if (ch != -1)                                                                              \
        {

#define FOR_ALL_VOICES_POST_EXT                                                                    \
    }                                                                                              \
    h_++;                                                                                          \
    }                                                                                              \
    engine->unlockAudioMutex_internal();

#define FOR_ALL_VOICES_PRE_3D_EXT                                                                  \
    SoundHandle* h_     = nullptr;                                                                 \
    SoundHandle  th_[2] = {voice_handle, 0};                                                       \
    h_                  = engine->voiceGroupHandleToArray(voice_handle);                           \
    if (h_ == nullptr)                                                                             \
        h_ = th_;                                                                                  \
    while (*h_)                                                                                    \
    {                                                                                              \
        int ch = (*h_ & 0xfff) - 1;                                                                \
        if (ch != -1 && engine->m_3d_data[ch].handle == *h_)                                       \
        {

#define FOR_ALL_VOICES_POST_3D_EXT                                                                 \
    }                                                                                              \
    h_++;                                                                                          \
    }
