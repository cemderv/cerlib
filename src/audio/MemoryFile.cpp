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
#include <cstdio>
#include <cstring>

namespace cer
{
uint8_t MemoryFile::read8()
{
    uint8_t d = 0;
    read(&d, sizeof(d));
    return d;
}

uint16_t MemoryFile::read16()
{
    uint16_t d = 0;
    read(reinterpret_cast<unsigned char*>(&d), sizeof(d));
    return d;
}

uint32_t MemoryFile::read32()
{
    uint32_t d = 0;
    read(reinterpret_cast<unsigned char*>(&d), sizeof(d));
    return d;
}

size_t MemoryFile::read(unsigned char* aDst, size_t aBytes)
{
    if (mOffset + aBytes >= mData.size())
    {
        aBytes = mData.size() - mOffset;
    }

    std::memcpy(aDst, mData.data() + mOffset, aBytes);
    mOffset += aBytes;

    return aBytes;
}

void MemoryFile::seek(int aOffset)
{
    mOffset = aOffset >= 0 ? aOffset : mData.size() + aOffset;

    if (mOffset > mData.size() - 1)
    {
        mOffset = mData.size() - 1;
    }
}

size_t MemoryFile::pos() const
{
    return mOffset;
}

MemoryFile::MemoryFile(std::span<const std::byte> data)
    : mData(data)
{
}

bool MemoryFile::eof() const
{
    return mOffset >= mData.size();
}
} // namespace cer
