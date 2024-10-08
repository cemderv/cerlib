# Copyright (C) 2023-2024 Cemalettin Dervis
# This file is part of cerlib.
# For conditions of distribution and use, see copyright notice in LICENSE.

set(cerlib_gfx_backend "OpenGL")

add_subdirectory(stb)

set(cerlib_library_type STATIC)

if (CERLIB_SHARED_LIBRARY)
  set(cerlib_library_type SHARED)
endif ()

if (EMSCRIPTEN)
  set(cerlib_library_type STATIC)
endif ()

cerlib_log("Creating cerlib library of type (${cerlib_library_type})")

add_library(cerlib ${cerlib_library_type})

set(cerlib_include_dir ${CMAKE_CURRENT_SOURCE_DIR}/../include)
set(cerlib_src_dir ${CMAKE_CURRENT_SOURCE_DIR})

set(cerlib_public_files
  cerlib/Audio.hpp
  cerlib/BlendState.hpp
  cerlib/Circle.hpp
  cerlib/Color.hpp
  cerlib/Content.hpp
  cerlib/Drawing.hpp
  cerlib/Event.hpp
  cerlib/Font.hpp
  cerlib/Game.hpp
  cerlib/GraphicsResource.hpp
  cerlib/Image.hpp
  cerlib/Input.hpp
  cerlib/Key.hpp
  cerlib/KeyModifier.hpp
  cerlib/Math.hpp
  cerlib/OStreamCompat.hpp
  cerlib/Matrix.hpp
  cerlib/Hashing.hpp
  cerlib/MouseButton.hpp
  cerlib/Rectangle.hpp
  cerlib/Sampler.hpp
  cerlib/Shader.hpp
  cerlib/SmallDataArray.hpp
  cerlib/Sound.hpp
  cerlib/SoundChannel.hpp
  cerlib/SoundTypes.hpp
  cerlib/ToString.hpp
  cerlib/Tween.hpp
  cerlib/Vector2.hpp
  cerlib/Export.hpp
  cerlib/Vector3.hpp
  cerlib/Vector4.hpp
  cerlib/Version.hpp
  cerlib/Window.hpp
  cerlib/Main.hpp
  cerlib/RunGame.hpp
  cerlib/details/MainImpl.hpp
  cerlib/Gamepad.hpp
  cerlib/Util.hpp
  cerlib.hpp
)

list(TRANSFORM cerlib_public_files PREPEND ${cerlib_include_dir}/)

# Add public files to target
target_sources(cerlib PRIVATE ${cerlib_public_files})
target_include_directories(cerlib PUBLIC ${cerlib_include_dir})

source_group("Public" FILES ${cerlib_public_files})

set(src_sub_dirs
  audio
  contentmanagement
  game
  graphics
  input
  math
  shadercompiler
  util
)

foreach (dir ${src_sub_dirs})
  include(${dir}/Files.cmake)

  set(absolute_filenames ${${dir}_files})
  list(TRANSFORM absolute_filenames PREPEND ${cerlib_src_dir}/${dir}/)

  # Add private files to target
  target_sources(cerlib PRIVATE ${absolute_filenames})

  string(REPLACE "/" "\\" dir_name_for_source_group ${dir})

  source_group("Private\\${dir_name_for_source_group}" FILES ${absolute_filenames})
endforeach ()

set_target_properties(cerlib PROPERTIES FOLDER "cerlib")

if (cerlib_library_type STREQUAL "SHARED")
  target_compile_definitions(cerlib PUBLIC -DCERLIB_SHARED_LIBRARY)
  target_compile_definitions(cerlib PRIVATE -Dcerlib_EXPORTS)
endif ()

if (cerlib_gfx_backend STREQUAL "OpenGL")
  cerlib_log("Using OpenGL")
  include(graphics/opengl/Files.cmake)
  list(TRANSFORM opengl_files PREPEND ${cerlib_src_dir}/graphics/opengl/)
  target_link_libraries(cerlib PUBLIC opengl-loader)
  target_sources(cerlib PRIVATE ${opengl_files})
