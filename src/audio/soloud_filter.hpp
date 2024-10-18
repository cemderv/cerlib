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

#include "soloud_fader.hpp"
#include <array>
#include <memory>

namespace cer
{
    class FilterInstance
    {
    public:
        FilterInstance() = default;

        virtual ~FilterInstance() noexcept = default;

        virtual void initParams(int aNumParams);

        virtual void updateParams(time_t aTime);

        virtual void filter(float* aBuffer,
                            size_t aSamples,
                            size_t aBufferSize,
                            size_t aChannels,
                            float aSamplerate,
                            time_t aTime);

        virtual void filterChannel(float* aBuffer,
                                   size_t aSamples,
                                   float aSamplerate,
                                   time_t aTime,
                                   size_t aChannel,
                                   size_t aChannels);

        virtual float getFilterParameter(size_t aAttributeId);

        virtual void setFilterParameter(size_t aAttributeId, float aValue);

        virtual void fadeFilterParameter(size_t aAttributeId,
                                         float aTo,
                                         time_t aTime,
                                         time_t aStartTime);

        virtual void oscillateFilterParameter(
            size_t aAttributeId, float aFrom, float aTo, time_t aTime, time_t aStartTime);

    protected:
        size_t mNumParams = 0;
        size_t mParamChanged = 0;
        std::unique_ptr<float[]> mParam;
        std::unique_ptr<Fader[]> mParamFader;
    };

    class Filter
    {
    public:
        Filter() = default;

        virtual ~Filter() noexcept = default;

        virtual std::shared_ptr<FilterInstance> createInstance() = 0;
    };

    class FlangerFilter;

    class FlangerFilterInstance final : public FilterInstance
    {
    public:
        void filter(float* aBuffer,
                    size_t aSamples,
                    size_t aBufferSize,
                    size_t aChannels,
                    float aSamplerate,
                    time_t aTime) override;

        explicit FlangerFilterInstance(FlangerFilter* aParent);

    private:
        std::unique_ptr<float[]> mBuffer;
        size_t mBufferLength;
        FlangerFilter* mParent;
        size_t mOffset;
        double mIndex;
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

        float mDelay;
        float mFreq;
        std::shared_ptr<FilterInstance> createInstance() override;
    };

    class FreeverbFilter;

    namespace FreeverbImpl
    {
        class Revmodel;
    }

    class FreeverbFilterInstance final : public FilterInstance
    {
        enum FILTERPARAM
        {
            WET = 0,
            FREEZE,
            ROOMSIZE,
            DAMP,
            WIDTH
        };

        FreeverbFilter* mParent = nullptr;
        std::unique_ptr<FreeverbImpl::Revmodel> mModel;

    public:
        void filter(float* aBuffer,
                    size_t aSamples,
                    size_t aBufferSize,
                    size_t aChannels,
                    float aSamplerate,
                    time_t aTime) override;

        explicit FreeverbFilterInstance(FreeverbFilter* aParent);
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

        std::shared_ptr<FilterInstance> createInstance() override;

        float mMode = 0.0f;
        float mRoomSize = 0.5f;
        float mDamp = 0.5f;
        float mWidth = 1.0f;
    };

    class DuckFilter;

    class DuckFilterInstance final : public FilterInstance
    {
    public:
        void filter(float* aBuffer,
                    size_t aSamples,
                    size_t aBufferSize,
                    size_t aChannels,
                    float aSamplerate,
                    time_t aTime) override;

        explicit DuckFilterInstance(DuckFilter* aParent);

    private:
        handle mListenTo;
        AudioDevice* mEngine;
        float mCurrentLevel;
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

        std::shared_ptr<FilterInstance> createInstance() override;

        AudioDevice* mEngine = nullptr;
        float mOnRamp = 0.1f;
        float mOffRamp = 0.5f;
        float mLevel = 0.5f;
        handle mListenTo = 0;
    };

    class EchoFilter;

    class EchoFilterInstance final : public FilterInstance
    {
        std::unique_ptr<float[]> mBuffer;
        int mBufferLength;
        int mBufferMaxLength;
        int mOffset;

    public:
        void filter(float* aBuffer,
                    size_t aSamples,
                    size_t aBufferSize,
                    size_t aChannels,
                    float aSamplerate,
                    time_t aTime) override;

        explicit EchoFilterInstance(EchoFilter* aParent);
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

        std::shared_ptr<FilterInstance> createInstance() override;

        float mDelay = 0.3f;
        float mDecay = 0.7f;
        float mFilter = 0.0f;
    };

    class LofiFilter;

    struct LofiChannelData
    {
        float mSample;
        float mSamplesToSkip;
    };

    class LofiFilterInstance final : public FilterInstance
    {
        enum FILTERPARAMS
        {
            WET,
            SAMPLERATE,
            BITDEPTH
        };

        std::array<LofiChannelData, 2> mChannelData{};

        LofiFilter* mParent;

    public:
        void filterChannel(float* aBuffer,
                           size_t aSamples,
                           float aSamplerate,
                           time_t aTime,
                           size_t aChannel,
                           size_t aChannels) override;

        explicit LofiFilterInstance(LofiFilter* aParent);
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

