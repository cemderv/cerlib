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
