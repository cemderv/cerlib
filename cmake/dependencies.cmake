# Catch2
if (CERLIB_ENABLE_TESTS)
  CPMAddPackage(
    NAME snitch
    GITHUB_REPOSITORY snitch-org/snitch
    VERSION 1.2.5
    GIT_SHALLOW
  )
endif ()

# GSL
CPMAddPackage(
  NAME GSL
  GITHUB_REPOSITORY microsoft/GSL
  VERSION 4.0.0
  OPTIONS
  "GSL_CXX_STANDARD 20"
  "GSL_INSTALL OFF"
  "GSL_TEST OFF"
  "GSL_VS_ADD_NATIVE_VISUALIZERS ON"
  GIT_SHALLOW
)

# SoLoud
CPMAddPackage(
  NAME SoLoud
  GITHUB_REPOSITORY c-dervis/soloud
  GIT_TAG v0.0.1
  GIT_SHALLOW
)

set_target_properties(SoLoud PROPERTIES FOLDER "Dependencies")

# utfcpp
CPMAddPackage(
  NAME utfcpp
  GITHUB_REPOSITORY nemtrif/utfcpp
  VERSION 4.0.5
  GIT_SHALLOW
)

# stb
CPMAddPackage(
  NAME stb
  GITHUB_REPOSITORY nothings/stb
  GIT_TAG master
  GIT_SHALLOW
  DOWNLOAD_ONLY # stb does not use CMake, we add its files manually
)

CPMAddPackage(
  NAME opengl_loader
  GITHUB_REPOSITORY c-dervis/opengl-loader
  GIT_TAG v0.0.1
  GIT_SHALLOW
)

if (NOT EMSCRIPTEN)
  CPMAddPackage(
    NAME SDL
    GITHUB_REPOSITORY libsdl-org/SDL
    GIT_TAG 40d85109acb676686de0db0ced7209e24bc9fc7f
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
    GIT_SHALLOW
  )

  set_target_properties(SDL3-shared PROPERTIES FOLDER "Dependencies")
  set_target_properties(SDL3-static PROPERTIES FOLDER "Dependencies")

  if (TARGET SDL_uclibc)
    set_target_properties(SDL_uclibc PROPERTIES FOLDER "Dependencies")
  endif ()
endif ()
