cmake_minimum_required(VERSION 3.13)

# Project name and a few useful settings. Other commands can pick up the results
project(
  demo
  VERSION 0.1
  DESCRIPTION "An example project with CMake"
  LANGUAGES C CXX)

set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
include(CTest)

set(SDL_INCLUDE_DIR ../SDL/include)
set(SDL_LIBRARY ../SDL/lib/Release/libSDL2.a)
find_package(SDL2 REQUIRED)

add_subdirectory(../src ${CMAKE_BINARY_DIR}/src)

if (BUILD_TESTING)
  #add_subdirectory(tests)
endif()
