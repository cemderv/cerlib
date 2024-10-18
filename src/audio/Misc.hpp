/*
SoLoud audio engine
Copyright (c) 2020 Jari Komppa

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

namespace cer
{
enum class Waveform;

// Class that handles aligned allocations to support vectorized operations
class AlignedFloatBuffer
{
  public:
    AlignedFloatBuffer() = default;

    // Allocate and align buffer
    explicit AlignedFloatBuffer(size_t floats);

    AlignedFloatBuffer(const AlignedFloatBuffer&) = delete;

    AlignedFloatBuffer& operator=(const AlignedFloatBuffer&) = delete;

    AlignedFloatBuffer(AlignedFloatBuffer&&) noexcept = default;

    AlignedFloatBuffer& operator=(AlignedFloatBuffer&&) noexcept = default;

    // Clear data to zero.
    void clear();

    auto data() -> float*;

    auto data() const -> const float*;

    auto operator[](size_t index) -> float&;

    auto operator[](size_t index) const -> const float&;

  private:
    float*                           m_aligned_ptr = nullptr; // aligned pointer
    std::unique_ptr<unsigned char[]> m_data;
    size_t                           m_count = 0; // size of buffer (w/out padding)
};

// Lightweight class that handles small aligned buffer to support vectorized operations
class TinyAlignedFloatBuffer
{
  public:
    TinyAlignedFloatBuffer();

    auto data() -> float*;

    auto data() const -> const float*;

    auto operator[](size_t index) -> float&;

    auto operator[](size_t index) const -> const float&;

  private:
    std::array<unsigned char, (sizeof(float) * 16) + 16> m_data{};
    float*                                               m_aligned_ptr = nullptr;
};

// Generate a waveform.
auto generate_waveform(Waveform waveform, float p) -> float;

// WELL512 random
class Prg
{
  public:
    Prg();

    auto rand() -> size_t;

    auto rand_float() -> float;

    void srand(int seed);

  private:
    std::array<size_t, 16> m_state{};
    size_t                 m_index = 0;
};
}; // namespace cer
