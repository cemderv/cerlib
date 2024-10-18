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

#pragma once

#include "audio/AudioSource.hpp"
#include "audio/Misc.hpp"
#include <array>
#include <span>

namespace cer
{
class MemoryFile;

struct SfxrParams
{
    int wave_type = 0;

    float p_base_freq  = 0.3f;
    float p_freq_limit = 0.0f;
    float p_freq_ramp  = 0.0f;
    float p_freq_dramp = 0.0f;
    float p_duty       = 0.0f;
    float p_duty_ramp  = 0.0f;

    float p_vib_strength = 0.0f;
    float p_vib_speed    = 0.0f;
    float p_vib_delay    = 0.0f;

    float p_env_attack  = 0.0f;
    float p_env_sustain = 0.3f;
    float p_env_decay   = 0.4f;
    float p_env_punch   = 0.0f;

    bool  filter_on       = false;
    float p_lpf_resonance = 0.0f;
    float p_lpf_freq      = 1.0f;
    float p_lpf_ramp      = 0.0f;
    float p_hpf_freq      = 0.0f;
    float p_hpf_ramp      = 0.0f;

    float p_pha_offset = 0.0f;
    float p_pha_ramp   = 0.0f;

    float p_repeat_speed = 0.0f;

    float p_arp_speed = 0.0f;
    float p_arp_mod   = 0.0f;

    float master_vol = 0.05f;
    float sound_vol  = 0.5f;
};

class Sfxr;

class SfxrInstance final : public AudioSourceInstance
{
    Sfxr* mParent;

    Prg        mRand;
    SfxrParams mParams;

    bool                    playing_sample;
    int                     phase;
    double                  fperiod;
    double                  fmaxperiod;
    double                  fslide;
    double                  fdslide;
    int                     period;
    float                   square_duty;
    float                   square_slide;
    int                     env_stage;
    int                     env_time;
    int                     env_length[3];
    float                   env_vol;
    float                   fphase;
    float                   fdphase;
    int                     iphase;
    std::array<float, 1024> phaser_buffer{};
    int                     ipp;
    std::array<float, 32>   noise_buffer{};
    float                   fltp;
    float                   fltdp;
    float                   fltw;
    float                   fltw_d;
    float                   fltdmp;
    float                   fltphp;
    float                   flthp;
    float                   flthp_d;
    float                   vib_phase;
    float                   vib_speed;
    float                   vib_amp;
    int                     rep_time;
    int                     rep_limit;
    int                     arp_time;
    int                     arp_limit;
    double                  arp_mod;

    void resetSample(bool aRestart);

  public:
    explicit SfxrInstance(Sfxr* aParent);

    size_t audio(float* aBuffer, size_t aSamplesToRead, size_t aBufferSize) override;

    bool has_ended() override;
};

enum class SFXR_PRESETS
{
    COIN,
    LASER,
    EXPLOSION,
    POWERUP,
    HURT,
    JUMP,
    BLIP
};

class Sfxr final : public AudioSource
{
    friend SfxrInstance;

  public:
    explicit Sfxr(std::span<const std::byte> data);

    explicit Sfxr(int aPresetNo, int aRandSeed);

    ~Sfxr() override;

    std::shared_ptr<AudioSourceInstance> create_instance() override;

  private:
    SfxrParams mParams;
    Prg        mRand;
};
}; // namespace cer
