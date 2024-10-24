// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <cerlib/String.hpp>
#include <cstddef>
#include <cstdint>

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

    auto operator<<(std::string_view str) -> Writer&;

    auto operator<<(const String& str) -> Writer&;

    auto operator<<(const char* str) -> Writer&;

    auto operator<<(char ch) -> Writer&;

    auto operator<<(int value) -> Writer&;

    auto operator<<(unsigned int value) -> Writer&;

    auto operator<<(bool value) -> Writer&;

    auto operator<<(WriterNewlineTag) -> Writer&;

    auto operator<<(WriteNewlineLazyTag) -> Writer&;

    auto operator<<(float) -> Writer& = delete;

    auto operator<<(double) -> Writer& = delete;

    void pad(uint32_t count);

    auto buffer() const -> std::string_view;

    auto take_buffer() -> String;

    auto buffer_length() const -> size_t;

    auto current_column() const -> int;

  private:
    String m_buffer;
    int    m_indentation{};
};
} // namespace cer::shadercompiler
