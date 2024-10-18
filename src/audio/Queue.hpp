/*
SoLoud audio engine
Copyright (c) 2013-2018 Jari Komppa

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

#include "audio/AudioSource.hpp"

#define SOLOUD_QUEUE_MAX 32

namespace cer
{
class Queue;

class QueueInstance final : public AudioSourceInstance
{
    Queue* mParent = nullptr;

  public:
    explicit QueueInstance(Queue* aParent);

    size_t audio(float* aBuffer, size_t aSamplesToRead, size_t aBufferSize) override;

    bool has_ended() override;
};

class Queue final : public AudioSource
{
  public:
    std::shared_ptr<AudioSourceInstance> create_instance() override;

    // Play sound through the queue
    void play(AudioSource& aSound);

    // Number of audio sources queued for replay
    size_t getQueueCount() const;

    // Is this audio source currently playing?
    bool isCurrentlyPlaying(const AudioSource& aSound) const;

    // Set params by reading them from an audio source
    void setParamsFromAudioSource(const AudioSource& aSound);

    // Set params manually
    void setParams(float aSamplerate, size_t aChannels = 2);

    void findQueueHandle();

    size_t                                                             mReadIndex  = 0;
    size_t                                                             mWriteIndex = 0;
    size_t                                                             mCount      = 0;
    std::array<std::shared_ptr<AudioSourceInstance>, SOLOUD_QUEUE_MAX> mSource{};
    std::shared_ptr<QueueInstance>                                     mInstance;
    SoundHandle                                                        mQueueHandle = 0;
};
}; // namespace cer
