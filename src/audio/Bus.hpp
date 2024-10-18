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

#pragma once

#include "audio/AudioSource.hpp"
#include "audio/Misc.hpp"
#include <array>

namespace cer
{
class Bus;
class DuckFilterInstance;

class BusInstance final : public AudioSourceInstance
{
    friend Bus;
    friend DuckFilterInstance;

  public:
    explicit BusInstance(Bus* parent);

    auto audio(float* buffer, size_t samples_to_read, size_t buffer_size) -> size_t override;

    auto has_ended() -> bool override;

    ~BusInstance() noexcept override;

  private:
    Bus*               m_parent;
    size_t             m_scratch_size;
    AlignedFloatBuffer m_scratch;

    // Approximate volume for channels.
    std::array<float, max_channels> m_visualization_channel_volume{};

    // Mono-mixed wave data for visualization and for visualization FFT input
    std::array<float, 256> m_visualization_wave_data{};
};

class Bus final : public AudioSource
{
    friend BusInstance;

  public:
    Bus();

    auto create_instance() -> std::shared_ptr<AudioSourceInstance> override;

    // Set filter. Set to nullptr to clear the filter.
    void set_filter(size_t filter_id, Filter* filter) override;

    // Play sound through the bus
    auto play(AudioSource& aSound, float aVolume = 1.0f, float aPan = 0.0f, bool aPaused = false)
        -> SoundHandle;

    // Play sound through the bus, delayed in relation to other sounds called via this function.
    auto play_clocked(SoundTime time, AudioSource& sound, float volume = 1.0f, float pan = 0.0f)
        -> SoundHandle;

    // Start playing a 3d audio source through the bus
    auto play3d(AudioSource& sound,
                Vector3      pos,
                Vector3      vel    = {},
                float        volume = 1.0f,
                bool         paused = false) -> SoundHandle;

    // Start playing a 3d audio source through the bus, delayed in relation to other sounds called
    // via this function.
    auto play3d_clocked(SoundTime    time,
                        AudioSource& sound,
                        Vector3      pos,
                        Vector3      vel    = {},
                        float        volume = 1.0f) -> SoundHandle;

    // Set number of channels for the bus (default 2)
    void set_channels(size_t channels);

    // Enable or disable visualization data gathering
    void set_visualization_enable(bool enable);

    // Move a live sound to this bus
    void annex_sound(SoundHandle voice_handle);

    // Calculate and get 256 floats of FFT data for visualization. Visualization has to be enabled
    // before use.
    auto calc_fft() -> float*;

    // Get 256 floats of wave data for visualization. Visualization has to be enabled before use.
    auto wave() -> float*;

    // Get approximate volume for output channel for visualization. Visualization has to be enabled
    // before use.
    auto approximate_volume(size_t aChannel) -> float;

    // Get number of immediate child voices to this bus
    auto active_voice_count() -> size_t;

    // Get current the resampler for this bus
    auto resampler() const -> Resampler;

    // Set the resampler for this bus
    void set_resampler(Resampler aResampler);

  private:
    // Internal: find the bus' channel
    void find_bus_handle();

    std::shared_ptr<BusInstance> m_instance;
    size_t                       m_channel_handle = 0;
    Resampler                    m_resampler      = default_resampler;

    // FFT output data
    std::array<float, 256> m_fft_data{};

    // Snapshot of wave data for visualization
    std::array<float, 256> m_wave_data{};
};
}; // namespace cer
