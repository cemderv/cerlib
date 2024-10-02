// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "cerlib/Formatters.hpp"
#include "shadercompiler/SourceLocation.hpp"
#include <exception>
#include <string>

namespace cer::shadercompiler
{
class SourceLocation;

class Error final : public std::exception
{
  public:
    Error();

    explicit Error(const SourceLocation& location, std::string_view message);

    template <typename... Args>
    explicit Error(const SourceLocation&           location,
                   cer_fmt::format_string<Args...> fmt,
                   Args&&... args)
        : Error(location, cer_fmt::format(fmt, std::forward<Args>(args)...))
    {
    }

    const char* what() const noexcept override;

    std::string_view full_message() const;

  private:
    std::string m_full_message;
};
} // namespace cer::shadercompiler
