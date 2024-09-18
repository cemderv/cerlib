/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2024 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

/**
 * Modified for use with cerlib.
 */

#pragma once

#ifdef _AIX
#define SDL_PLATFORM_AIX 1
#endif
#ifdef __HAIKU__
#define SDL_PLATFORM_HAIKU 1
#endif
#if defined(bsdi) || defined(__bsdi) || defined(__bsdi__)
#define SDL_PLATFORM_BSDI 1
#endif
#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__DragonFly__)
#define SDL_PLATFORM_FREEBSD 1
#endif
#if defined(hpux) || defined(__hpux) || defined(__hpux__)
#define SDL_PLATFORM_HPUX 1
#endif
#if defined(sgi) || defined(__sgi) || defined(__sgi__) || defined(_SGI_SOURCE)
#define SDL_PLATFORM_IRIX 1
#endif
#if (defined(linux) || defined(__linux) || defined(__linux__))
#define SDL_PLATFORM_LINUX 1
#endif
#if defined(ANDROID) || defined(__ANDROID__)
#undef SDL_PLATFORM_LINUX /* do we need to do this? */
#define SDL_PLATFORM_ANDROID 1
#endif
#ifdef __NGAGE__
#define SDL_PLATFORM_NGAGE 1
#endif

#if defined(__unix__) || defined(__unix) || defined(unix)
#define SDL_PLATFORM_UNIX 1
#endif

#ifdef __APPLE__
#define SDL_PLATFORM_APPLE 1

#include <AvailabilityMacros.h>
#include <TargetConditionals.h>

#ifndef TARGET_OS_MACCATALYST
#define TARGET_OS_MACCATALYST 0
#endif
#ifndef TARGET_OS_IOS
#define TARGET_OS_IOS 0
#endif
#ifndef TARGET_OS_IPHONE
#define TARGET_OS_IPHONE 0
#endif
#ifndef TARGET_OS_TV
#define TARGET_OS_TV 0
#endif
#ifndef TARGET_OS_SIMULATOR
#define TARGET_OS_SIMULATOR 0
#endif
#ifndef TARGET_OS_VISION
#define TARGET_OS_VISION 0
#endif

#if TARGET_OS_TV
#define SDL_PLATFORM_TVOS 1
#endif
#if TARGET_OS_VISION
#define SDL_PLATFORM_VISIONOS 1
#endif
#if TARGET_OS_IPHONE
#define SDL_PLATFORM_IOS 1
#else
#define SDL_PLATFORM_MACOS 1
#if MAC_OS_X_VERSION_MIN_REQUIRED < 1070
#error SDL for macOS only supports deploying on 10.7 and above.
#endif
#endif
#endif

#ifdef __EMSCRIPTEN__
#define SDL_PLATFORM_EMSCRIPTEN 1
#endif
#ifdef __NetBSD__
#define SDL_PLATFORM_NETBSD 1
#endif
#ifdef __OpenBSD__
#define SDL_PLATFORM_OPENBSD 1
#endif
#if defined(__OS2__) || defined(__EMX__)
#define SDL_PLATFORM_OS2 1
#endif
#if defined(osf) || defined(__osf) || defined(__osf__) || defined(_OSF_SOURCE)
#define SDL_PLATFORM_OSF 1
#endif
#ifdef __QNXNTO__
#define SDL_PLATFORM_QNXNTO 1
#endif
#if defined(riscos) || defined(__riscos) || defined(__riscos__)
#define SDL_PLATFORM_RISCOS 1
#endif
#if defined(__sun) && defined(__SVR4)
#define SDL_PLATFORM_SOLARIS 1
#endif

#if defined(__CYGWIN__)
#define SDL_PLATFORM_CYGWIN 1
#endif

#if defined(_WIN32) || defined(SDL_PLATFORM_CYGWIN)
#define SDL_PLATFORM_WINDOWS 1