endif ()

if (CERLIB_ATOMIC_REFCOUNTING)
  target_compile_definitions(cerlib PRIVATE -DCERLIB_ATOMIC_REFCOUNTING)
endif ()

target_include_directories(cerlib PUBLIC ${cerlib_include_dir})
target_include_directories(cerlib PRIVATE ${cerlib_src_dir})

# Link out-of-source dependencies
target_link_libraries(cerlib PRIVATE
  GSL
  utf8cpp
  stblib
)

target_link_libraries(cerlib PUBLIC
  fmt::fmt-header-only
)

add_library(cerlib_SDL INTERFACE)

if (CERLIB_ENABLE_TESTS)
  target_compile_definitions(cerlib PUBLIC "-DCERLIB_ENABLE_TESTS")
endif ()

if (ANDROID)
  # On Android, we have to use the shared library version of SDL.
  # This is because the main activity calls directly into functions that are
  # exposed by SDL, via JNI.
  cerlib_log("Linking with SHARED library of SDL3")
  target_link_libraries(cerlib_SDL INTERFACE SDL3-shared)
elseif (EMSCRIPTEN)
  # Use Emscripten's SDL port.
  cerlib_log("Linking with Emscripten's port of SDL2")
  target_link_libraries(cerlib_SDL INTERFACE SDL2)
else ()
  # Can link SDL directly into the cerlib library.
  cerlib_log("Linking with STATIC library of SDL3")
  target_link_libraries(cerlib_SDL INTERFACE SDL3-static)
endif ()

if (EMSCRIPTEN)
  target_compile_options(cerlib PUBLIC
    "SHELL:-s USE_SDL=2"
  )

  target_link_options(cerlib PUBLIC
    "SHELL:-s MIN_WEBGL_VERSION=2"
    "SHELL:-s FULL_ES3=1"
    "SHELL:-sDEFAULT_LIBRARY_FUNCS_TO_INCLUDE=[\"$autoResumeAudioContext\",\"$dynCall\"]"
  )
endif ()

if (CERLIB_ENABLE_IMGUI)
  add_subdirectory(imgui)
  target_link_libraries(cerlib PUBLIC cerlib_imgui)
  target_compile_definitions(cerlib PRIVATE -DCERLIB_ENABLE_IMGUI)
endif ()

if (ANDROID)
  target_link_libraries(cerlib PUBLIC dl m atomic stdc++ android log)

  if (ANDROID_ABI STREQUAL armeabi-v7a)
    set_target_properties(cerlib PROPERTIES COMPILE_FLAGS -mfpu=neon)
  endif ()
endif ()

if (IOS)
  set_target_properties(cerlib PROPERTIES
    FRAMEWORK TRUE
    MACOSX_FRAMEWORK_IDENTIFIER com.cemdervis.cerlib
    VERSION ${CMAKE_PROJECT_VERSION}
    SOVERSION ${CMAKE_PROJECT_VERSION}
  )
endif ()

target_link_libraries(cerlib PUBLIC cerlib_SDL)
target_link_libraries(cerlib PRIVATE SoLoud)

enable_default_cpp_flags(cerlib)
enable_clang_tidy(cerlib)

if (CERLIB_ENABLE_VERBOSE_LOGGING)
  target_compile_definitions(cerlib PRIVATE "-DCERLIB_ENABLE_VERBOSE_LOGGING")
endif ()

cerlib_embed_file(cerlib ${CMAKE_CURRENT_SOURCE_DIR}/resources/VeraRegular.ttf)
cerlib_embed_file(cerlib ${CMAKE_CURRENT_SOURCE_DIR}/resources/VeraBold.ttf)
cerlib_embed_file(cerlib ${CMAKE_CURRENT_SOURCE_DIR}/resources/GrayscaleShader.shd)

cerlib_compile_shader(shaders/SpriteBatchVS.vert)
cerlib_compile_shader(shaders/SpriteBatchPSDefault.frag)
cerlib_compile_shader(shaders/SpriteBatchPSMonochromatic.frag)
