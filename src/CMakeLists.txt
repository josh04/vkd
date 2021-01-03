
set(DEMO_SOURCE
    gl3w.c
    ui.cpp
    main.cpp
    glerror.c
    vulkan.cpp
    vulkan_enum.cpp
)

add_executable(demo ${DEMO_SOURCE})
add_subdirectory(imgui)

target_include_directories(demo PUBLIC ../include)

if (NOT CMAKE_VERSION VERSION_LESS 3.7.0)
	message(STATUS "Using module to find Vulkan")
	find_package(Vulkan)
endif()

target_link_libraries(demo PUBLIC imgui SDL2 ${CMAKE_DL_LIBS} Vulkan::Vulkan)
