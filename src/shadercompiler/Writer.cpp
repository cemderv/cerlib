// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "shadercompiler/Writer.hpp"

#include "util/narrow_cast.hpp"
#include <cassert>

namespace cer::shadercompiler
{
Writer::Writer()
{
    m_buffer.reserve(64);
}

void Writer::append_line(std::string_view str)
{
    append(str);
    append("\n");
}

void Writer::append(std::string_view str)
{
    if (!m_buffer.empty() && m_buffer.back() == '\n')
    {
        m_buffer.append(size_t(2) * size_t(m_indentation), ' ');
    }

    m_buffer += str;
}

void Writer::indent()
{
    ++m_indentation;
}

void Writer::unindent()
{
    assert(m_indentation > 0);
    --m_indentation;
}

void Writer::open_brace()
{
    append_line("{");
    indent();
}

void Writer::close_brace(bool semicolon)
{
    unindent();
    append(semicolon ? "};" : "}");
}

void Writer::clear()
{
    m_buffer.clear();
}

void Writer::pad(uint32_t count)
{
    m_buffer.append(count, ' ');
}

auto Writer::buffer() const -> std::string_view
{
    return m_buffer;
}

auto Writer::take_buffer() -> std::string
{
    return std::move(m_buffer);
}

auto Writer::buffer_length() const -> size_t
{
    return m_buffer.size();
}

auto Writer::current_column() const -> int
{
    // TODO: optimize this, this is ugly
    int column = 0;

    const auto end = narrow_cast<int>(m_buffer.size()) - 1;

    for (int i = end; i >= 0; --i)
    {
        if (m_buffer[i] == '\n')
        {
            break;
        }

        ++column;
    }

    return column;
}

auto Writer::operator<<(std::string_view str) -> Writer&
{
    append(str);
    return *this;
}

auto Writer::operator<<(const std::string& str) -> Writer&
{
    append(str);
    return *this;
}

auto Writer::operator<<(const char* str) -> Writer&
{
    append(str);
    return *this;
}

auto Writer::operator<<(char ch) -> Writer&
{
    append(std::string_view{&ch, 1});
    return *this;
}

auto Writer::operator<<(int value) -> Writer&
{
    append(std::to_string(value));
    return *this;
}

auto Writer::operator<<(unsigned int value) -> Writer&
{
    append(std::to_string(value));
    return *this;
}

auto Writer::operator<<(bool value) -> Writer&
{
    append(value ? "true" : "false");
    return *this;
}

auto Writer::operator<<(WriterNewlineTag) -> Writer&
{
    append("\n");
    return *this;
}

auto Writer::operator<<(WriteNewlineLazyTag) -> Writer&
{
    if (m_buffer.empty() || m_buffer.back() != '\n')
    {
        append("\n");
    }

    return *this;
}
} // namespace cer::shadercompiler
