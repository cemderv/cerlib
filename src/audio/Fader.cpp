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

#include <algorithm>

namespace cer
{
void Fader::set(float from, float to, double time, double start_time)
{
    current          = from;
    this->from       = from;
    this->to         = to;
    this->time       = time;
    this->start_time = start_time;
    delta            = to - from;
    end_time         = start_time + time;
    active           = 1;
}

void Fader::setLFO(float from, float to, double time, double start_time)
{
    active     = 2;
    current    = 0;
    this->from = from;
    this->to   = to;
    this->time = time;
    delta      = (to - from) / 2;
    if (delta < 0)
    {
        delta = -delta;
    }
    this->start_time = start_time;
    end_time         = two_pi / time;
}

auto Fader::get(double current_time) -> float
{
    if (active == 2)
    {
        // LFO mode
        start_time   = min(start_time, current_time);
        const auto t = current_time - start_time;
        return float((sin(t * end_time) * delta) + (from + delta));
    }
    if (start_time > current_time)
    {
        // Time rolled over.
        // Figure out where we were..
        const float p = (current - from) / delta; // 0..1
        from          = current;
        start_time    = current_time;
        time          = time * (1 - p); // time left
        delta         = to - from;
        end_time      = start_time + time;
    }
    if (current_time > end_time)
    {
        active = -1;
        return to;
    }
    current = float(from + (delta * ((current_time - start_time) / time)));
    return current;
}
}; // namespace cer
