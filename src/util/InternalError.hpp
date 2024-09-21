// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "cerlib/Formatters.hpp"
#include <exception>
#include <stdexcept>
#include <string>

namespace cer
{
class InternalError final : public std::exception
{
  public:
    explicit InternalError(std::string_view location, std::string_view message)
        : m_message(cer_fmt::format("{}: {}", location, message))
    {
    }

    const char* what() const noexcept override
    {
        return m_message.c_str();
    }

  private:
    std::string m_message;
};
} // namespace cer

#define CER_THROW_INTERNAL_ERROR(str, ...)                                                         \
    throw cer::InternalError                                                                       \
    {                                                                                              \
        __FUNCTION__, cer_fmt::format(str, __VA_ARGS__)                                                \
    }

#define CER_THROW_INTERNAL_ERROR_STR(message)                                                      \
    throw cer::InternalError                                                                       \
    {                                                                                              \
        __FUNCTION__, message                                                                      \
    }

#define CER_THROW_INVALID_ARG(str, ...)                                                            \
    throw std::invalid_argument                                                                    \
    {                                                                                              \
        cer_fmt::format(str, __VA_ARGS__)                                                              \
    }

#define CER_THROW_INVALID_ARG_STR(message)                                                         \
    throw std::invalid_argument                                                                    \
    {                                                                                              \
        message                                                                                    \
    }

#define CER_THROW_LOGIC_ERROR(str, ...)                                                            \
    throw std::logic_error                                                                         \
    {                                                                                              \
        cer_fmt::format(str, __VA_ARGS__)                                                              \
    }

#define CER_THROW_LOGIC_ERROR_STR(message)                                                         \
    throw std::logic_error                                                                         \
    {                                                                                              \
        message                                                                                    \
    }

#define CER_THROW_RUNTIME_ERROR(str, ...)                                                          \
    throw std::runtime_error                                                                       \
    {                                                                                              \
        cer_fmt::format(str, __VA_ARGS__)                                                              \
    }

#define CER_THROW_RUNTIME_ERROR_STR(message)                                                       \
    throw std::runtime_error                                                                       \
    {                                                                                              \
        message                                                                                    \
    }

#define CER_THROW_NOT_IMPLEMENTED(feature_name)                                                    \
    throw std::logic_error                                                                         \
    {                                                                                              \
        "The feature '" feature_name "' is not implemented yet."                                   \
    }
