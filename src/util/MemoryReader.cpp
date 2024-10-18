/*
SoLoud audio engine
Copyright (c) 2013-2015 Jari Komppa

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

#include "util/MemoryReader.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>

namespace cer
{
auto MemoryReader::read_s8() -> int8_t
{
    int8_t d = 0;
    read(reinterpret_cast<unsigned char*>(&d), sizeof(d));
    return d;
}

auto MemoryReader::read_s16() -> int16_t
{
    int16_t d = 0;
    read(reinterpret_cast<unsigned char*>(&d), sizeof(d));
    return d;
}

auto MemoryReader::read_s32() -> int32_t
{
    int32_t d = 0;
    read(reinterpret_cast<unsigned char*>(&d), sizeof(d));
    return d;
}

auto MemoryReader::read_u8() -> uint8_t
{
    uint8_t d = 0;
    read(&d, sizeof(d));
    return d;
}

auto MemoryReader::read_u16() -> uint16_t
{
    uint16_t d = 0;
    read(reinterpret_cast<unsigned char*>(&d), sizeof(d));
    return d;
}

auto MemoryReader::read_u32() -> uint32_t
{
    uint32_t d = 0;
    read(reinterpret_cast<unsigned char*>(&d), sizeof(d));
    return d;
}

auto MemoryReader::read_f32() -> float
{
    float d = 0.0f;
    read(reinterpret_cast<unsigned char*>(&d), sizeof(d));
    return d;
}

auto MemoryReader::read(unsigned char* dst, size_t bytes) -> size_t
{
    if (m_offset + bytes >= m_data.size())
    {
        bytes = m_data.size() - m_offset;
    }

    std::memcpy(dst, m_data.data() + m_offset, bytes);
    m_offset += bytes;

    return bytes;
}

void MemoryReader::seek(int offset)
{
    m_offset = offset >= 0 ? offset : m_data.size() + offset;
    m_offset = std::min(m_offset, m_data.size() - 1);
}

auto MemoryReader::pos() const -> size_t
{
    return m_offset;
}

auto MemoryReader::data() const -> const std::byte*
{
    return m_data.data();
}

auto MemoryReader::data_uc() const -> const unsigned char*
{
    return reinterpret_cast<const unsigned char*>(data());
}

auto MemoryReader::size() const -> size_t
{
    return m_data.size();
}

MemoryReader::MemoryReader(std::span<const std::byte> data)
    : m_data(data)
{
}
} // namespace cer
