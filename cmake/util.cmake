function(cerlib_log message)
  message(STATUS "[cerlib] ${message}")
endfunction()

function(cerlib_warn message)
  message(WARNING "[cerlib] ${message}")
endfunction()

function(cerlib_fatal_error message)
  message(FATAL_ERROR "[cerlib] ${message}")
endfunction()

macro(cerlib_compile_shader filename)
  cerlib_embed_file(cerlib ${CMAKE_CURRENT_SOURCE_DIR}/${filename})
endmacro()

macro(cerlib_ensure_dependency_added dependency_name)
  if (NOT ${dependency_name}_ADDED)
    cerlib_fatal_error("Dependency \'${dependency_name}\' was not found. Deleting the CMake cache may help.")
  endif()
endmacro()
