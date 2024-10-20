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

#include "audio/Common.hpp"
#include "audio/Fader.hpp"
#include <array>
#include <memory>

namespace cer
{
struct FilterArgs
{
    float*    buffer      = nullptr;
    size_t    samples     = 0;
    size_t    buffer_size = 0;
    size_t    channels    = 0;
    float     sample_rate = 0.0f;
    SoundTime time        = {};
};

struct FilterChannelArgs
{
    float*    buffer        = nullptr;
    size_t    samples       = 0;
    float     sample_rate   = 0.0f;
    SoundTime time          = {};
    size_t    channel       = 0;
    size_t    channel_count = 0;
};

class FilterInstance
{
  public:
    FilterInstance() = default;

    virtual ~FilterInstance() noexcept = default;

    virtual void init_params(int param_count);

    virtual void update_params(SoundTime time);

    virtual void filter(const FilterArgs& args);

    virtual void filter_channel(const FilterChannelArgs& args);

    virtual auto filter_parameter(size_t attribute_id) -> float;

    virtual void set_filter_parameter(size_t attribute_id, float value);

    virtual void fade_filter_parameter(size_t    attribute_id,
                                       float     to,
                                       SoundTime time,
                                       SoundTime start_time);

    virtual void oscillate_filter_parameter(
        size_t attribute_id, float from, float to, SoundTime time, SoundTime start_time);

  protected:
    size_t                   m_param_count    = 0;
    size_t                   m_params_changed = 0;
    std::unique_ptr<float[]> m_params{};
    std::unique_ptr<Fader[]> m_param_faders{};
};

class Filter
{
  public:
    Filter() = default;

    virtual ~Filter() noexcept = default;

    virtual auto create_instance() -> std::shared_ptr<FilterInstance> = 0;
};

class FlangerFilter;

class FlangerFilterInstance final : public FilterInstance
{
  public:
    void filter(const FilterArgs& args) override;

    explicit FlangerFilterInstance(FlangerFilter* parent);

  private:
    std::unique_ptr<float[]> m_buffer;
    size_t                   m_buffer_size = 0;
    FlangerFilter*           m_parent      = nullptr;
    size_t                   m_offset      = 0;
    double                   m_index       = 0.0;
};

class FlangerFilter final : public Filter
{
  public:
    enum FILTERPARAMS
    {
        WET,
        DELAY,
        FREQ
    };

    FlangerFilter();

    auto create_instance() -> std::shared_ptr<FilterInstance> override;

    float m_delay = 0.0f;
    float m_freq  = 0.0f;
};

class FreeverbFilter;

namespace freeverb_impl
{
class Revmodel;
}

class FreeverbFilterInstance final : public FilterInstance
{
  public:
    void filter(const FilterArgs& args) override;

    explicit FreeverbFilterInstance(FreeverbFilter* parent);

  private:
    enum FILTERPARAM
    {
        WET = 0,
        FREEZE,
        ROOMSIZE,
        DAMP,
        WIDTH
    };

    FreeverbFilter*                          m_parent = nullptr;
    std::unique_ptr<freeverb_impl::Revmodel> m_model;
};

class FreeverbFilter final : public Filter
{
  public:
    enum FILTERPARAM
    {
        WET = 0,
        FREEZE,
        ROOMSIZE,
        DAMP,
        WIDTH
    };

    auto create_instance() -> std::shared_ptr<FilterInstance> override;

    float mode      = 0.0f;
    float room_size = 0.5f;
    float damp      = 0.5f;
    float width     = 1.0f;
};

class DuckFilter;

class DuckFilterInstance final : public FilterInstance
{
  public:
    explicit DuckFilterInstance(const DuckFilter* parent);

    void filter(const FilterArgs& args) override;

  private:
    SoundHandle  m_listen_to;
    AudioDevice* m_engine;
    float        m_current_level;
};

class DuckFilter final : public Filter
{
  public:
    enum FILTERATTRIBUTE
    {
        WET = 0,
        ONRAMP,
        OFFRAMP,
        LEVEL
    };

    std::shared_ptr<FilterInstance> create_instance() override;

    AudioDevice* mEngine   = nullptr;
    float        mOnRamp   = 0.1f;
    float        mOffRamp  = 0.5f;
    float        mLevel    = 0.5f;
    SoundHandle  mListenTo = 0;
};

class EchoFilter;

class EchoFilterInstance final : public FilterInstance
{
  public:
    void filter(const FilterArgs& args) override;

    explicit EchoFilterInstance(const EchoFilter* parent);

  private:
    std::unique_ptr<float[]> m_buffer;
    size_t                   m_buffer_size     = 0;
    size_t                   m_buffer_max_size = 0;
    size_t                   m_offset          = 0;
};

class EchoFilter final : public Filter
{
  public:
    enum FILTERATTRIBUTE
    {
        WET = 0,
        DELAY,
        DECAY,
        FILTER
    };

    std::shared_ptr<FilterInstance> create_instance() override;

    float delay  = 0.3f;
    float decay  = 0.7f;
    float filter = 0.0f;
};

class LofiFilter;

struct LofiChannelData
{
    float mSample;
    float mSamplesToSkip;
};

class LofiFilterInstance final : public FilterInstance
{
  public:
    void filter_channel(const FilterChannelArgs& args) override;

    explicit LofiFilterInstance(LofiFilter* parent);

  private:
    enum FILTERPARAMS
    {
        WET,
        SAMPLERATE,
        BITDEPTH
    };

