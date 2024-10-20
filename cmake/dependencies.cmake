# snitch
if (CERLIB_ENABLE_TESTS)
  CPMAddPackage(
    NAME snitch
    GITHUB_REPOSITORY snitch-org/snitch
    VERSION 1.2.5
    GIT_SHALLOW
    SYSTEM
  )
endif ()

# fmt
CPMAddPackage(
  NAME fmt
  GITHUB_REPOSITORY fmtlib/fmt
  GIT_TAG 11.0.2
  OPTIONS
  "FMT_INSTALL OFF"
  "FMT_SYSTEM_HEADERS ON"
  GIT_SHALLOW
  SYSTEM
)

set_target_properties(fmt PROPERTIES FOLDER "Dependencies")

# stb
CPMAddPackage(
  NAME stb
  GITHUB_REPOSITORY nothings/stb
  GIT_TAG master
  GIT_SHALLOW
  DOWNLOAD_ONLY # stb does not use CMake, we add its files manually
)

# cerlib's OpenGL loader
CPMAddPackage(
  NAME opengl_loader
  GITHUB_REPOSITORY cemderv/opengl-loader
  GIT_TAG v0.0.2
  GIT_SHALLOW
  SYSTEM
)

# ImGui
if (CERLIB_ENABLE_IMGUI)
  CPMAddPackage(
    NAME ImGui
    GITHUB_REPOSITORY ocornut/imgui
    VERSION 1.91.3
    GIT_SHALLOW
  )
endif ()

if (NOT EMSCRIPTEN)
  set(sdl_version "3.1.3")

  include(cmake/sdl_prebuilt.cmake)
  cerlib_check_can_use_sdl_prebuilt()

  if (NOT CERLIB_FORCE_SDL_BUILD AND cerlib_can_use_prebuilt_sdl)
    cerlib_log("Using prebuilt SDL")
    cerlib_fetch_and_add_prebuilt_sdl(
      "${cerlib_sdl_prebuilt_url}/v${sdl_version}/${cerlib_prebuilt_sdl_package}.zip"
    )
  else ()
    cerlib_log("Building SDL along with cerlib")

    CPMAddPackage(
      NAME SDL
      GITHUB_REPOSITORY libsdl-org/SDL
      GIT_TAG preview-${sdl_version}
      OPTIONS
      "SDL_MISC OFF"
      "SDL_OFFSCREEN OFF"
      "SDL_POWER OFF"
      "SDL_TEST OFF"
      "SDL_TESTS OFF"
      "SDL_TEST_LIBRARY OFF"
      "SDL_TEST_ENABLED_BY_DEFAULT OFF"
      "SDL_VULKAN OFF"
      "SDL_OPENGL ON"
      "SDL_OPENGLES ON"
      "SDL_RENDER_D3D OFF"
      "SDL_RENDER_METAL OFF"
      "SDL_DIRECTX OFF"
      "SDL_DISABLE_UNINSTALL ON"
      "SDL_VENDOR_INFO OFF"
      "SDL_3DNOW OFF"
      "SDL_ALTIVEC OFF"
      "SDL_CCACHE ON"
      "SDL_DUMMYVIDEO OFF"
      "SDL_WASAPI OFF"
      "SDL_ATOMIC OFF"
      "SDL_DIALOG OFF"
      "SDL_LIBUDEV OFF"
      "SDL_STATIC_VCRT OFF"
      "SDL_STATIC ON"
      "SDL_SHARED ON"
      "SDL_GPU OFF"
      GIT_SHALLOW
      SYSTEM
    )

    set_target_properties(SDL3-shared PROPERTIES FOLDER "Dependencies")
    set_target_properties(SDL3-static PROPERTIES FOLDER "Dependencies")

    if (TARGET SDL_uclibc)
      set_target_properties(SDL_uclibc PROPERTIES FOLDER "Dependencies")
    endif ()
  endif ()
endif ()
