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
class MemoryReader;

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
  public:
    explicit SfxrInstance(Sfxr* parent);

    auto audio(float* buffer, size_t samples_to_read, size_t buffer_size) -> size_t override;

    auto has_ended() -> bool override;

  private:
    void reset_sample(bool restart);

    Sfxr*                   m_parent       = nullptr;
    Prg                     m_rand         = {};
    SfxrParams              m_params       = {};
    bool                    playing_sample = true;
    int                     phase          = 0;
    double                  fperiod        = 0.0;
    double                  fmaxperiod     = 0.0;
    double                  fslide         = 0.0;
    double                  fdslide        = 0.0;
    int                     period         = 0;
    float                   square_duty    = 0.0f;
    float                   square_slide   = 0.0f;
    int                     env_stage      = 0;
    int                     env_time       = 0;
    std::array<int, 3>      env_length     = {};
    float                   env_vol        = 0.0f;
    float                   fphase         = 0.0f;
    float                   fdphase        = 0.0f;
    int                     iphase         = 0;
    std::array<float, 1024> phaser_buffer  = {};
    int                     ipp            = 0;
    std::array<float, 32>   noise_buffer   = {};
    float                   fltp           = 0.0f;
    float                   fltdp          = 0.0f;
    float                   fltw           = 0.0f;
    float                   fltw_d         = 0.0f;
    float                   fltdmp         = 0.0f;
    float                   fltphp         = 0.0f;
    float                   flthp          = 0.0f;
    float                   flthp_d        = 0.0f;
    float                   vib_phase      = 0.0f;
    float                   vib_speed      = 0.0f;
    float                   vib_amp        = 0.0f;
    int                     rep_time       = 0;
    int                     rep_limit      = 0;
    int                     arp_time       = 0;
    int                     arp_limit      = 0;
    double                  arp_mod        = 0.0;
};

enum class SfxrPreset
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

    explicit Sfxr(SfxrPreset preset, int seed);

    ~Sfxr() override;

    auto create_instance() -> std::shared_ptr<AudioSourceInstance> override;

  private:
    SfxrParams m_params;
    Prg        m_rand;
};
}; // namespace cer
