// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
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
        {
            m_data[i] = data[i];
        }

        m_size = count;
    }

    explicit SmallDataArray(uint32_t size)
        : m_size(size)
    {
        assert(size <= m_data.size());
    }

    auto size() const -> uint32_t
    {
        return m_size;
    }

    auto begin() -> T*
    {
        return m_data.data();
    }

    auto begin() const -> const T*
    {
        return m_data.data();
    }

    auto cbegin() const -> const T*
    {
        return m_data.cbegin();
    }

    auto end() -> T*
    {
        return m_data.data() + m_size;
    }

    auto end() const -> const T*
    {
        return m_data.data() + m_size;
    }

    auto cend() const -> const T*
    {
        return m_data.data() + m_size;
    }

    auto operator[](uint32_t index) -> T&
    {
        assert(index < m_size);
        return m_data.at(index);
    }

    auto operator[](uint32_t index) const -> const T&
    {
        assert(index < m_size);
        return m_data.at(index);
    }

  private:
    std::array<T, Size> m_data{};
    uint32_t            m_size{};
};
} // namespace cer
