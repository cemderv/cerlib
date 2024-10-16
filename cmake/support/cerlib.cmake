# Copyright (C) 2023-2024 Cemalettin Dervis
# This file is part of cerlib.
# For conditions of distribution and use, see copyright notice in LICENSE.

function(cerlib_add_executable)
  set(options VERBOSE)
  set(one_value_args NAME COMPANY VERSION)
  set(multi_value_args FILES)

  cmake_parse_arguments(CERLIB_AE "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

  if (NOT TARGET cerlib)
    message(FATAL_ERROR "cerlib was not found! Did you forget to call find_package(cerlib)?")
  endif ()

  set(target_name ${CERLIB_AE_NAME})
  set(company ${CERLIB_AE_COMPANY})
  set(version ${CERLIB_AE_VERSION})
  set(current_dir ${CMAKE_CURRENT_SOURCE_DIR})
  set(binary_dir ${CMAKE_CURRENT_BINARY_DIR})
  set(assets_dir ${current_dir}/assets)

  if (${CERLIB_AE_VERBOSE})
    message(STATUS "Adding cerlib-based executable '${target_name}'")
    message(STATUS "  Company:         ${company}")
    message(STATUS "  Version:         ${version}")
    message(STATUS "  SourceDirectory: ${current_dir}")
    message(STATUS "  BinaryDirectory: ${binary_dir}")
    message(STATUS "  AssetsDirectory: ${assets_dir}")
  endif ()

  if (EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
  endif ()

  if (ANDROID)
    add_library(${target_name} SHARED ${CERLIB_AE_FILES})
  else ()
    if (WIN32)
      list(APPEND extra_flags WIN32)
    endif ()

    add_executable(${target_name} ${extra_flags} ${CERLIB_AE_FILES})
  endif ()

  target_compile_features(${target_name} PRIVATE cxx_std_20)

  set_target_properties(${target_name} PROPERTIES
    USE_FOLDERS TRUE
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
  )

  target_link_libraries(${target_name} PRIVATE cerlib)

  if (EMSCRIPTEN)
    target_compile_options(${target_name} PRIVATE -Wno-unused-command-line-argument)

    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
      set(link_flags
        "SHELL:-s ALLOW_MEMORY_GROWTH"
        "SHELL:-s ASSERTIONS"
        "SHELL:-s NO_DISABLE_EXCEPTION_CATCHING"
        "SHELL:-s LZ4"
        "SHELL:-s GL_ENABLE_GET_PROC_ADDRESS"
        "SHELL:--preload-file ${assets_dir}@"
      )
    else ()
      set(link_flags
        "SHELL:-s ALLOW_MEMORY_GROWTH"
        "SHELL:-s LZ4"
        "SHELL:-s GL_ENABLE_GET_PROC_ADDRESS"
        "SHELL:--preload-file ${assets_dir}@"
      )
    endif ()

    target_link_options(${target_name} PRIVATE ${link_flags})
  endif ()

  get_target_property(target_type cerlib TYPE)

  if (target_type STREQUAL "SHARED_LIBRARY")
    add_custom_command(
      TARGET ${target_name}
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_FILE:cerlib> $<TARGET_FILE_DIR:${target_name}>
    )
  endif ()

  set(script_dir ${CMAKE_CURRENT_FUNCTION_LIST_DIR})

  if (APPLE)
    set(bundle_identifier "com.${company}.${target_name}")
    set(bundle_version ${version})
    set(bundle_name ${target_name})
    set(bundle_display_name ${target_name})
    set(executable_name ${target_name})

    set(info_plist_dst_filename "${binary_dir}/${target_name}_Info.plist")
    configure_file("${script_dir}/Info.plist.in" ${info_plist_dst_filename})

    set_target_properties(${target_name} PROPERTIES
      MACOSX_BUNDLE TRUE
      MACOSX_BUNDLE_GUI_IDENTIFIER ${bundle_display_name}
      MACOSX_BUNDLE_INFO_PLIST ${info_plist_dst_filename}
      XCODE_LINK_BUILD_PHASE_MODE BUILT_ONLY
      BUILD_WITH_INSTALL_RPATH TRUE
      INSTALL_RPATH "@executable_path/"
    )

    file(GLOB_RECURSE
      asset_files
      CONFIGURE_DEPENDS
      "${assets_dir}/*"
    )

    target_sources(${target_name} PRIVATE ${asset_files})

    foreach (file ${asset_files})
      file(RELATIVE_PATH new_file ${assets_dir} ${file})
      get_filename_component(new_file_path ${new_file} DIRECTORY)
      set_property(
        SOURCE ${file}
        PROPERTY MACOSX_PACKAGE_LOCATION "Resources/${new_file_path}"
      )
      source_group("Resources/${new_file_path}" FILES "${file}")
    endforeach ()
  else ()
    add_custom_target(${target_name}_CopyAssets ALL
      COMMAND ${CMAKE_COMMAND} -E copy_directory
      ${assets_dir} $<TARGET_FILE_DIR:${target_name}>/
    )

    add_dependencies(${target_name} ${target_name}_CopyAssets)

    set_target_properties(
      ${target_name}
      ${target_name}_CopyAssets
      PROPERTIES
      VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:${target_name}>
      FOLDER ${target_name}
    )
  endif ()

  if (ANDROID)
    foreach (file ${asset_files})
      string(REPLACE ${assets_dir} "" asset_short_file ${file})
      list(APPEND asset_short_files ${CMAKE_ANDROID_ASSETS_DIRECTORIES}${asset_short_file})
    endforeach ()

    add_custom_command(
      TARGET ${target_name}
      POST_BUILD
      COMMAND
      ${CMAKE_COMMAND} -E copy_directory ${assets_dir} ${CMAKE_ANDROID_ASSETS_DIRECTORIES}/
      OUTPUT ${asset_short_files}
    )
  endif ()

  if (NOT EMSCRIPTEN AND cerlib_FOUND)
    add_custom_command(
      TARGET ${target_name}
      POST_BUILD
      COMMAND
      ${CMAKE_COMMAND} -E copy_if_different ${cerlib_LIBRARY_LOCATION} $<TARGET_FILE_DIR:${target_name}>
    )
  endif ()

  target_precompile_headers(${target_name} PRIVATE
    <cerlib.hpp>
  )
endfunction()
