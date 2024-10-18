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

#pragma once

#include "cerlib/Audio.hpp"

namespace cer
{
// Helper class to process faders
class Fader
{
  public:
    // Set up LFO
    void setLFO(float aFrom, float aTo, SoundTime aTime, SoundTime aStartTime);

    // Set up fader
    void set(float aFrom, float aTo, SoundTime aTime, SoundTime aStartTime);

    // Get the current fading value
    float get(SoundTime aCurrentTime);

    // Value to fade from
    float mFrom = 0.0f;

    // Value to fade to
    float mTo = 0.0f;

    // Delta between from and to
    float mDelta = 0.0f;

    // Total time to fade
    SoundTime mTime = 0;

    // Time fading started
    SoundTime mStartTime = 0;

    // Time fading will end
    SoundTime mEndTime = 0;

    // Current value. Used in case time rolls over.
    float mCurrent = 0.0f;

    // Active flag; 0 means disabled, 1 is active, 2 is LFO, -1 means was active, but stopped
    int mActive = 0;
};
}; // namespace cer
