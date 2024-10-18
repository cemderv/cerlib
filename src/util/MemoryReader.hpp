// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <span>

namespace cer
{
class MemoryReader final
{
  public:
    MemoryReader() = default;

    explicit MemoryReader(std::span<const std::byte> data);

    auto read_s8() -> int8_t;
    auto read_s16() -> int16_t;
    auto read_s32() -> int32_t;
    auto read_u8() -> uint8_t;
    auto read_u16() -> uint16_t;
    auto read_u32() -> uint32_t;
    auto read_f32() -> float;
    auto read(unsigned char* dst, size_t bytes) -> size_t;
    void seek(int offset);
    auto pos() const -> size_t;
    auto data() const -> const std::byte*;
    auto data_uc() const -> const unsigned char*;
    auto size() const -> size_t;

  private:
    std::span<const std::byte> m_data;
    size_t                     m_offset = 0;
};
}; // namespace cer
