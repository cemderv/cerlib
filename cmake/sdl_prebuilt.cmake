set(cerlib_sdl_prebuilt_url "https://github.com/cemderv/cerlib-sdl-prebuilt/releases/download/")

macro(cerlib_allow_sdl_prebuilt package_name)
  set(cerlib_can_use_prebuilt_sdl ON)
  set(cerlib_prebuilt_sdl_package ${package_name})
endmacro()

macro(cerlib_check_can_use_sdl_prebuilt)
  cerlib_log("Checking possibility of using prebuilt SDL")
  set(cerlib_can_use_prebuilt_sdl OFF)

  cerlib_log("Host name: ${CMAKE_HOST_SYSTEM}")
  cerlib_log("Host version: ${CMAKE_HOST_SYSTEM_VERSION}")
  cerlib_log("Compiler ID: ${CMAKE_CXX_COMPILER_ID}")
  cerlib_log("Compiler version: ${CMAKE_CXX_COMPILER_VERSION}")
  cerlib_log("System processor: ${CMAKE_SYSTEM_PROCESSOR}")

  string(REPLACE "." ";" compiler_version_list ${CMAKE_CXX_COMPILER_VERSION})
  list(GET compiler_version_list 0 compiler_major_version)

  if (WIN32 AND MSVC)
    if (${MSVC_VERSION} GREATER_EQUAL 1930 AND CMAKE_SYSTEM_PROCESSOR STREQUAL "AMD64")
      cerlib_allow_sdl_prebuilt("windows-2022-x64")
    endif ()
  elseif (APPLE AND NOT IOS)
    set(darwin_version ${CMAKE_HOST_SYSTEM_VERSION})

    if (darwin_version VERSION_GREATER_EQUAL 24.0) # macOS 15
      cerlib_allow_sdl_prebuilt("macos-15")
    elseif (darwin_version VERSION_GREATER_EQUAL 23.0) # macOS 14
      cerlib_allow_sdl_prebuilt("macos-14")
    elseif (darwin_version VERSION_GREATER_EQUAL 22.0) # macOS 13
      cerlib_allow_sdl_prebuilt("macos-13")
    endif ()
  elseif (LINUX AND CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
      if (compiler_major_version GREATER_EQUAL 16 AND compiler_major_version LESS_EQUAL 19)
        cerlib_allow_sdl_prebuilt("linux-clang-${compiler_major_version}-x64")
      endif ()
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
      if (compiler_major_version GREATER_EQUAL 13 AND compiler_major_version LESS_EQUAL 14)
        cerlib_allow_sdl_prebuilt("linux-gcc-${compiler_major_version}-x64")
      endif ()
    endif ()
  endif ()
endmacro()

function(cerlib_fetch_and_add_prebuilt_sdl fetch_url)
  CPMAddPackage(
    NAME cerlib_sdl_prebuilt
    URL ${fetch_url}
    DOWNLOAD_ONLY
    SYSTEM
  )

  set(sdl_dir ${${CPM_LAST_PACKAGE_NAME}_SOURCE_DIR})
  cerlib_log("Fetched prebuilt SDL to: ${sdl_dir}")

  add_library(SDL3-static STATIC IMPORTED)

  set_target_properties(SDL3-static PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${sdl_dir}/sdl-src/include"
  )

  if (WIN32)
    set_target_properties(SDL3-static PROPERTIES
      IMPORTED_LOCATION_DEBUG "${sdl_dir}/sdl-build/Debug/SDL3-static.lib"
      IMPORTED_LOCATION_RELEASE "${sdl_dir}/sdl-build/Release/SDL3-static.lib"
      IMPORTED_LOCATION_RELWITHDEBINFO "${sdl_dir}/sdl-build/Release/SDL3-static.lib"
      IMPORTED_LOCATION_MINSIZEREL "${sdl_dir}/sdl-build/Release/SDL3-static.lib"
    )

    target_link_libraries(SDL3-static INTERFACE
      kernel32
      user32
      gdi32
      winmm
      imm32
      ole32
      oleaut32
      version
      uuid
      advapi32
      setupapi
      shell32
      dinput8
    )
  elseif (APPLE)
    set_target_properties(SDL3-static PROPERTIES
      IMPORTED_LOCATION_DEBUG "${sdl_dir}/sdl-build/Debug/libSDL3.a"
      IMPORTED_LOCATION_RELEASE "${sdl_dir}/sdl-build/Release/libSDL3.a"
      IMPORTED_LOCATION_RELWITHDEBINFO "${sdl_dir}/sdl-build/Release/libSDL3.a"
      IMPORTED_LOCATION_MINSIZEREL "${sdl_dir}/sdl-build/Release/libSDL3.a"
    )

    target_link_libraries(SDL3-static INTERFACE
      "-framework Cocoa"
      "-framework CoreMedia"
      "-framework CoreVideo"
      "-framework CoreAudio"
      "-framework IOKit"
      "-framework Carbon"
      "-framework ForceFeedback"
      "-framework AudioToolbox"
      "-framework AVFoundation"
      "-framework Foundation"
      "-framework GameController"
      "-framework Metal"
      "-framework QuartzCore"
      "-framework CoreHaptics"
    )
  elseif (LINUX)
    set_target_properties(SDL3-static PROPERTIES
      IMPORTED_LOCATION_DEBUG "${sdl_dir}/sdl-build/Debug/libSDL3.a"
      IMPORTED_LOCATION_RELEASE "${sdl_dir}/sdl-build/Release/libSDL3.a"
      IMPORTED_LOCATION_RELWITHDEBINFO "${sdl_dir}/sdl-build/Release/libSDL3.a"
      IMPORTED_LOCATION_MINSIZEREL "${sdl_dir}/sdl-build/Release/libSDL3.a"
      INTERFACE_LINK_LIBRARIES "\$<LINK_ONLY:m>"
      INTERFACE_LINK_OPTIONS "-pthread"
    )
  endif ()
endfunction()