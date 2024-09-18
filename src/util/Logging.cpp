// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "cerlib/Logging.hpp"
#include <iostream>

namespace cer::details
{
auto log_internal(const std::string& message, LogMessageType type) -> void
{
    switch (type)
    {
        case LogMessageType::Info: std::cout << message << std::endl; break;
        case LogMessageType::Warning: std::cout << "WARNING: " << message << std::endl; break;
        case LogMessageType::Error: std::cout << "ERROR: " << message << std::endl; break;
    }
}
} // namespace cer::details
