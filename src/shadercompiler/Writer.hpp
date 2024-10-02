// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "util/InternalExport.hpp"
#include <cstddef>
#include <cstdint>
#include <string>

namespace cer::shadercompiler
{
enum WriterNewlineTag
{
    WNewline,
};

enum WriteNewlineLazyTag
{
    WNewlineLazy,
};

class Writer final
{
  public:
    Writer();

    void append_line(std::string_view str);

    void append(std::string_view str);

    void indent();

    void unindent();

    void open_brace();

    void close_brace(bool semicolon = false);

    void clear();

    Writer& operator<<(std::string_view str);

    Writer& operator<<(const std::string& str);

    Writer& operator<<(const char* str);

    Writer& operator<<(char ch);

    Writer& operator<<(int value);

    Writer& operator<<(unsigned int value);

    Writer& operator<<(bool value);

    Writer& operator<<(WriterNewlineTag);

    Writer& operator<<(WriteNewlineLazyTag);

    Writer& operator<<(float) = delete;

    Writer& operator<<(double) = delete;

    void pad(uint32_t count);

    std::string_view buffer() const;

    std::string take_buffer();

    size_t buffer_length() const;

    int current_column() const;

  private:
    std::string m_buffer;
    int         m_indentation{};
};
} // namespace cer::shadercompiler
