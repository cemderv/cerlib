// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

// clang-format off
#if defined(CERLIB_SHARED_LIBRARY) && defined(CERLIB_ENABLE_TESTS)
#  if defined(_MSC_VER)
#    if defined(cerlib_EXPORTS)
#      define __declspec(dllexport)
#    else
#      define __declspec(dllimport)
#    endif
#  else
#    if defined(cerlib_EXPORTS)
#      define __attribute__((visibility("default")))
#    else
#      define CERLIB_API_INTERNAL
#    endif
#  endif
#else
#  define CERLIB_API_INTERNAL
#endif
// clang-format on
