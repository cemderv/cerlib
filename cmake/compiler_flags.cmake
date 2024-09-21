# Address sanitizer
if (CERLIB_ENABLE_ADDRESS_SANITIZER)
  cerlib_log("Enabling address sanitizer")

  set(CMAKE_XCODE_SCHEME_ADDRESS_SANITIZER ON)
  set(CMAKE_XCODE_SCHEME_ADDRESS_SANITIZER_USE_AFTER_RETURN ON)

  add_compile_options(-fsanitize=address)
  add_link_options(-fsanitize=address)
endif ()

function(enable_default_cpp_flags targetName)
  target_compile_features(${targetName} PUBLIC cxx_std_20)
  set_target_properties(${targetName} PROPERTIES CXX_EXTENSIONS OFF)

  set_target_properties(${targetName}
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${cerlib_binary_dir}/bin
    LIBRARY_OUTPUT_DIRECTORY ${cerlib_binary_dir}/bin
    RUNTIME_OUTPUT_DIRECTORY ${cerlib_binary_dir}/bin
  )

  set_target_properties(${targetName}
    PROPERTIES
    XCODE_ATTRIBUTE_OSX_DEPLOYMENT_TARGET "12.0"
    XCODE_ATTRIBUTE_IPHONE_DEPLOYMENT_TARGET "12.0"
  )

  if (MSVC)
    # Warning level 4, warnings as errors etc.
    target_compile_options(${targetName} PRIVATE /W4 /WX /MP)

    # Ignore warnings about sprintf etc.
    target_compile_definitions(${targetName} PRIVATE -D_CRT_SECURE_NO_WARNINGS)
  else ()
    target_compile_options(${targetName} PRIVATE -Wall -Wextra -Wpedantic -Werror)

    # Disable some of the warnings
    target_compile_options(${targetName} PRIVATE -Wno-nonnull)
  endif ()

  # Compile-time tracing
  if (CERLIB_ENABLE_CLANG_TIME_TRACE)
    cerlib_log("Enabling Clang time tracing")
    target_compile_options(${targetName} PRIVATE -ftime-trace)
  endif ()

  # Disable common warnings
  if (MSVC)
    target_compile_options(${targetName} PRIVATE
      /wd4100 # Unreferenced formal parameter
      /wd4505 # Unreferenced function with internal linkage has been removed
      /wd4251 # DLL export warnings
      /wd4458 # Declaration of xyz hides class member
    )
  else ()
    target_compile_options(${targetName} PRIVATE
      -Wno-unused-parameter
      -Wno-dtor-name
      -Wno-unused-private-field
      -Wno-gnu-anonymous-struct
      -Wno-nested-anon-types
      -Wno-unused-function
      -Wno-psabi
      -fvisibility=hidden # Don't export symbols by default
    )
  endif ()

  set(CMAKE_XCODE_GENERATE_SCHEME ON)

  # clang-tidy
  if (CERLIB_ENABLE_CLANG_TIDY)
    cerlib_log("Enabling clang-tidy")

    if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang" OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "AppleClang")
      find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
      if (CLANG_TIDY_EXE)
        set(CLANG_TIDY_COMMAND "${CLANG_TIDY_EXE}" "-p" "${CMAKE_BINARY_DIR}" "--config-file=${CMAKE_SOURCE_DIR}/.clang-tidy")
        set_target_properties(${targetName} PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}")
      else ()
        cerlib_warn("clang-tidy executable not found; ignoring")
      endif ()
    else ()
      cerlib_warn("clang-tidy analysis is enabled, however the current compiler (${CMAKE_CXX_COMPILER_ID}) is not clang-tidy-compatible; ignoring")
    endif ()
  endif ()

  if (CERLIB_ENABLE_LTO)
    check_ipo_supported(RESULT ltoSupported OUTPUT error)
    if (ltoSupported)
      cerlib_log("Enabling LTO for target ${targetName}")
      set_property(TARGET ${targetName} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    else ()
      cerlib_log("LTO NOT supported by the compiler; ignoring for target ${targetName}")
    endif ()
  endif ()

  if (ANDROID)
    # Non-standard type char_traits<...> is not supported beginning with LLVM 19.
    # But we need it right now, so disable those warnings.
    target_compile_options(${targetName} PRIVATE -Wno-deprecated-declarations)
  endif ()
endfunction()