#if defined(_MSC_VER) && defined(__has_include)
#if __has_include(<winapifamily.h>)
#define HAVE_WINAPIFAMILY_H 1
#else
#define HAVE_WINAPIFAMILY_H 0
#endif

#elif defined(_MSC_VER) && (_MSC_VER >= 1700 && !_USING_V110_SDK71_)
#define HAVE_WINAPIFAMILY_H 1
#else
#define HAVE_WINAPIFAMILY_H 0
#endif

#if HAVE_WINAPIFAMILY_H
#include <winapifamily.h>
#define WINAPI_FAMILY_WINRT                                                                        \
    (!WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) &&                                         \
     WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP))
#else
#define WINAPI_FAMILY_WINRT 0
#endif

#if HAVE_WINAPIFAMILY_H && HAVE_WINAPIFAMILY_H
#define SDL_WINAPI_FAMILY_PHONE (WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP)
#else
#define SDL_WINAPI_FAMILY_PHONE 0
#endif

#if WINAPI_FAMILY_WINRT
#define SDL_PLATFORM_WINRT 1
#elif defined(_GAMING_DESKTOP)
#define SDL_PLATFORM_WINGDK 1
#elif defined(_GAMING_XBOX_XBOXONE)
#define SDL_PLATFORM_XBOXONE 1
#elif defined(_GAMING_XBOX_SCARLETT)
#define SDL_PLATFORM_XBOXSERIES 1
#else
#define SDL_PLATFORM_WIN32 1
#endif
#endif

#if defined(SDL_PLATFORM_WINGDK) || defined(SDL_PLATFORM_XBOXONE) ||                               \
    defined(SDL_PLATFORM_XBOXSERIES)
#define SDL_PLATFORM_GDK 1
#endif
#if defined(__PSP__) || defined(__psp__)
#define SDL_PLATFORM_PSP 1
#endif
#if defined(__PS2__) || defined(PS2)
#define SDL_PLATFORM_PS2 1
#endif

#if defined(__vita__) || defined(__psp2__)
#define SDL_PLATFORM_VITA 1
#endif

#ifdef __3DS__
#undef __3DS__
#define SDL_PLATFORM_3DS 1
#endif

#ifdef SDL_PLATFORM_WIN32
#define SDL_MAIN_AVAILABLE

#elif defined(SDL_PLATFORM_WINRT)
#define SDL_MAIN_NEEDED

#elif defined(SDL_PLATFORM_GDK)
#define SDL_MAIN_NEEDED
#elif defined(SDL_PLATFORM_IOS)
#define SDL_MAIN_NEEDED
#elif defined(SDL_PLATFORM_ANDROID)
#define SDL_MAIN_NEEDED

/* We need to export SDL_main so it can be launched from Java */
#define SDLMAIN_DECLSPEC DECLSPEC

#elif defined(SDL_PLATFORM_PSP)
#define SDL_MAIN_AVAILABLE

#elif defined(SDL_PLATFORM_PS2)
#define SDL_MAIN_AVAILABLE

#define SDL_PS2_SKIP_IOP_RESET()                                                                   \
    void reset_IOP();                                                                              \
    void reset_IOP()                                                                               \
    {                                                                                              \
    }

#elif defined(SDL_PLATFORM_3DS)
#define SDL_MAIN_AVAILABLE

#elif defined(SDL_PLATFORM_NGAGE)
#define SDL_MAIN_AVAILABLE
#endif

#ifndef SDLMAIN_DECLSPEC
#define SDLMAIN_DECLSPEC
#endif

#if defined(SDL_MAIN_NEEDED) || defined(SDL_MAIN_AVAILABLE)
#define main SDL_main
#endif

#ifndef SDL_DEPRECATED
#if defined(__GNUC__) && (__GNUC__ >= 4)
#define SDL_DEPRECATED __attribute__((deprecated))
#elif defined(_MSC_VER)
#define SDL_DEPRECATED __declspec(deprecated)
#else
#define SDL_DEPRECATED
#endif
#endif

