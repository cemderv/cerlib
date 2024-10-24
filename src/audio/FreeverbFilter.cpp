/*
SoLoud audio engine
Copyright (c) 2013-2020 Jari Komppa

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

#include "audio/Filter.hpp"
#include <array>

namespace cer
{
namespace freeverb_impl
{
// Based on code written by Jezar at Dreampoint, June 2000 http://www.dreampoint.co.uk,
// which was placed in public domain. The code was massaged quite a bit by
// Jari Komppa, result in the license listed at top of this file.

class Comb
{
  public:
    void setbuffer(float* buf, int size);
    auto process(float inp) -> float;
    void mute();
    void setdamp(float val);
    void setfeedback(float val);

    float  feedback     = 0.0f;
    float  filterstore  = 0.0f;
    float  damp1        = 0.0f;
    float  damp2        = 0.0f;
    float* buffer       = nullptr;
    int    buffer_size  = 0;
    int    buffer_index = 0;
};

class Allpass
{
  public:
    Allpass();
    void setbuffer(float* buf, int size);
    auto process(float inp) -> float;
    void mute();
    void setfeedback(float val);

    float  feedback;
    float* buffer;
    int    buffer_size;
    int    buffer_index;
};

static constexpr int   s_numcombs     = 8;
static constexpr int   s_numallpasses = 4;
static constexpr float s_muted        = 0;
static constexpr float s_fixedgain    = 0.015f;
static constexpr float s_scalewet     = 3;
static constexpr float s_scaledry     = 2;
static constexpr float s_scaledamp    = 0.4f;
static constexpr float s_scaleroom    = 0.28f;
static constexpr float s_offsetroom   = 0.7f;
static constexpr float s_initialroom  = 0.5f;
static constexpr float s_initialdamp  = 0.5f;
static constexpr float s_initialwet   = 1 / s_scalewet;
static constexpr float s_initialdry   = 0;
static constexpr float s_initialwidth = 1;
static constexpr float s_initialmode  = 0;
static constexpr float s_freezemode   = 0.5f;
static constexpr int   s_stereospread = 23;

// These values assume 44.1KHz sample rate
// they will probably be OK for 48KHz sample rate
// but would need scaling for 96KHz (or other) sample rates.
// The values were obtained by listening tests.
static constexpr int s_combtuning_l1    = 1116;
static constexpr int s_combtuning_r1    = 1116 + s_stereospread;
static constexpr int s_combtuning_l2    = 1188;
static constexpr int s_combtuning_r2    = 1188 + s_stereospread;
static constexpr int s_combtuning_l3    = 1277;
static constexpr int s_combtuning_r3    = 1277 + s_stereospread;
static constexpr int s_combtuning_l4    = 1356;
static constexpr int s_combtuning_r4    = 1356 + s_stereospread;
static constexpr int s_combtuning_l5    = 1422;
static constexpr int s_combtuning_r5    = 1422 + s_stereospread;
static constexpr int s_combtuning_l6    = 1491;
static constexpr int s_combtuning_r6    = 1491 + s_stereospread;
static constexpr int s_combtuning_l7    = 1557;
static constexpr int s_combtuning_r7    = 1557 + s_stereospread;
static constexpr int s_combtuning_l8    = 1617;
static constexpr int s_combtuning_r8    = 1617 + s_stereospread;
static constexpr int s_allpasstuning_l1 = 556;
static constexpr int s_allpasstuning_r1 = 556 + s_stereospread;
static constexpr int s_allpasstuning_l2 = 441;
static constexpr int s_allpasstuning_r2 = 441 + s_stereospread;
static constexpr int s_allpasstuning_l3 = 341;
static constexpr int s_allpasstuning_r3 = 341 + s_stereospread;
static constexpr int s_allpasstuning_l4 = 225;
static constexpr int s_allpasstuning_r4 = 225 + s_stereospread;

class Revmodel
{
  public:
    Revmodel();

    void mute();
    void process(float* sample_data, size_t num_samples, size_t stride);
    void setroomsize(float value);
    void setdamp(float value);
    void setwet(float value);
    void setdry(float value);
    void setwidth(float value);
    void setmode(float value);
    void update();

    float gain;
    float room_size;
    float room_size1;
    float damp, damp1;
    float wet, wet1, wet2;
    float dry;
    float width;
    float mode;

    int dirty;

    // The following are all declared inline
    // to remove the need for dynamic allocation
    // with its subsequent error-checking messiness

    // Comb filters
    std::array<Comb, s_numcombs> comb_l{};
    std::array<Comb, s_numcombs> comb_r{};

    // Allpass filters
    std::array<Allpass, s_numallpasses> allpass_l{};
    std::array<Allpass, s_numallpasses> allpass_r{};

    // Buffers for the combs
    std::array<float, s_combtuning_l1> bufcomb_l1{};
    std::array<float, s_combtuning_r1> bufcomb_r1{};
    std::array<float, s_combtuning_l2> bufcomb_l2{};
    std::array<float, s_combtuning_r2> bufcomb_r2{};
    std::array<float, s_combtuning_l3> bufcomb_l3{};
    std::array<float, s_combtuning_r3> bufcomb_r3{};
    std::array<float, s_combtuning_l4> bufcomb_l4{};
    std::array<float, s_combtuning_r4> bufcomb_r4{};
    std::array<float, s_combtuning_l5> bufcomb_l5{};
    std::array<float, s_combtuning_r5> bufcomb_r5{};
    std::array<float, s_combtuning_l6> bufcomb_l6{};
    std::array<float, s_combtuning_r6> bufcomb_r6{};
    std::array<float, s_combtuning_l7> bufcomb_l7{};
    std::array<float, s_combtuning_r7> bufcomb_r7{};
    std::array<float, s_combtuning_l8> bufcomb_l8{};
    std::array<float, s_combtuning_r8> bufcomb_r8{};

    // Buffers for the allpasses
    std::array<float, s_allpasstuning_l1> bufallpass_l1{};
    std::array<float, s_allpasstuning_r1> bufallpass_r1{};
    std::array<float, s_allpasstuning_l2> bufallpass_l2{};
    std::array<float, s_allpasstuning_r2> bufallpass_r2{};
    std::array<float, s_allpasstuning_l3> bufallpass_l3{};
    std::array<float, s_allpasstuning_r3> bufallpass_r3{};
    std::array<float, s_allpasstuning_l4> bufallpass_l4{};
    std::array<float, s_allpasstuning_r4> bufallpass_r4{};
};

Allpass::Allpass()
{
    buffer_index = 0;
    feedback     = 0;
    buffer       = nullptr;
    buffer_size  = 0;
}

auto Allpass::process(float inp) -> float
{
    const float bufout   = buffer[buffer_index];
    const float output   = -inp + bufout;
    buffer[buffer_index] = inp + (bufout * feedback);

    if (++buffer_index >= buffer_size)
    {
        buffer_index = 0;
    }

    return output;
}

void Allpass::setbuffer(float* buf, int size)
{
    buffer      = buf;
    buffer_size = size;
}

void Allpass::mute()
{
    for (int i = 0; i < buffer_size; ++i)
    {
        buffer[i] = 0;
    }
}

void Allpass::setfeedback(float val)
{
    feedback = val;
}

auto Comb::process(float inp) -> float
{
    const float output = buffer[buffer_index];

    filterstore = (output * damp2) + (filterstore * damp1);

    buffer[buffer_index] = inp + (filterstore * feedback);

    if (++buffer_index >= buffer_size)
    {
        buffer_index = 0;
    }

    return output;
}

void Comb::setbuffer(float* buf, int size)
{
    buffer      = buf;
    buffer_size = size;
}

void Comb::mute()
{
    for (int i = 0; i < buffer_size; ++i)
    {
        buffer[i] = 0;
    }
}

void Comb::setdamp(float val)
{
    damp1 = val;
    damp2 = 1 - val;
}

void Comb::setfeedback(float val)
{
    feedback = val;
}

Revmodel::Revmodel()
{
    gain       = 0;
    room_size  = 0;
    room_size1 = 0;
    damp       = 0;
    damp1      = 0;
    wet        = 0;
    wet1       = 0;
    wet2       = 0;
    dry        = 0;
    width      = 0;
    mode       = 0;

    dirty = 1;

    // Tie the components to their buffers
    comb_l[0].setbuffer(bufcomb_l1.data(), s_combtuning_l1);
    comb_r[0].setbuffer(bufcomb_r1.data(), s_combtuning_r1);
    comb_l[1].setbuffer(bufcomb_l2.data(), s_combtuning_l2);
    comb_r[1].setbuffer(bufcomb_r2.data(), s_combtuning_r2);
    comb_l[2].setbuffer(bufcomb_l3.data(), s_combtuning_l3);
    comb_r[2].setbuffer(bufcomb_r3.data(), s_combtuning_r3);
    comb_l[3].setbuffer(bufcomb_l4.data(), s_combtuning_l4);
    comb_r[3].setbuffer(bufcomb_r4.data(), s_combtuning_r4);
    comb_l[4].setbuffer(bufcomb_l5.data(), s_combtuning_l5);
    comb_r[4].setbuffer(bufcomb_r5.data(), s_combtuning_r5);
    comb_l[5].setbuffer(bufcomb_l6.data(), s_combtuning_l6);
    comb_r[5].setbuffer(bufcomb_r6.data(), s_combtuning_r6);
    comb_l[6].setbuffer(bufcomb_l7.data(), s_combtuning_l7);
    comb_r[6].setbuffer(bufcomb_r7.data(), s_combtuning_r7);
    comb_l[7].setbuffer(bufcomb_l8.data(), s_combtuning_l8);
    comb_r[7].setbuffer(bufcomb_r8.data(), s_combtuning_r8);
    allpass_l[0].setbuffer(bufallpass_l1.data(), s_allpasstuning_l1);
    allpass_r[0].setbuffer(bufallpass_r1.data(), s_allpasstuning_r1);
    allpass_l[1].setbuffer(bufallpass_l2.data(), s_allpasstuning_l2);
    allpass_r[1].setbuffer(bufallpass_r2.data(), s_allpasstuning_r2);
    allpass_l[2].setbuffer(bufallpass_l3.data(), s_allpasstuning_l3);
    allpass_r[2].setbuffer(bufallpass_r3.data(), s_allpasstuning_r3);
    allpass_l[3].setbuffer(bufallpass_l4.data(), s_allpasstuning_l4);
    allpass_r[3].setbuffer(bufallpass_r4.data(), s_allpasstuning_r4);

    // Set default values
    allpass_l[0].setfeedback(0.5f);
    allpass_r[0].setfeedback(0.5f);
    allpass_l[1].setfeedback(0.5f);
    allpass_r[1].setfeedback(0.5f);
    allpass_l[2].setfeedback(0.5f);
    allpass_r[2].setfeedback(0.5f);
    allpass_l[3].setfeedback(0.5f);
    allpass_r[3].setfeedback(0.5f);
    setwet(s_initialwet);
    setroomsize(s_initialroom);
    setdry(s_initialdry);
    setdamp(s_initialdamp);
    setwidth(s_initialwidth);
    setmode(s_initialmode);

    // Buffer will be full of rubbish - so we MUST mute them
    mute();
}

void Revmodel::mute()
{
    if (mode >= s_freezemode)
    {
        return;
    }

    for (int i = 0; i < s_numcombs; ++i)
    {
        comb_l[i].mute();
        comb_r[i].mute();
    }
    for (int i = 0; i < s_numallpasses; ++i)
    {
        allpass_l[i].mute();
        allpass_r[i].mute();
    }
}

void Revmodel::process(float* sample_data, size_t num_samples, size_t stride)
{
    float* input_l = sample_data;
    float* input_r = sample_data + stride;

    if (dirty != 0)
    {
        update();
    }

    dirty = 0;

    while (num_samples-- > 0)
    {
        auto       out_r = 0.0f;
        auto       out_l = 0.0f;
        const auto input = (*input_l + *input_r) * gain;

        // Accumulate comb filters in parallel
        for (int i = 0; i < s_numcombs; ++i)
        {
            out_l += comb_l[i].process(input);
            out_r += comb_r[i].process(input);
        }

        // Feed through allpasses in series
        for (int i = 0; i < s_numallpasses; ++i)
        {
            out_l = allpass_l[i].process(out_l);
            out_r = allpass_r[i].process(out_r);
        }

        // Calculate output REPLACING anything already there
        *input_l = out_l * wet1 + out_r * wet2 + *input_l * dry;
        *input_r = out_r * wet1 + out_l * wet2 + *input_r * dry;

        // Increment sample pointers, allowing for interleave (if any)
        input_l++;
        input_r++;
    }
}

void Revmodel::update()
{
    // Recalculate internal values after parameter change

    wet1 = wet * (width / 2 + 0.5f);
    wet2 = wet * ((1 - width) / 2);

    if (mode >= s_freezemode)
    {
        room_size1 = 1;
        damp1      = 0;
        gain       = s_muted;
    }
    else
    {
        room_size1 = room_size;
        damp1      = damp;
        gain       = s_fixedgain;
    }

    for (int i = 0; i < s_numcombs; ++i)
    {
        comb_l[i].setfeedback(room_size1);
        comb_r[i].setfeedback(room_size1);
    }

    for (int i = 0; i < s_numcombs; ++i)
    {
        comb_l[i].setdamp(damp1);
        comb_r[i].setdamp(damp1);
    }
}

void Revmodel::setroomsize(float value)
{
    room_size = (value * s_scaleroom) + s_offsetroom;
    dirty     = 1;
}

void Revmodel::setdamp(float value)
{
    damp  = value * s_scaledamp;
    dirty = 1;
}

void Revmodel::setwet(float value)
{
    wet   = value * s_scalewet;
    dirty = 1;
}

void Revmodel::setdry(float value)
{
    dry = value * s_scaledry;
}

void Revmodel::setwidth(float value)
{
    width = value;
    dirty = 1;
}

void Revmodel::setmode(float value)
{
    mode  = value;
    dirty = 1;
}
} // namespace freeverb_impl

FreeverbFilterInstance::FreeverbFilterInstance(FreeverbFilter* parent)
    : m_parent(parent)
    , m_model(std::make_unique<freeverb_impl::Revmodel>())
{
    FilterInstance::init_params(5);

    m_params[FREEZE]   = parent->mode;
    m_params[ROOMSIZE] = parent->room_size;
    m_params[DAMP]     = parent->damp;
    m_params[WIDTH]    = parent->width;
    m_params[WET]      = 1;
}

void FreeverbFilterInstance::filter(const FilterArgs& args)
{
    assert(args.channels == 2); // Only stereo supported at this time

    if (m_params_changed != 0u)
    {
        m_model->setdamp(m_params[DAMP]);
        m_model->setmode(m_params[FREEZE]);
        m_model->setroomsize(m_params[ROOMSIZE]);
        m_model->setwidth(m_params[WIDTH]);
        m_model->setwet(m_params[WET]);
        m_model->setdry(1 - m_params[WET]);
        m_params_changed = 0;
    }

    m_model->process(args.buffer, args.samples, args.buffer_size);
}

auto FreeverbFilter::create_instance() -> SharedPtr<FilterInstance>
{
    return std::make_shared<FreeverbFilterInstance>(this);
}
} // namespace cer
