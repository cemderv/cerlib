// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <array>
#include <cstdint>
#include <cstddef>
#include <cassert>
#include <span>

namespace cer
{
template <typename T, size_t Size>
class SmallDataArray final
{
  public:
    SmallDataArray() = default;

    explicit SmallDataArray(std::span<const T> data)
    {
        const size_t count = data.size() > m_data.size() ? m_data.size() : data.size();

        for (size_t i = 0; i < count; ++i)
            m_data[i] = data[i];

        m_size = count;
    }

    explicit SmallDataArray(uint32_t size)
        : m_size(size)
    {
        assert(size <= m_data.size());
    }

    uint32_t size() const
    {
        return m_size;
    }

    T* begin()
    {
        return m_data.data();
    }

    const T* begin() const
    {
        return m_data.data();
    }

    const T* cbegin() const
    {
        return m_data.cbegin();
    }

    T* end()
    {
        return m_data.data() + m_size;
    }

    const T* end() const
    {
        return m_data.data() + m_size;
    }

    const T* cend() const
    {
        return m_data.data() + m_size;
    }

    T operator[](uint32_t index) const
    {
        assert(index < m_size);
        return m_data.at(index);
    }

  private:
    std::array<T, Size> m_data{};
    uint32_t            m_size{};
};
} // namespace cer
