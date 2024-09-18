// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "shadercompiler/Writer.hpp"

#include <cassert>
#include <gsl/narrow>

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
        m_buffer.append(2 * m_indentation, ' ');
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

std::string_view Writer::buffer() const
{
    return m_buffer;
}

std::string Writer::take_buffer()
{
    return std::move(m_buffer);
}

size_t Writer::buffer_length() const
{
    return m_buffer.size();
}

int Writer::current_column() const
{
    // TODO: optimize this, this is ugly
    int column = 0;

    for (int i = gsl::narrow_cast<int>(m_buffer.size()) - 1; i >= 0; --i)
    {
        if (m_buffer[i] == '\n')
        {
            break;
        }

        ++column;
    }

    return column;
}

Writer& Writer::operator<<(std::string_view str)
{
    append(str);
    return *this;
}

Writer& Writer::operator<<(const std::string& str)
{
    append(str);
    return *this;
}

Writer& Writer::operator<<(const char* str)
{
    append(str);
    return *this;
}

Writer& Writer::operator<<(char ch)
{
    append(std::string_view{&ch, 1});
    return *this;
}

Writer& Writer::operator<<(int value)
{
    append(std::to_string(value));
    return *this;
}

Writer& Writer::operator<<(unsigned int value)
{
    append(std::to_string(value));
    return *this;
}

Writer& Writer::operator<<(bool value)
{
    append(value ? "true" : "false");
    return *this;
}

Writer& Writer::operator<<(WriterNewlineTag)
{
    append("\n");
    return *this;
}

Writer& Writer::operator<<(WriteNewlineLazyTag)
{
    if (m_buffer.empty() || m_buffer.back() != '\n')
    {
        append("\n");
    }

    return *this;
}
} // namespace cer::shadercompiler
