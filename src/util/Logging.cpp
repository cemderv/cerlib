// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "cerlib/Logging.hpp"
#include "util/Platform.hpp"

#if CERLIB_PLATFORM_WINDOWS
#include <Windows.h>
#elif CERLIB_PLATFORM_ANDROID
#include <android/log.h>
#else
#include <iostream>
#endif

namespace cer::details
{
void log_internal(const String& message, LogMessageType type)
{
#if CERLIB_PLATFORM_WINDOWS
    auto full_msg = String{};
    full_msg.reserve(message.size() + 1);

    switch (type)
    {
        case LogMessageType::Warning: full_msg = "WARNING: "; break;
        case LogMessageType::Error: full_msg = "ERROR: "; break;
    }

    full_msg += message;
    full_msg += '\n';

    OutputDebugString(full_msg.c_str());
#elif CERLIB_PLATFORM_ANDROID
    const auto log_priority = [type] {
        switch (type)
        {
            case LogMessageType::Info: return ANDROID_LOG_INFO;
            case LogMessageType::Warning: return ANDROID_LOG_WARN;
            case LogMessageType::Error: return ANDROID_LOG_ERROR;
        }

        return ANDROID_LOG_INFO;
    }();

    __android_log_print(log_priority, "cerlib", "%s", message.c_str());
#else
    switch (type)
    {
        case LogMessageType::Info: std::cout << message << '\n'; break;
        case LogMessageType::Warning: std::cout << "WARNING: " << message << '\n'; break;
        case LogMessageType::Error: std::cout << "ERROR: " << message << '\n'; break;
    }
#endif
}
} // namespace cer::details
