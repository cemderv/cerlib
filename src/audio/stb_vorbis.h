#pragma once

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4244)
#pragma warning(disable : 4245)
#pragma warning(disable : 4457)
#endif

// #define STB_VORBIS_HEADER_ONLY

#include "stb_vorbis.c"