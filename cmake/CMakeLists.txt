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

set(CMAKE_POLICY_DEFAULT_CMP0074 NEW)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
include(CTest)

set(CMAKE_MODULE_PATH ./modules)

if(WIN32)

set(SDL2_PATH ../external/SDL2)
set(SDL2_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/../external/SDL2/include)
set(SDL2_LIBRARY ${CMAKE_SOURCE_DIR}/../external/SDL2/lib/RelWithDebInfo/SDL2.lib)

set(FFMPEG_ROOT ../external/ffmpeg-4.4-full_build-shared)

add_definitions(-DUNICODE -D_UNICODE)

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
if(NOT WIN32)
find_package(pthreads)
endif()

if(APPLE)
set(CMAKE_PREFIX_PATH ../external/ocio/mac/lib/cmake/OpenColorIO ${CMAKE_PREFIX_PATH})
elseif(WIN32)
set(CMAKE_PREFIX_PATH ../external/ocio/win32/lib/cmake/OpenColorIO ${CMAKE_PREFIX_PATH})
endif()

find_package(OpenColorIO CONFIG REQUIRED)

if(APPLE)
set(ZLIB_ROOT /usr/local/Cellar/zlib/1.2.13/)
elseif(WIN32)
set(ZLIB_ROOT ${CMAKE_SOURCE_DIR}/../external/zlib/)
endif()
find_package(ZLIB REQUIRED)
include_directories(${ZLIB_INCLUDE_DIRS})

if(APPLE)
set(CMAKE_OSX_SYSROOT /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX12.1.sdk)
set(MATH_LIBRARY /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX12.1.sdk/usr/lib/libm.tbd)
set(CMAKE_DISABLE_FIND_PACKAGE_JPEG TRUE)
set(JPEG_FOUND TRUE)
set(ENABLE_LCMS OFF)
endif()


IF(SDL2_FOUND)
  add_library(sdl2 INTERFACE IMPORTED)
  set_property(TARGET sdl2 PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${SDL2_INCLUDE_DIR})
  set_property(TARGET sdl2 PROPERTY INTERFACE_LINK_LIBRARIES ${SDL2_LIBRARY})
ENDIF(SDL2_FOUND)

if(WIN32)

  if (DEBUG)
      install(FILES ../external/SDL2/lib/Debug/SDL2d.dll DESTINATION bin)
      install(FILES ../external/zlib/bin/zlibd.dll DESTINATION bin)
  else()
      install(FILES ../external/SDL2/lib/RelWithDebInfo/SDL2.dll DESTINATION bin)
      install(FILES ../external/zlib/bin/zlib.dll DESTINATION bin)
  endif()
  install(DIRECTORY ../external/ffmpeg-4.4-full_build-shared/bin/ DESTINATION bin)
  install(FILES ../external/ocio/win32/bin/OpenColorIO_2_2.dll DESTINATION bin)
elseif(APPLE)
  install(FILES ../external/ocio/mac/lib/libOpenColorIO.2.2.0.dylib DESTINATION lib)
  install(FILES LibRaw/libraw_r.22.dylib LibRaw/libraw_r.dylib LibRaw/libraw_r.22.0.0.dylib DESTINATION lib)
  install(DIRECTORY ../../train/externals/SDL2.framework DESTINATION lib)
endif()

if (EXISTS ../external/aces_1.2/)
install(DIRECTORY ../external/aces_1.2/ DESTINATION bin/aces_1.2)
endif ()

if(NOT APPLE AND UNIX)
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -Wno-psabi)
endif()
if(WIN32)
#set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} /EHsc)
endif()

set(ENABLE_RAWSPEED ON CACHE BOOL "" FORCE)
option(ENABLE_RAWSPEED             "Build library with extra RawSpeed codec support (default=OFF)"                ON)
set(RAWSPEED_RPATH ../../ CACHE BOOL "" FORCE)
option(RAWSPEED_RPATH             "Build library with extra RawSpeed codec support (default=OFF)"                ../../)