        std::shared_ptr<FilterInstance> createInstance() override;

        float mSampleRate = 4000.0f;
        float mBitdepth = 3.0f;
    };

    class WaveShaperFilter;

    class WaveShaperFilterInstance final : public FilterInstance
    {
        WaveShaperFilter* mParent;

    public:
        void filterChannel(float* aBuffer,
                           size_t aSamples,
                           float aSamplerate,
                           time_t aTime,
                           size_t aChannel,
                           size_t aChannels) override;

        explicit WaveShaperFilterInstance(WaveShaperFilter* aParent);
    };

    class WaveShaperFilter final : public Filter
    {
    public:
        enum FILTERPARAMS
        {
            WET = 0,
            AMOUNT
        };

        std::shared_ptr<FilterInstance> createInstance() override;

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
        void filterChannel(float* aBuffer,
                           size_t aSamples,
                           float aSamplerate,
                           time_t aTime,
                           size_t aChannel,
                           size_t aChannels) override;
        explicit RobotizeFilterInstance(RobotizeFilter* aParent);
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

        std::shared_ptr<FilterInstance> createInstance() override;

        float mFreq = 30.0f;
        int mWave = 0;
    };

    class FFTFilter;

    class FFTFilterInstance : public FilterInstance
    {
    public:
        virtual void fftFilterChannel(float* aFFTBuffer,
                                      size_t aSamples,
                                      float aSamplerate,
                                      time_t aTime,
                                      size_t aChannel,
                                      size_t aChannels);
        void filterChannel(float* aBuffer,
                           size_t aSamples,
                           float aSamplerate,
                           time_t aTime,
                           size_t aChannel,
                           size_t aChannels) override;

        explicit FFTFilterInstance(FFTFilter* aParent);
        FFTFilterInstance();

        void comp2MagPhase(float* aFFTBuffer, size_t aSamples);
        void magPhase2MagFreq(float* aFFTBuffer, size_t aSamples, float aSamplerate, size_t aChannel);
        void magFreq2MagPhase(float* aFFTBuffer, size_t aSamples, float aSamplerate, size_t aChannel);
        void magPhase2Comp(float* aFFTBuffer, size_t aSamples);

    private:
        std::unique_ptr<float[]> mTemp;
        std::unique_ptr<float[]> mInputBuffer;
        std::unique_ptr<float[]> mMixBuffer;
        std::unique_ptr<float[]> mLastPhase;
        std::unique_ptr<float[]> mSumPhase;
        size_t mInputOffset[max_channels];
        size_t mMixOffset[max_channels];
        size_t mReadOffset[max_channels];
        FFTFilter* mParent;
    };

    class FFTFilter : public Filter
    {
    public:
        std::shared_ptr<FilterInstance> createInstance() override;
    };

    class EqFilter;

    class EqFilterInstance final : public FFTFilterInstance
    {
        enum FILTERATTRIBUTE
        {
            WET = 0,
            BAND1 = 1,
            BAND2 = 2,
            BAND3 = 3,
            BAND4 = 4,
            BAND5 = 5,
            BAND6 = 6,
            BAND7 = 7,
            BAND8 = 8
        };

        EqFilter* mParent;

    public:
        void fftFilterChannel(float* aFFTBuffer,
                              size_t aSamples,
                              float aSamplerate,
                              time_t aTime,
                              size_t aChannel,
                              size_t aChannels) override;
        explicit EqFilterInstance(EqFilter* aParent);
    };

    class EqFilter final : public FFTFilter
    {
    public:
        enum FILTERATTRIBUTE
        {
            WET = 0,
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

        std::shared_ptr<FilterInstance> createInstance() override;

        std::array<float, 8> mVolume{};
    };

    class BiquadResonantFilter;

    struct BQRStateData
    {
        float mY1, mY2, mX1, mX2;
    };

    class BiquadResonantFilterInstance final : public FilterInstance
    {
    public:
        explicit BiquadResonantFilterInstance(BiquadResonantFilter* aParent);

        void filterChannel(float* aBuffer,
                           size_t aSamples,
                           float aSamplerate,
                           time_t aTime,
                           size_t aChannel,
                           size_t aChannels) override;

    protected:
        enum FilterAttribute
        {
            Wet = 0,
            Type,
            Frequency,
            Resonance
        };

        std::array<BQRStateData, 8> mState{};
        float mA0 = 0.0f;
        float mA1 = 0.0f;
        float mA2 = 0.0f;
        float mB1 = 0.0f;
        float mB2 = 0.0f;
        int mDirty{};
        float mSamplerate;

        BiquadResonantFilter* mParent;
        void calcBQRParams();
    };

    class BiquadResonantFilter final : public Filter
    {
    public:
        enum FILTERTYPE
        {
            LOWPASS = 0,
            HIGHPASS = 1,
            BANDPASS = 2
        };

        enum FILTERATTRIBUTE
        {
            WET = 0,
            TYPE,
            FREQUENCY,
            RESONANCE
        };

        std::shared_ptr<FilterInstance> createInstance() override;

        int mFilterType = LOWPASS;
        float mFrequency = 1000.0f;
        float mResonance = 2.0f;
    };
}; // namespace cer
