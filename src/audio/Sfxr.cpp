/*
SFXR module for SoLoud audio engine
Copyright (c) 2014 Jari Komppa
Based on code (c) by Tomas Pettersson, re-licensed under zlib by permission

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

#include "audio/Sfxr.hpp"
#include "util/MemoryReader.hpp"

namespace cer
{
SfxrInstance::SfxrInstance(Sfxr* parent)
    : m_parent(parent)
{
    m_params = parent->m_params;
    m_rand.srand(0x792352);
    reset_sample(false);
}

#define frnd(x) ((float)(m_rand.rand() % 10001) / 10000 * (x))

auto SfxrInstance::audio(float* aBuffer, size_t aSamplesToRead, size_t /*aBufferSize*/) -> size_t
{
    float* buffer = aBuffer;
    size_t i;
    for (i = 0; i < aSamplesToRead; ++i)
    {
        rep_time++;
        if (rep_limit != 0 && rep_time >= rep_limit)
        {
            rep_time = 0;
            reset_sample(true);
        }

        // frequency envelopes/arpeggios
        arp_time++;
        if (arp_limit != 0 && arp_time >= arp_limit)
        {
            arp_limit = 0;
            fperiod *= arp_mod;
        }
        fslide += fdslide;
        fperiod *= fslide;
        if (fperiod > fmaxperiod)
        {
            fperiod = fmaxperiod;
            if (m_params.p_freq_limit > 0.0f)
            {
                if (flags.loops)
                {
                    reset_sample(false);
                }
                else
                {
                    playing_sample = false;
                    return i;
                }
            }
        }
        auto rfperiod = float(fperiod);
        if (vib_amp > 0.0f)
        {
            vib_phase += vib_speed;
            rfperiod = float(fperiod * (1.0 + sin(vib_phase) * vib_amp));
        }
        period = int(rfperiod);
        if (period < 8)
            period = 8;
        square_duty += square_slide;
        if (square_duty < 0.0f)
            square_duty = 0.0f;
        if (square_duty > 0.5f)
            square_duty = 0.5f;
        // volume envelope
        env_time++;
        if (env_time > env_length[env_stage])
        {
            env_time = 0;
            env_stage++;
            if (env_stage == 3)
            {
                if (flags.loops)
                {
                    reset_sample(false);
                }
                else
                {
                    playing_sample = false;
                    return i;
                }
            }
        }
        if (env_stage == 0)
        {
            if (env_length[0] != 0)
            {
                env_vol = float(env_time) / env_length[0];
            }
            else
            {
                env_vol = 0;
            }
        }
        if (env_stage == 1)
        {
            if (env_length[1] != 0)
            {
                env_vol = 1.0f + pow(1.0f - (float(env_time) / env_length[1]), 1.0f) * 2.0f *
                                     m_params.p_env_punch;
            }
            else
            {
                env_vol = 0;
            }
        }
        if (env_stage == 2)
        {
            if (env_length[2] != 0)
            {
                env_vol = 1.0f - float(env_time) / env_length[2];
            }
            else
            {
                env_vol = 0;
            }
        }

        // phaser step
        fphase += fdphase;
        iphase = abs((int)fphase);
        iphase = std::min(iphase, 1023);

        if (flthp_d != 0.0f)
        {
            flthp *= flthp_d;
            flthp = std::max(flthp, 0.00001f);
            flthp = std::min(flthp, 0.1f);
        }

        float ssample = 0.0f;
        for (int si = 0; si < 8; si++) // 8x supersampling
        {
            float sample = 0.0f;
            phase++;
            if (phase >= period)
            {
                phase %= period;
                if (m_params.wave_type == 3)
                {
                    for (float& k : noise_buffer)
                    {
                        k = frnd(2.0f) - 1.0f;
                    }
                }
            }
            // base waveform
            const float fp = float(phase) / period;
            switch (m_params.wave_type)
            {
                case 0: // square
                    if (fp < square_duty)
                    {
                        sample = 0.5f;
                    }
                    else
                    {
                        sample = -0.5f;
                    }
                    break;
                case 1: // sawtooth
                    sample = 1.0f - fp * 2;
                    break;
                case 2: // sine
                    sample = float(sin(fp * cer::two_pi));
                    break;
                case 3: // noise
                    sample = noise_buffer[phase * 32 / period];
                    break;
                default: break;
            }
            // lp filter
            const float pp = fltp;
            fltw *= fltw_d;
            fltw = std::max(fltw, 0.0f);
            fltw = std::min(fltw, 0.1f);
            if (m_params.p_lpf_freq != 1.0f)
            {
                fltdp += (sample - fltp) * fltw;
                fltdp -= fltdp * fltdmp;
            }
            else
            {
                fltp  = sample;
                fltdp = 0.0f;
            }
            fltp += fltdp;
            // hp filter
            fltphp += fltp - pp;
            fltphp -= fltphp * flthp;
            sample = fltphp;
            // phaser
            phaser_buffer[ipp & 1023] = sample;
            sample += phaser_buffer[(ipp - iphase + 1024) & 1023];
            ipp = (ipp + 1) & 1023;
            // final accumulation and envelope application
            ssample += sample * env_vol;
        }
        ssample = ssample / 8 * m_params.master_vol;

        ssample *= 2.0f * m_params.sound_vol;

        if (buffer != nullptr)
        {
            ssample = std::min(ssample, 1.0f);
            ssample = std::max(ssample, -1.0f);
            *buffer = ssample;
            buffer++;
        }
    }
    return aSamplesToRead;
}

