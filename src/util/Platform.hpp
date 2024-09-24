// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

// Internal header that can be used to query information about the current target platform
// at compile time.
// Usage:
// #if CERLIB_PLATFORM_LINUX
// ...
// #endif

#pragma once

// clang-format off

#if defined(__ANDROID__)
#  define CERLIB_PLATFORM_ANDROID 1
#elif defined(__linux__)
#  define CERLIB_PLATFORM_LINUX 1
#endif

#if defined(_WIN32) || defined(_WIN64)
#  define CERLIB_PLATFORM_WINDOWS 1
#endif

#if defined(__APPLE__)
#  if defined(IPHONE)
#    define CERLIB_PLATFORM_IOS 1
#  elif defined(__MACH__)
#    define CERLIB_PLATFORM_MACOS 1
#  endif
#endif

#if defined(__EMSCRIPTEN__)
#  define CERLIB_PLATFORM_WEB 1
#endif

#if defined(__clang__) || defined(__llvm__)
#  define CERLIB_COMPILER_CLANG 1
#endif

#if defined(__GNUC__)
#  define CERLIB_COMPILER_GCC 1
#endif

#if defined(_MSC_VER)
#  define CERLIB_COMPILER_MSVC 1
#endif

// clang-format on
