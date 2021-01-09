
set(SPIRV_SOURCES
    spirv_reflect.c)

add_library(spirv-reflect STATIC ${SPIRV_SOURCES})
target_link_libraries(spirv-reflect PUBLIC Vulkan::Vulkan)
target_include_directories(spirv-reflect PUBLIC ./)
