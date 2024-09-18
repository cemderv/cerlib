# CMake script for the game

cmake_minimum_required(VERSION 3.22)

# Set general game information.
set(game_name MyGame)
set(game_company_name MyCompany)
set(game_display_name "My Game")
set(game_company_display_name "My Company")
set(game_version 0.0.1)

# Declare the game project.
project(
  ${game_name}
  VERSION ${game_version}
  LANGUAGES CXX
)

if (ANDROID)
  set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
  set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
  set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)
endif ()

# Set the path where the cerlib package is located.
# This is modifiable and depends on where you put the cerlib package.
set(cerlib_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)

# Let CMake find the cerlib package and all of its libraries.
list(APPEND CMAKE_MODULE_PATH ${cerlib_DIR}/cmake)

add_subdirectory(
  ${cerlib_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}/cerlib_build
)

# Imports the cerlib_add_executable function.
include(${cerlib_DIR}/cmake/support/cerlib.cmake)

# Include the list of the game's source files.
file(
  GLOB_RECURSE
    game_source_files
  CONFIGURE_DEPENDS
    "src/*.hpp"
    "src/*.cpp"
)

# Register the game as a cerlib-based executable.
# This will call add_executable() internally, and set the executable
# up correctly depending on the target platform.
cerlib_add_executable(
  NAME ${game_name}
  COMPANY ${game_company_name}
  VERSION ${game_version}
  FILES ${game_source_files}
)

# When building for Android, include some cerlib-specific setup code and link with Android
# system libraries.
if (ANDROID)
  find_library(AndroidLibrary android REQUIRED)
  target_link_libraries(${game_name} PRIVATE ${AndroidLibrary})
  target_sources(${game_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/android_project/app/src/MainActivitySetup.cpp)
endif ()