    LofiFilter*                    mParent;
    std::array<LofiChannelData, 2> mChannelData{};
};

class LofiFilter final : public Filter
{
  public:
    enum FILTERPARAMS
    {
        WET,
        SAMPLERATE,
        BITDEPTH
    };

    std::shared_ptr<FilterInstance> create_instance() override;

    float mSampleRate = 4000.0f;
    float mBitdepth   = 3.0f;
};

class WaveShaperFilter;

class WaveShaperFilterInstance final : public FilterInstance
{
    WaveShaperFilter* mParent;

  public:
    void filter_channel(const FilterChannelArgs& args) override;

    explicit WaveShaperFilterInstance(WaveShaperFilter* parent);
};

class WaveShaperFilter final : public Filter
{
  public:
    enum FILTERPARAMS
    {
        WET = 0,
        AMOUNT
    };

    std::shared_ptr<FilterInstance> create_instance() override;

    float mAmount = 0.0f;
};

class RobotizeFilter;

class RobotizeFilterInstance final : public FilterInstance
{
    enum FILTERATTRIBUTE
    {
        WET = 0,
        FREQ,
        WAVE
    };

    RobotizeFilter* mParent;

  public:
    void filter_channel(const FilterChannelArgs& args) override;

    explicit RobotizeFilterInstance(RobotizeFilter* parent);
};

class RobotizeFilter final : public Filter
{
  public:
    enum FILTERATTRIBUTE
    {
        WET = 0,
        FREQ,
        WAVE
    };

    auto create_instance() -> std::shared_ptr<FilterInstance> override;

    float mFreq = 30.0f;
    int   mWave = 0;
};

class FFTFilter;

class FFTFilterInstance : public FilterInstance
{
  public:
    FFTFilterInstance();

    explicit FFTFilterInstance(FFTFilter* parent);

    virtual void fft_filter_channel(const FilterChannelArgs& args);

    void filter_channel(const FilterChannelArgs& args) override;

    static void comp2MagPhase(float* fft_buffer, size_t samples);

    void magPhase2MagFreq(float* fft_buffer, size_t samples, float sample_rate, size_t channel);

    void magFreq2MagPhase(float* fft_buffer, size_t samples, float sample_rate, size_t channel);

    void magPhase2Comp(float* fft_buffer, size_t samples);

  private:
    std::unique_ptr<float[]>         m_temp;
    std::unique_ptr<float[]>         m_input_buffer;
    std::unique_ptr<float[]>         m_mix_buffer;
    std::unique_ptr<float[]>         m_last_phase;
    std::unique_ptr<float[]>         m_sum_phase;
    std::array<size_t, max_channels> m_input_offset{};
    std::array<size_t, max_channels> m_mix_offset{};
    std::array<size_t, max_channels> m_read_offset{};
    FFTFilter*                       m_parent = nullptr;
};

class FFTFilter : public Filter
{
  public:
    auto create_instance() -> std::shared_ptr<FilterInstance> override;
};

class EqFilter;

class EqFilterInstance final : public FFTFilterInstance
{
    enum FILTERATTRIBUTE
    {
        WET   = 0,
        BAND1 = 1,
        BAND2 = 2,
        BAND3 = 3,
        BAND4 = 4,
        BAND5 = 5,
        BAND6 = 6,
        BAND7 = 7,
        BAND8 = 8
    };

    EqFilter* m_parent = nullptr;

  public:
    void fft_filter_channel(const FilterChannelArgs& args) override;

    explicit EqFilterInstance(EqFilter* parent);
};

class EqFilter final : public FFTFilter
{
  public:
    enum FILTERATTRIBUTE
    {
        WET   = 0,
        BAND1 = 1,
        BAND2 = 2,
        BAND3 = 3,
        BAND4 = 4,
        BAND5 = 5,
        BAND6 = 6,
        BAND7 = 7,
        BAND8 = 8
    };

    EqFilter();

    auto create_instance() -> std::shared_ptr<FilterInstance> override;

    std::array<float, 8> m_volume{};
};

class BiquadResonantFilter;

struct BQRStateData
{
    float y1 = 0.0f;
    float y2 = 0.0f;
    float x1 = 0.0f;
    float x2 = 0.0f;
};

class BiquadResonantFilterInstance final : public FilterInstance
{
  public:
    explicit BiquadResonantFilterInstance(BiquadResonantFilter* parent);

    void filter_channel(const FilterChannelArgs& args) override;

  protected:
    enum FilterAttribute
    {
        Wet = 0,
        Type,
        Frequency,
        Resonance
    };

    void calc_bqr_params();

    BiquadResonantFilter*       m_parent      = nullptr;
    std::array<BQRStateData, 8> m_state       = {};
    float                       m_a0          = 0.0f;
    float                       m_a1          = 0.0f;
    float                       m_a2          = 0.0f;
    float                       m_b1          = 0.0f;
    float                       m_b2          = 0.0f;
    int                         m_dirty       = {};
    float                       m_sample_rate = 44'100.0f;
};

class BiquadResonantFilter final : public Filter
{
  public:
    enum class FilterType
    {
        LowPass  = 0,
        HighPass = 1,
        BandPass = 2
    };

    enum class FilterAttribute
    {
        Wet = 0,
        Type,
        Frequency,
        Resonance
    };

    auto create_instance() -> std::shared_ptr<FilterInstance> override;

    FilterType filter_type = FilterType::LowPass;
    float      frequency   = 1000.0f;
    float      resonance   = 2.0f;
};
}; // namespace cer
