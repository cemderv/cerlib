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

#include "audio/Fader.hpp"

namespace cer
{
void Fader::set(float aFrom, float aTo, double aTime, double aStartTime)
{
    current   = from;
    from      = aFrom;
    to        = aTo;
    time      = aTime;
    start_time = aStartTime;
    delta     = aTo - aFrom;
    end_time   = start_time + time;
    active    = 1;
}

void Fader::setLFO(float aFrom, float aTo, double aTime, double aStartTime)
{
    active  = 2;
    current = 0;
    from    = aFrom;
    to      = aTo;
    time    = aTime;
    delta   = (aTo - aFrom) / 2;
    if (delta < 0)
        delta = -delta;
    start_time = aStartTime;
    end_time   = float(M_PI) * 2 / time;
}

float Fader::get(double aCurrentTime)
{
    if (active == 2)
    {
        // LFO mode
        if (start_time > aCurrentTime)
        {
            // Time rolled over.
            start_time = aCurrentTime;
        }
        const auto t = aCurrentTime - start_time;
        return float(sin(t * end_time) * delta + (from + delta));
    }
    if (start_time > aCurrentTime)
    {
        // Time rolled over.
        // Figure out where we were..
        const float p = (current - from) / delta; // 0..1
        from         = current;
        start_time    = aCurrentTime;
        time         = time * (1 - p); // time left
        delta        = to - from;
        end_time      = start_time + time;
    }
    if (aCurrentTime > end_time)
    {
        active = -1;
        return to;
    }
    current = float(from + delta * ((aCurrentTime - start_time) / time));
    return current;
}
}; // namespace cer
