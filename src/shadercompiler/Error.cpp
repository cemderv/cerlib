// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "shadercompiler/Error.hpp"
#include "cerlib/Logging.hpp"
#include "shadercompiler/SourceLocation.hpp"
#include <cassert>

namespace cer::shadercompiler
{
static std::string build_full_message(const SourceLocation& location, std::string_view message)
{
    assert(!location.filename.empty());

    return location.line == 0 ? cer_fmt::format("{}: error: {}", location.filename, message)
           : location.column == 0
               ? cer_fmt::format("{}({}): error: {}", location.filename, location.line, message)
               : cer_fmt::format("{}({}, {}): error: {}",
                                 location.filename,
                                 location.line,
                                 location.column,
                                 message);
}

Error::Error() = default;

Error::Error(const SourceLocation& location, std::string_view message)
    : m_full_message(build_full_message(location, message))
{
    log_debug("{}", m_full_message);
}

const char* Error::what() const noexcept
{
    return m_full_message.c_str();
}

std::string_view Error::full_message() const
{
    return m_full_message;
}
} // namespace cer::shadercompiler