auto SfxrInstance::has_ended() -> bool
{
    return !playing_sample;
}

void SfxrInstance::reset_sample(bool restart)
{
    if (!restart)
    {
        phase = 0;
    }

    fperiod      = 100.0 / (m_params.p_base_freq * m_params.p_base_freq + 0.001);
    period       = int(fperiod);
    fmaxperiod   = 100.0 / (m_params.p_freq_limit * m_params.p_freq_limit + 0.001);
    fslide       = 1.0 - pow(double(m_params.p_freq_ramp), 3.0) * 0.01;
    fdslide      = -pow(double(m_params.p_freq_dramp), 3.0) * 0.000001;
    square_duty  = 0.5f - m_params.p_duty * 0.5f;
    square_slide = -m_params.p_duty_ramp * 0.00005f;

    if (m_params.p_arp_mod >= 0.0f)
    {
        arp_mod = 1.0 - pow(double(m_params.p_arp_mod), 2.0) * 0.9;
    }
    else
    {
        arp_mod = 1.0 + pow(double(m_params.p_arp_mod), 2.0) * 10.0;
    }

    arp_time  = 0;
    arp_limit = int((pow(1.0f - m_params.p_arp_speed, 2.0f) * 20000) + 32);

    if (m_params.p_arp_speed == 1.0f)
    {
        arp_limit = 0;
    }

    if (!restart)
    {
        // reset filter
        fltp   = 0.0f;
        fltdp  = 0.0f;
        fltw   = pow(m_params.p_lpf_freq, 3.0f) * 0.1f;
        fltw_d = 1.0f + m_params.p_lpf_ramp * 0.0001f;

        fltdmp = 5.0f / (1.0f + pow(m_params.p_lpf_resonance, 2.0f) * 20.0f) * (0.01f + fltw);
        fltdmp = std::min(fltdmp, 0.8f);

        fltphp  = 0.0f;
        flthp   = pow(m_params.p_hpf_freq, 2.0f) * 0.1f;
        flthp_d = float(1.0 + m_params.p_hpf_ramp * 0.0003f);
        // reset vibrato
        vib_phase = 0.0f;
        vib_speed = pow(m_params.p_vib_speed, 2.0f) * 0.01f;
        vib_amp   = m_params.p_vib_strength * 0.5f;
        // reset envelope
        env_vol       = 0.0f;
        env_stage     = 0;
        env_time      = 0;
        env_length[0] = int(m_params.p_env_attack * m_params.p_env_attack * 100000.0f);
        env_length[1] = int(m_params.p_env_sustain * m_params.p_env_sustain * 100000.0f);
        env_length[2] = int(m_params.p_env_decay * m_params.p_env_decay * 100000.0f);

        fphase = pow(m_params.p_pha_offset, 2.0f) * 1020.0f;

        if (m_params.p_pha_offset < 0.0f)
        {
            fphase = -fphase;
        }

        fdphase = pow(m_params.p_pha_ramp, 2.0f) * 1.0f;

        if (m_params.p_pha_ramp < 0.0f)
        {
            fdphase = -fdphase;
        }

        iphase = abs(int(fphase));
        ipp    = 0;

        std::ranges::fill(phaser_buffer, 0.0f);

        for (auto& i : noise_buffer)
        {
            i = frnd(2.0f) - 1.0f;
        }

        rep_time  = 0;
        rep_limit = int((pow(1.0f - m_params.p_repeat_speed, 2.0f) * 20000) + 32);

        if (m_params.p_repeat_speed == 0.0f)
        {
            rep_limit = 0;
        }
    }
}


