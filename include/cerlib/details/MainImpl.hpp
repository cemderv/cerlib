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

#include <cerlib/RunGame.hpp>

#ifdef main
#undef main
#endif

#if defined(SDL_PLATFORM_WIN32) || defined(SDL_PLATFORM_GDK)

#ifndef WINAPI
#define WINAPI __stdcall
#endif

typedef struct HINSTANCE__* HINSTANCE;
typedef char*               LPSTR;
typedef wchar_t*            PWSTR;

#if defined(_MSC_VER) && !defined(SDL_PLATFORM_GDK)

#if defined(UNICODE) && UNICODE
int wmain(int argc, wchar_t* wargv[], wchar_t* wenvp)
{
    (void)argc;
    (void)wargv;
    (void)wenvp;
    return cer::details::run_game(0, nullptr, SDL_main, nullptr);
}
#else
int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;
    return cer::details::run_game(0, nullptr, SDL_main, nullptr);
}
#endif

#endif

extern "C"
{

#if defined(UNICODE) && UNICODE
    int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE hPrev, PWSTR szCmdLine, int sw)
#else
    int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR szCmdLine, int sw)
#endif
    {
        (void)hInst;
        (void)hPrev;
        (void)szCmdLine;
        (void)sw;
        return cer::details::run_game(0, nullptr, SDL_main, nullptr);
    }
}

#elif defined(SDL_PLATFORM_WINRT)
#include <wrl.h>

#ifndef SDL_WINRT_METADATA_FILE_AVAILABLE
#if !defined(__cplusplus_winrt)
#error The C++ file that includes <cerlib/Main.hpp> must be compiled as C++ code with /ZW, otherwise build errors due to missing .winmd files can occur.
#endif
#endif

#ifdef _MSC_VER
#pragma warning(disable : 4447)
#pragma comment(lib, "runtimeobject.lib")
#endif

extern "C"
{
    int CALLBACK WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
    {
        return cer::details::run_game(0, nullptr, SDL_main, nullptr);
    }
}

#elif defined(SDL_PLATFORM_NGAGE)
typedef signed int TInt;

TInt E32Main()
{
    return cer::details::run_game(0, nullptr, SDL_main, nullptr);
}

#else
int main(int argc, char* argv[])
{
    return cer::details::run_game(argc, argv, SDL_main, nullptr);
}

#endif

#define main SDL_main