if (NOT APPLE)
set(ENABLE_OPENMP ON)
else()
set(ENABLE_OPENMP OFF)
endif()

if(CMAKE_BUILD_TYPE MATCHES Debug)
    set(CMAKE_INSTALL_PREFIX ./bundle/debug)
    set(BUNDLE_DIR debug)
else(CMAKE_BUILD_TYPE MATCHES Debug)
    set(CMAKE_INSTALL_PREFIX ./bundle/release)
    set(BUNDLE_DIR release)
endif(CMAKE_BUILD_TYPE MATCHES Debug)

add_subdirectory(../external/enkiTS ${CMAKE_BINARY_DIR}/enkiTS)

target_include_directories(enkiTS PUBLIC "../external/enkiTS/src")

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(SPIRV_REFLECT_EXECUTABLE OFF)
set(SPIRV_REFLECT_EXAMPLES OFF)
set(SPIRV_REFLECT_STATIC_LIB ON)
set(ENABLE_RTTI ON)
set(ENABLE_EXCEPTIONS ON)

add_subdirectory(../external/spirv-reflect ${CMAKE_BINARY_DIR}/spirv-reflect)

set(BUILD_ST ${BUILD_SHARED_LIBS})
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(BUILD_SHARED_LIBS OFF)
set(BUILD_EXTERNAL OFF)
set(ENABLE_GLSLANG_BINARIES OFF)

enable_language( C CXX ASM )
set(WITH_JPEG8 ON)
set(CMAKE_INSTALL_DOCDIR ${CMAKE_BINARY_DIR}/docs)
set(CMAKE_INSTALL_MANDIR ${CMAKE_BINARY_DIR}/man)

add_subdirectory(../external/libjpeg-turbo ${CMAKE_BINARY_DIR}/libjpeg-turbo)

add_library(JPEG::JPEG ALIAS jpeg-static)
target_include_directories(jpeg-static PUBLIC $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libjpeg-turbo> $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/../external/libjpeg-turbo>)

add_subdirectory(../external/glslang ${CMAKE_BINARY_DIR}/glslang)
add_subdirectory(../external/tinyxml2 ${CMAKE_BINARY_DIR}/tinyxml2)
add_subdirectory(../external/glm ${CMAKE_BINARY_DIR}/glm)
add_subdirectory(../external/PlatformFolders ${CMAKE_BINARY_DIR}/PlatformFolders) 
set(SKIP_PERFORMANCE_COMPARISON ON)
set(JUST_INSTALL_CEREAL ON)
add_subdirectory(../external/cereal ${CMAKE_BINARY_DIR}/cereal)
set(BUILD_SHARED_LIBS ${BUILD_ST})

set(LIBRAW_PATH "${CMAKE_BINARY_DIR}/../external/LibRaw" CACHE STRING "")
add_subdirectory(../external/LibRaw-cmake ${CMAKE_BINARY_DIR}/LibRaw)

add_subdirectory(../external/filesystem ${CMAKE_BINARY_DIR}/filesystem)

add_subdirectory(../external/ktx ${CMAKE_BINARY_DIR}/ktx)
add_subdirectory(../external/Imath ${CMAKE_BINARY_DIR}/Imath)


set(IMATH_HEADER_ONLY_INCLUDE_DIRS ${CMAKE_BINARY_DIR}/Imath/config ${CMAKE_BINARY_DIR}/../external/Imath/src/Imath)

add_subdirectory(../external/openexr ${CMAKE_BINARY_DIR}/openexr)
add_subdirectory(../external/libpng ${CMAKE_BINARY_DIR}/libpng)
add_subdirectory(../external/imgui ${CMAKE_BINARY_DIR}/imgui)

add_subdirectory(../shaders ${CMAKE_BINARY_DIR}/shaders)
add_subdirectory(../textures ${CMAKE_BINARY_DIR}/textures)
add_subdirectory(../src ${CMAKE_BINARY_DIR}/src)
add_subdirectory(../tests ${CMAKE_BINARY_DIR}/tests)
