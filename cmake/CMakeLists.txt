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

if(WIN32)
set(SDL2_PATH ../external/SDL2)
set(SDL2_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/../external/SDL2/include)
set(SDL2_LIBRARY ${CMAKE_SOURCE_DIR}/../external/SDL2/lib/RelWithDebInfo/SDL2.lib)

set(FFMPEG_ROOT ../external/ffmpeg-4.4-full_build-shared)

elseif(NOT APPLE)
set(SDL_INCLUDE_DIR ../external/SDL/include)
set(SDL_LIBRARY ../external/SDL/lib/Release/libSDL2.a)
else()
set(SDL2_PATH ../../train/externals)
set(SDL2_BUILDING_LIBRARY 1)
set(FFMPEG_ROOT ../external/ffmpeg-mac)
endif()

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Vulkan REQUIRED)
find_package(X11)
find_package(FFmpeg REQUIRED)


IF(SDL2_FOUND)
  add_library(sdl2 INTERFACE IMPORTED)
  set_property(TARGET sdl2 PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${SDL2_INCLUDE_DIR})
  set_property(TARGET sdl2 PROPERTY INTERFACE_LINK_LIBRARIES ${SDL2_LIBRARY})
ENDIF(SDL2_FOUND)

if(NOT APPLE AND UNIX)
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -Wno-psabi)
endif()

add_subdirectory(../external/enkiTS-1.10 ${CMAKE_BINARY_DIR}/enkiTS)
add_subdirectory(../external/spirv-reflect ${CMAKE_BINARY_DIR}/spirv-reflect)
add_subdirectory(../external/glm ${CMAKE_BINARY_DIR}/glm)
add_subdirectory(../external/ktx ${CMAKE_BINARY_DIR}/ktx)
add_subdirectory(../external/sago ${CMAKE_BINARY_DIR}/sago)
add_subdirectory(../external/imgui ${CMAKE_BINARY_DIR}/imgui)
add_subdirectory(../shaders ${CMAKE_BINARY_DIR}/shaders)
add_subdirectory(../textures ${CMAKE_BINARY_DIR}/textures)
add_subdirectory(../src ${CMAKE_BINARY_DIR}/src)
add_subdirectory(../tests ${CMAKE_BINARY_DIR}/tests)