#define rnd(n) (m_rand.rand() % ((n) + 1))
#undef frnd
#define frnd(x) ((float)(m_rand.rand() % 10001) / 10000 * (x))

Sfxr::Sfxr(SfxrPreset preset, int seed)
{
    m_rand.srand(seed);

    switch (preset)
    {
        case SfxrPreset::COIN: {
            m_params.p_base_freq   = 0.4f + frnd(0.5f);
            m_params.p_env_attack  = 0.0f;
            m_params.p_env_sustain = frnd(0.1f);
            m_params.p_env_decay   = 0.1f + frnd(0.4f);
            m_params.p_env_punch   = 0.3f + frnd(0.3f);
            if (rnd(1))
            {
                m_params.p_arp_speed = 0.5f + frnd(0.2f);
                m_params.p_arp_mod   = 0.2f + frnd(0.4f);
            }
            break;
        }
        case SfxrPreset::LASER: {
            m_params.wave_type = rnd(2);

            if (m_params.wave_type == 2 && rnd(1))
            {
                m_params.wave_type = rnd(1);
            }

            m_params.p_base_freq  = 0.5f + frnd(0.5f);
            m_params.p_freq_limit = m_params.p_base_freq - 0.2f - frnd(0.6f);
            if (m_params.p_freq_limit < 0.2f)
                m_params.p_freq_limit = 0.2f;
            m_params.p_freq_ramp = -0.15f - frnd(0.2f);
            if (rnd(2) == 0)
            {
                m_params.p_base_freq  = 0.3f + frnd(0.6f);
                m_params.p_freq_limit = frnd(0.1f);
                m_params.p_freq_ramp  = -0.35f - frnd(0.3f);
            }
            if (rnd(1))
            {
                m_params.p_duty      = frnd(0.5f);
                m_params.p_duty_ramp = frnd(0.2f);
            }
            else
            {
                m_params.p_duty      = 0.4f + frnd(0.5f);
                m_params.p_duty_ramp = -frnd(0.7f);
            }
            m_params.p_env_attack  = 0.0f;
            m_params.p_env_sustain = 0.1f + frnd(0.2f);
            m_params.p_env_decay   = frnd(0.4f);
            if (rnd(1))
                m_params.p_env_punch = frnd(0.3f);
            if (rnd(2) == 0)
            {
                m_params.p_pha_offset = frnd(0.2f);
                m_params.p_pha_ramp   = -frnd(0.2f);
            }
            if (rnd(1))
                m_params.p_hpf_freq = frnd(0.3f);
            break;
        }
        case SfxrPreset::EXPLOSION: {
            m_params.wave_type = 3;
            if (rnd(1))
            {
                m_params.p_base_freq = 0.1f + frnd(0.4f);
                m_params.p_freq_ramp = -0.1f + frnd(0.4f);
            }
            else
            {
                m_params.p_base_freq = 0.2f + frnd(0.7f);
                m_params.p_freq_ramp = -0.2f - frnd(0.2f);
            }
            m_params.p_base_freq *= m_params.p_base_freq;
            if (rnd(4) == 0)
                m_params.p_freq_ramp = 0.0f;
            if (rnd(2) == 0)
                m_params.p_repeat_speed = 0.3f + frnd(0.5f);
            m_params.p_env_attack  = 0.0f;
            m_params.p_env_sustain = 0.1f + frnd(0.3f);
            m_params.p_env_decay   = frnd(0.5f);
            if (rnd(1) == 0)
            {
                m_params.p_pha_offset = -0.3f + frnd(0.9f);
                m_params.p_pha_ramp   = -frnd(0.3f);
            }
            m_params.p_env_punch = 0.2f + frnd(0.6f);
            if (rnd(1))
            {
                m_params.p_vib_strength = frnd(0.7f);
                m_params.p_vib_speed    = frnd(0.6f);
            }
            if (rnd(2) == 0)
            {
                m_params.p_arp_speed = 0.6f + frnd(0.3f);
                m_params.p_arp_mod   = 0.8f - frnd(1.6f);
            }
            break;
        }
        case SfxrPreset::POWERUP: {
            if (rnd(1))
            {
                m_params.wave_type = 1;
            }
            else
            {
                m_params.p_duty = frnd(0.6f);
            }
            if (rnd(1))
            {
                m_params.p_base_freq    = 0.2f + frnd(0.3f);
                m_params.p_freq_ramp    = 0.1f + frnd(0.4f);
                m_params.p_repeat_speed = 0.4f + frnd(0.4f);
            }
            else
            {
                m_params.p_base_freq = 0.2f + frnd(0.3f);
                m_params.p_freq_ramp = 0.05f + frnd(0.2f);
                if (rnd(1))
                {
                    m_params.p_vib_strength = frnd(0.7f);
                    m_params.p_vib_speed    = frnd(0.6f);
                }
            }
            m_params.p_env_attack  = 0.0f;
            m_params.p_env_sustain = frnd(0.4f);
            m_params.p_env_decay   = 0.1f + frnd(0.4f);
            break;
        }
        case SfxrPreset::HURT: {
            m_params.wave_type = rnd(2);
            if (m_params.wave_type == 2)
            {
                m_params.wave_type = 3;
            }
            if (m_params.wave_type == 0)
            {
                m_params.p_duty = frnd(0.6f);
            }
            m_params.p_base_freq   = 0.2f + frnd(0.6f);
            m_params.p_freq_ramp   = -0.3f - frnd(0.4f);
            m_params.p_env_attack  = 0.0f;
            m_params.p_env_sustain = frnd(0.1f);
            m_params.p_env_decay   = 0.1f + frnd(0.2f);
            if (rnd(1))
            {
                m_params.p_hpf_freq = frnd(0.3f);
            }
            break;
        }
        case SfxrPreset::JUMP: {
            m_params.wave_type     = 0;
            m_params.p_duty        = frnd(0.6f);
            m_params.p_base_freq   = 0.3f + frnd(0.3f);
            m_params.p_freq_ramp   = 0.1f + frnd(0.2f);
            m_params.p_env_attack  = 0.0f;
            m_params.p_env_sustain = 0.1f + frnd(0.3f);
            m_params.p_env_decay   = 0.1f + frnd(0.2f);
            if (rnd(1))
            {
                m_params.p_hpf_freq = frnd(0.3f);
            }
            if (rnd(1))
            {
                m_params.p_lpf_freq = 1.0f - frnd(0.6f);
            }
            break;
        }
        case SfxrPreset::BLIP: {
            m_params.wave_type = rnd(1);
            if (m_params.wave_type == 0)
            {
                m_params.p_duty = frnd(0.6f);
            }
            m_params.p_base_freq   = 0.2f + frnd(0.4f);
            m_params.p_env_attack  = 0.0f;
            m_params.p_env_sustain = 0.1f + frnd(0.1f);
            m_params.p_env_decay   = frnd(0.2f);
            m_params.p_hpf_freq    = 0.1f;
            break;
        }
        default: break;
    }
}