#ifndef SDL_UNUSED
#ifdef __GNUC__
#define SDL_UNUSED __attribute__((unused))
#else
#define SDL_UNUSED
#endif
#endif

#ifndef DECLSPEC
#if defined(SDL_PLATFORM_WIN32) || defined(SDL_PLATFORM_WINRT) || defined(SDL_PLATFORM_CYGWIN) ||  \
    defined(SDL_PLATFORM_GDK)
#ifdef DLL_EXPORT
#define DECLSPEC __declspec(dllexport)
#else
#define DECLSPEC
#endif
#else
#if defined(__GNUC__) && __GNUC__ >= 4
#define DECLSPEC __attribute__((visibility("default")))
#else
#define DECLSPEC
#endif
#endif
#endif

/* By default SDL uses the C calling convention */
#ifndef SDLCALL
#if (defined(SDL_PLATFORM_WIN32) || defined(SDL_PLATFORM_WINRT) || defined(SDL_PLATFORM_GDK)) &&   \
    !defined(__GNUC__)
#define SDLCALL __cdecl
#else
#define SDLCALL
#endif
#endif /* SDLCALL */

/* Force structure packing at 4 byte alignment.
   This is necessary if the header is included in code which has structure
   packing set to an alternate value, say for loading structures from disk.
   The packing is reset to the previous value in SDL_close_code.h
 */
#if defined(_MSC_VER) || defined(__MWERKS__) || defined(__BORLANDC__)
#ifdef _MSC_VER
#pragma warning(disable : 4103)
#endif
#ifdef __clang__
#pragma clang diagnostic ignored "-Wpragma-pack"
#endif
#ifdef __BORLANDC__
#pragma nopackwarning
#endif
#ifdef _WIN64
/* Use 8-byte alignment on 64-bit architectures, so pointers are aligned */
#pragma pack(push, 8)
#else
#pragma pack(push, 4)
#endif
#endif /* Compiler needs structure packing set */

#ifndef SDL_INLINE
#ifdef __GNUC__
#define SDL_INLINE __inline__
#elif defined(_MSC_VER) || defined(__BORLANDC__) || defined(__DMC__) || defined(__SC__) ||         \
    defined(__WATCOMC__) || defined(__LCC__) || defined(__DECC) || defined(__CC_ARM)
#define SDL_INLINE __inline
#ifndef __inline__
#define __inline__ __inline
#endif
#else
#define SDL_INLINE inline
#ifndef __inline__
#define __inline__ inline
#endif
#endif
#endif /* SDL_INLINE not defined */

#ifndef SDL_FORCE_INLINE
#ifdef _MSC_VER
#define SDL_FORCE_INLINE __forceinline
#elif ((defined(__GNUC__) && (__GNUC__ >= 4)) || defined(__clang__))
#define SDL_FORCE_INLINE __attribute__((always_inline)) static __inline__
#else
#define SDL_FORCE_INLINE static SDL_INLINE
#endif
#endif /* SDL_FORCE_INLINE not defined */

#ifndef SDL_NORETURN
#ifdef __GNUC__
#define SDL_NORETURN __attribute__((noreturn))
#elif defined(_MSC_VER)
#define SDL_NORETURN __declspec(noreturn)
#else
#define SDL_NORETURN
#endif
#endif /* SDL_NORETURN not defined */

/* Apparently this is needed by several Windows compilers */
#ifndef __MACH__
#ifndef NULL
#ifdef __cplusplus
#define NULL 0
#else
#define NULL ((void*)0)
#endif
#endif
#endif

extern "C"
{
    extern SDLMAIN_DECLSPEC int SDLCALL SDL_main(int argc, char* argv[]);
}

#if defined(_MSC_VER) || defined(__MWERKS__) || defined(__BORLANDC__)
#ifdef __BORLANDC__
#pragma nopackwarning
#endif
#pragma pack(pop)
#endif

#include <cerlib/details/MainImpl.hpp>
