if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(is_root_directory TRUE)
else ()
  set(is_root_directory FALSE)
endif ()

option(
  CERLIB_ENABLE_ADDRESS_SANITIZER
  "Enable clang address sanitizer"
  OFF
)

option(
  CERLIB_ENABLE_CLANG_TIDY
  "Enable clang-tidy analysis throughout the codebase"
  OFF
)

option(
  CERLIB_ENABLE_CLANG_TIME_TRACE
  "Enable clang compile-time profiling."
  OFF
)

option(
  CERLIB_ENABLE_DIAGNOSTIC_LOGGING
  "Enable diagnostic logging. Should only be used for debugging purposes."
  OFF
)

option(
  CERLIB_ATOMIC_REFCOUNTING
  "Enable atomic reference counting for cerlib objects"
  OFF
)

option(
  CERLIB_SHARED_LIBRARY
  "Build cerlib as a shared library"
  OFF
)

option(
  CERLIB_ENABLE_LTO
  "Enable link-time-optimizations"
  OFF
)

option(
  CERLIB_BUILD_TESTBED
  "Build the cerlib testbed application"
  ${is_root_directory}
)

option(
  CERLIB_BUILD_PLATFORMER_DEMO
  "Build the platformer demo game"
  ${is_root_directory}
)

option(
  CERLIB_ENABLE_CLANG_TIDY
  "Enable clang-tidy checks across the entire codebase"
  OFF
)

if (ANDROID)
  if (CERLIB_BUILD_TESTBED)
    cerlib_log("Disabling Testbed implicitly due to Android")
    set(CERLIB_BUILD_TESTBED OFF)
  endif ()

  if (CERLIB_BUILD_PLATFORMER_DEMO)
    cerlib_log("Disabling Platformer implicitly due to Android")
    set(CERLIB_BUILD_PLATFORMER_DEMO OFF)
  endif ()
endif ()

option(
  CERLIB_ENABLE_TESTS
  "Enable unit testing"
  OFF
)

