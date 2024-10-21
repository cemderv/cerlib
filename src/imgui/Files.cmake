set(imgui_files
  imgui.h
  imconfig.h
  impl/imgui.cpp
  impl/imgui_draw.cpp
  impl/imgui_tables.cpp
  impl/imgui_widgets.cpp
)

if (EMSCRIPTEN)
  list(APPEND imgui_files
    impl/imgui_impl_sdl2.hpp
    impl/imgui_impl_sdl2.cpp
  )
else ()
  list(APPEND imgui_files
    impl/imgui_impl_sdl3.hpp
    impl/imgui_impl_sdl3.cpp
  )
endif ()

if (cerlib_gfx_backend STREQUAL "OpenGL")
  list(APPEND imgui_files
    impl/imgui_impl_opengl3.hpp
    impl/imgui_impl_opengl3.cpp
  )
endif ()
