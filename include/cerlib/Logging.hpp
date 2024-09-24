// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include <cerlib/Export.hpp>
#include <cerlib/Formatters.hpp>

namespace cer
{
namespace details
{
enum class LogMessageType
{
    Info    = 1,
    Warning = 2,
    Error   = 3,
};

CERLIB_API void log_internal(const std::string& message, LogMessageType type);
} // namespace details

/**
 * Logs information to the system's output.
 *
 * Example:
 * @code{.cpp}
 * cer::log_info("Player '{}' is at {}", player.name(), cer::Vector2{10, 20});
 * @endcode
 *
 * @ingroup Misc
 */
template <typename... Args>
void log_info(cer_fmt::format_string<Args...> fmt, Args&&... args)
{
    details::log_internal(cer_fmt::format(fmt, std::forward<Args>(args)...),
                          details::LogMessageType::Info);
}

/**
 * Logs a warning to the system's output.
 *
 * See the documentation of `cer::log_info()` for an example.
 *
 * @ingroup Misc
 */
template <typename... Args>
void log_warning(cer_fmt::format_string<Args...> fmt, Args&&... args)
{
    details::log_internal(cer_fmt::format(fmt, std::forward<Args>(args)...),
                          details::LogMessageType::Warning);
}

/**
 * Logs an error to the system's output.
 *
 * See the documentation of `cer::log_info()` for an example.
 *
 * @ingroup Misc
 */
template <typename... Args>
void log_error(cer_fmt::format_string<Args...> fmt, Args&&... args)
{
    details::log_internal(cer_fmt::format(fmt, std::forward<Args>(args)...),
                          details::LogMessageType::Error);
}

/**
 * Logs information to the system's output **in debug mode only**.
 *
 * In release mode, this will result in a no-op.
 *
 * See the documentation of `cer::log_info()` for an example.
 *
 * @ingroup Misc
 */
template <typename... Args>
void log_debug([[maybe_unused]] cer_fmt::format_string<Args...> fmt, [[maybe_unused]] Args&&... args)
{
#ifndef NDEBUG
    details::log_internal(cer_fmt::format(fmt, std::forward<Args>(args)...),
                          details::LogMessageType::Info);
#endif
}

/**
 * Logs information to the system's output **in debug mode only**,
 * and only if the CERLIB_ENABLE_VERBOSE_LOGGING option was enabled.
 *
 * In release mode, this will result in a no-op.
 *
 * See the documentation of `cer::log_info()` for an example.
 *
 * @ingroup Misc
 */
template <typename... Args>
void log_verbose([[maybe_unused]] cer_fmt::format_string<Args...> fmt, [[maybe_unused]] Args&&... args)
{
#if defined(CERLIB_ENABLE_VERBOSE_LOGGING) && !defined(NDEBUG)
    details::log_internal(cer_fmt::format(fmt, std::forward<Args>(args)...),
                          details::LogMessageType::Info);
#endif
}
} // namespace cer
