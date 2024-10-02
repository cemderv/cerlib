cerlib_ensure_dependency_added(ImGui)

cerlib_Log("Including ImGui as part of cerlib")

project(cerlib_imgui LANGUAGES CXX)

add_library(cerlib_imgui STATIC)

target_compile_features(cerlib_imgui PRIVATE cxx_std_17)

# Provide the user access to the ImGui API.
target_include_directories(cerlib_imgui SYSTEM PUBLIC ${ImGui_SOURCE_DIR})

target_sources(cerlib_imgui PRIVATE
  ${ImGui_SOURCE_DIR}/imgui.cpp
  ${ImGui_SOURCE_DIR}/imgui_draw.cpp
  ${ImGui_SOURCE_DIR}/imgui_tables.cpp
  ${ImGui_SOURCE_DIR}/imgui_widgets.cpp
)

# Platform-specific files
if (EMSCRIPTEN)
  target_sources(cerlib_imgui PRIVATE
    ${ImGui_SOURCE_DIR}/backends/imgui_impl_sdl2.h
    ${ImGui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
  )
else ()
  target_sources(cerlib_imgui PRIVATE
    ${ImGui_SOURCE_DIR}/backends/imgui_impl_sdl3.h
    ${ImGui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
  )
endif ()

# Graphics API-specific files
if (cerlib_gfx_backend STREQUAL "OpenGL")
  target_sources(cerlib_imgui PRIVATE
    ${ImGui_SOURCE_DIR}/backends/imgui_impl_opengl3.h
    ${ImGui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
  )

  if (ANDROID)
    target_link_libraries(cerlib_imgui PRIVATE GLESv3 EGL)
  endif ()
endif ()

# We use ImGui's SDL variant, so provide it access to SDL.
target_link_libraries(cerlib_imgui PRIVATE cerlib_SDL)

# Our ImGui counts as an external dependency.
set_target_properties(cerlib_imgui PROPERTIES FOLDER "Dependencies")

if (EMSCRIPTEN)
  target_compile_options(cerlib_imgui PRIVATE "SHELL:-s USE_SDL=2")
  target_link_options(cerlib_imgui PRIVATE "SHELL:-s MIN_WEBGL_VERSION=2" "SHELL:-s FULL_ES3=1")
endif ()