if (NOT stb_ADDED)
  message(FATAL_ERROR "stb was not fetched!")
endif ()

cerlib_Log("STB libraries directory: ${stb_SOURCE_DIR}")

project(stblib LANGUAGES C)
add_library(stblib STATIC)
target_sources(stblib PRIVATE stb.c)
target_include_directories(stblib SYSTEM PUBLIC ${stb_SOURCE_DIR})

if (ANDROID)
  if (ANDROID_ABI STREQUAL "arm64-v8a")
    cerlib_log("Detected ARM64 for stb; enabling STBIR_NEON")
    target_compile_definitions(stblib PUBLIC "-DSTBIR_NEON")
  elseif (ANDROID_ABI STREQUAL "x86_64")
    cerlib_log("Detected x64 for stb; enaling STBIR_SSE2")
    target_compile_definitions(stblib PUBLIC "-DSTBIR_SSE2")
  else ()
    cerlib_log("Disabling SIMD for stb_image_resize2")
    target_compile_definitions(stblib PUBLIC "-DSTBIR_NO_SIMD")
  endif ()
endif ()

set_target_properties(stblib PROPERTIES FOLDER "Dependencies")

