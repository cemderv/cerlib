# CMake script for the game

cmake_minimum_required(VERSION 3.22)

# Declare the game project.
project(
  MyGame
  VERSION 0.0.1
  DESCRIPTION "My Game"
  LANGUAGES CXX
)

# Set up build directories for Android.
if (ANDROID)
  set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
  set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
  set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)
endif ()

# Set the path where the cerlib package is located.
# This is modifiable and depends on where you put the cerlib repository.
set(cerlib_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_subdirectory(
  ${cerlib_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}/cerlib_build
)

# Define our game as a CMake target.
cerlib_add_game(
  NAME ${CMAKE_PROJECT_NAME}
  COMPANY MyCompany
  VERSION ${CMAKE_PROJECT_VERSION}
)
