// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "cerlib/Version.hpp"

#include "util/Platform.hpp"

cer::TargetPlatform cer::target_platform()
{
#if CERLIB_PLATFORM_WINDOWS
    return TargetPlatform::Windows;
#elif CERLIB_PLATFORM_MACOS
    return TargetPlatform::macOS;
#elif CERLIB_PLATFORM_LINUX
    return TargetPlatform::Linux;
#elif CERLIB_PLATFORM_ANDROID
    return TargetPlatform::Android;
#elif CERLIB_PLATFORM_WEB
    return TargetPlatform::Web;
#else
#error "Unhandled target platform"
#endif
}

bool cer::is_desktop_platform()
{
    const TargetPlatform platform = target_platform();

    return platform == TargetPlatform::Windows || platform == TargetPlatform::macOS ||
           platform == TargetPlatform::Linux;
}

bool cer::is_mobile_platform()
{
    const TargetPlatform platform = target_platform();

    return platform == TargetPlatform::Android;
}
