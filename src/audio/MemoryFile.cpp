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

#include "audio/MemoryFile.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>

namespace cer
{
auto MemoryFile::read_s8() -> int8_t
{
    int8_t d = 0;
    read(reinterpret_cast<unsigned char*>(&d), sizeof(d));
    return d;
}

auto MemoryFile::read_s16() -> int16_t
{
    int16_t d = 0;
    read(reinterpret_cast<unsigned char*>(&d), sizeof(d));
    return d;
}

auto MemoryFile::read_s32() -> int32_t
{
    int32_t d = 0;
    read(reinterpret_cast<unsigned char*>(&d), sizeof(d));
    return d;
}

auto MemoryFile::read_u8() -> uint8_t
{
    uint8_t d = 0;
    read(&d, sizeof(d));
    return d;
}

auto MemoryFile::read_u16() -> uint16_t
{
    uint16_t d = 0;
    read(reinterpret_cast<unsigned char*>(&d), sizeof(d));
    return d;
}

auto MemoryFile::read_u32() -> uint32_t
{
    uint32_t d = 0;
    read(reinterpret_cast<unsigned char*>(&d), sizeof(d));
    return d;
}

auto MemoryFile::read_f32() -> float
{
    float d = 0.0f;
    read(reinterpret_cast<unsigned char*>(&d), sizeof(d));
    return d;
}

auto MemoryFile::read(unsigned char* dst, size_t bytes) -> size_t
{
    if (mOffset + bytes >= mData.size())
    {
        bytes = mData.size() - mOffset;
    }

    std::memcpy(dst, mData.data() + mOffset, bytes);
    mOffset += bytes;

    return bytes;
}

void MemoryFile::seek(int offset)
{
    mOffset = offset >= 0 ? offset : mData.size() + offset;
    mOffset = std::min(mOffset, mData.size() - 1);
}

auto MemoryFile::pos() const -> size_t
{
    return mOffset;
}

auto MemoryFile::data() const -> const std::byte*
{
    return mData.data();
}

auto MemoryFile::data_uc() const -> const unsigned char*
{
    return reinterpret_cast<const unsigned char*>(data());
}

auto MemoryFile::size() const -> size_t
{
    return mData.size();
}

MemoryFile::MemoryFile(std::span<const std::byte> data)
    : mData(data)
{
}
} // namespace cer