Sfxr::Sfxr(std::span<const std::byte> data)
{
    auto mf = MemoryReader{data};

    const auto version = mf.read_s32();

    if (version != 100 && version != 101 && version != 102)
    {
        throw std::runtime_error{"Failed to load sfxr"};
    }

    m_params.wave_type = mf.read_s32();

    if (version == 102)
    {
        m_params.sound_vol = mf.read_f32();
    }

    m_params.p_base_freq  = mf.read_f32();
    m_params.p_freq_limit = mf.read_f32();
    m_params.p_freq_ramp  = mf.read_f32();

    if (version >= 101)
    {
        m_params.p_freq_dramp = mf.read_f32();
    }

    m_params.p_duty      = mf.read_f32();
    m_params.p_duty_ramp = mf.read_f32();

    m_params.p_vib_strength = mf.read_f32();
    m_params.p_vib_speed    = mf.read_f32();
    m_params.p_vib_delay    = mf.read_f32();

    m_params.p_env_attack  = mf.read_f32();
    m_params.p_env_sustain = mf.read_f32();
    m_params.p_env_decay   = mf.read_f32();
    m_params.p_env_punch   = mf.read_f32();

    m_params.filter_on       = mf.read_f32();
    m_params.p_lpf_resonance = mf.read_f32();
    m_params.p_lpf_freq      = mf.read_f32();
    m_params.p_lpf_ramp      = mf.read_f32();
    m_params.p_hpf_freq      = mf.read_f32();
    m_params.p_hpf_ramp      = mf.read_f32();

    m_params.p_pha_offset = mf.read_f32();
    m_params.p_pha_ramp   = mf.read_f32();

    m_params.p_repeat_speed = mf.read_f32();

    if (version >= 101)
    {
        m_params.p_arp_speed = mf.read_f32();
        m_params.p_arp_mod   = mf.read_f32();
    }
}

Sfxr::~Sfxr()
{
    stop();
}

auto Sfxr::create_instance() -> std::shared_ptr<AudioSourceInstance>
{
    return std::make_shared<SfxrInstance>(this);
}
}; // namespace cer