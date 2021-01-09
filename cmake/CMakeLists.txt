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

set(CMAKE_MODULE_PATH ./modules)

if(NOT APPLE)
set(SDL_INCLUDE_DIR ../external/SDL/include)
set(SDL_LIBRARY ../external/SDL/lib/Release/libSDL2.a)
else()
set(SDL2_PATH ../../train/externals)
set(SDL2_BUILDING_LIBRARY 1)
set(FFMPEG_ROOT ../external/ffmpeg-mac)
endif()

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Vulkan)
find_package(X11)
find_package(FFmpeg REQUIRED)


IF(SDL2_FOUND)
  add_library(sdl2 INTERFACE IMPORTED)
  set_property(TARGET sdl2 PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${SDL2_INCLUDE_DIR})
  set_property(TARGET sdl2 PROPERTY INTERFACE_LINK_LIBRARIES ${SDL2_LIBRARY})
ENDIF(SDL2_FOUND)

if(NOT APPLE)
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -Wno-psabi)
endif()

add_subdirectory(../external/spirv-reflect ${CMAKE_BINARY_DIR}/spirv-reflect)
add_subdirectory(../external/glm ${CMAKE_BINARY_DIR}/glm)
add_subdirectory(../external/ktx ${CMAKE_BINARY_DIR}/ktx)
add_subdirectory(../external/imgui ${CMAKE_BINARY_DIR}/imgui)
add_subdirectory(../shaders ${CMAKE_BINARY_DIR}/shaders)
add_subdirectory(../textures ${CMAKE_BINARY_DIR}/textures)
add_subdirectory(../src ${CMAKE_BINARY_DIR}/src)

if (BUILD_TESTING)
  #add_subdirectory(tests)
endif()
