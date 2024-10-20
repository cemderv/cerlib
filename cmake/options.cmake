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
  CERLIB_ATOMIC_REFCOUNTING
  "Enable atomic reference counting for cerlib objects"
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
  CERLIB_ENABLE_IMGUI
  "Enable and provide ImGui as part of the cerlib API"
  ON
)

option(
  CERLIB_BUILD_PLATFORMER_DEMO
  "Build the platformer demo game"
  ${is_root_directory}
)

option(
  CERLIB_ENABLE_VERBOSE_LOGGING
  "Enable verbose logging during debug mode"
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

option(
  CERLIB_ENABLE_RENDERING_TESTS
  "Enable rendering unit tests. Requires CERLIB_ENABLE_TESTS=ON"
  ON
)

option(
  CERLIB_FORCE_SDL_BUILD
  "Build SDL along with cerlib, even if prebuilt SDL libraries are available"
  OFF
)
