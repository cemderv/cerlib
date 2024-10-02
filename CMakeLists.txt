# Copyright (C) 2023-2024 Cemalettin Dervis
# This file is part of cerlib.
# For conditions of distribution and use, see copyright notice in LICENSE.

cmake_minimum_required(VERSION 3.22)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

file(READ "VERSION.txt" CERLIB_VERSION)

if (CERLIB_VERSION STREQUAL "")
  message(FATAL_ERROR "No version found in VERSION.txt!")
endif ()

project(
  cerlib
  DESCRIPTION "cerlib is a 2D Game Library for C++"
  HOMEPAGE_URL "https://cerlib.org"
  LANGUAGES C CXX
  VERSION ${CERLIB_VERSION}
)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/Version.hpp.in
  ${CMAKE_CURRENT_SOURCE_DIR}/include/cerlib/Version.hpp
)

if (EMSCRIPTEN)
  set(CMAKE_EXECUTABLE_SUFFIX ".html")
endif ()

include(cmake/CPM.cmake)
include(CheckIPOSupported)

# General C++ settings
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(cerlib_binary_dir ${CMAKE_CURRENT_BINARY_DIR})
set(cerlib_cmake_scripts_dir "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Make support scripts available to the includer.
list(APPEND CMAKE_MODULE_PATH ${cerlib_cmake_scripts_dir}/support)

include(cmake/util.cmake)
include(cmake/options.cmake)
include(cmake/compiler_flags.cmake)
include(cmake/bin2header.cmake)

# Dependencies & main library
include(cmake/dependencies.cmake)
add_subdirectory(src)

# Applications
include(cmake/support/cerlib.cmake)
add_subdirectory(applications)

# Tests
if (CERLIB_ENABLE_TESTS)
  enable_testing()
  add_subdirectory(testing)
endif ()

