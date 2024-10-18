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

#pragma once

#include <span>

namespace cer
{
class MemoryFile final
{
  public:
    MemoryFile() = default;

    explicit MemoryFile(std::span<const std::byte> data);

    uint8_t  read8();
    uint16_t read16();
    uint32_t read32();
    bool     eof() const;
    size_t   read(unsigned char* aDst, size_t aBytes);
    void     seek(int aOffset);
    size_t   pos() const;

    const std::byte* data() const
    {
        return mData.data();
    }

    const unsigned char* data_uc() const
    {
        return reinterpret_cast<const unsigned char*>(data());
    }

    size_t size() const
    {
        return mData.size();
    }

  private:
    std::span<const std::byte> mData;
    size_t                     mOffset = 0;
};
}; // namespace cer
