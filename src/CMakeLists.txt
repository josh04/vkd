set(LIBVKD_SOURCE
    application.cpp
    buffer.cpp
    command_buffer.cpp
    compute_pipeline.cpp
    descriptor_layout.cpp
    descriptor_pool.cpp
    descriptor_set.cpp
    depth_helper.cpp
    device.cpp
    engine_node.cpp
    fence.cpp
    framebuffer.cpp
    gl3w.c
    glerror.c
    host_cache.cpp
    image.cpp
    imgui_drawer.cpp
    instance.cpp
    load_ktx.cpp
    parameter.cpp
    pipeline.cpp
    renderdoc_integration.cpp
    renderpass.cpp
    shader.cpp
    surface.cpp
    swapchain.cpp
    vertex_input.cpp
    viewport_and_scissor.cpp
    vulkan.cpp
    vulkan_enum.cpp

    graph/fake_node.cpp
    graph/graph.cpp
    render/draw_fullscreen.cpp
    render/draw_particles.cpp
    render/draw_triangle.cpp
    render/draw_ui.cpp
    ui/bin.cpp
    ui/node_window.cpp
    ui/timeline.cpp
    ui/performance.cpp
    ui/preferences.cpp
    ui/main_ui.cpp
    ui/memory_window.cpp
    ui/render_window.cpp
    ui/inspector.cpp
)

set(VKD_SOURCE
    vkd-app.cpp
)

set(VKD_PHOTO_SOURCE
    vkd-photo.cpp
)

add_library(vkd SHARED ${LIBVKD_SOURCE})

add_subdirectory(compute ${CMAKE_CURRENT_BINARY_DIR}/compute)
add_subdirectory(inputs ${CMAKE_CURRENT_BINARY_DIR}/inputs)
add_subdirectory(memory ${CMAKE_CURRENT_BINARY_DIR}/memory)
add_subdirectory(ocio ${CMAKE_CURRENT_BINARY_DIR}/ocio)
add_subdirectory(outputs ${CMAKE_CURRENT_BINARY_DIR}/outputs)

target_compile_definitions(vkd PRIVATE LIBVKD_EXPORTS)

target_include_directories(vkd PUBLIC ../include .)
add_dependencies(vkd spirv-shaders copy_tex)

if (WIN32)
    if (DEBUG)
        install(FILES ${CMAKE_SOURCE_DIR}/LibRaw/raw_d.dll DESTINATION bin)
    else()
        install(FILES ${CMAKE_SOURCE_DIR}/LibRaw/raw.dll DESTINATION bin)
    endif()
endif()

target_link_libraries(vkd PUBLIC SPIRV glslang OpenColorIO::OpenColorIO tinyxml2 spirv-reflect-static imgui sdl2 platform_folders ${CMAKE_DL_LIBS} ktx glm::glm Vulkan::Vulkan enkiTS Imath::Imath OpenEXR::OpenEXRCore OpenEXR::OpenEXR libraw::libraw png)
target_link_libraries(vkd PUBLIC FFMPEG::avcodec FFMPEG::avformat FFMPEG::avutil FFMPEG::swscale)

target_include_directories(vkd PRIVATE ../cmake/libpng)
target_include_directories(vkd PRIVATE ../external/libpng)
target_include_directories(vkd PUBLIC ../external/SPIRV-Reflect)

if (WIN32)
target_link_libraries(vkd PUBLIC)
elseif(NOT APPLE)
target_link_libraries(vkd PUBLIC xcb X11-xcb SDL2 ${X11_LIBRARIES})
else()
target_link_libraries(vkd PUBLIC ${OPENGL_LIBRARIES})
endif()

if (WIN32)
target_compile_definitions(vkd PUBLIC VK_USE_PLATFORM_WIN32_KHR=1 -D_CRT_SECURE_NO_WARNINGS -DNOMINMAX)
elseif(NOT APPLE)
target_compile_definitions(vkd PUBLIC VK_USE_PLATFORM_XCB_KHR=1)
else()
target_compile_definitions(vkd PUBLIC VK_USE_PLATFORM_MACOS_MVK=1)
endif()

add_executable(vkd-app ${VKD_SOURCE})
target_link_libraries(vkd-app PUBLIC vkd sdl2)

add_executable(vkd-photo ${VKD_PHOTO_SOURCE})
target_link_libraries(vkd-photo PUBLIC vkd sdl2)

if(WIN32)
target_compile_options(vkd PRIVATE "/wd4251")
target_compile_options(vkd-app PRIVATE "/wd4251")
target_compile_options(vkd-photo PRIVATE "/wd4251")
install(TARGETS vkd DESTINATION bin)
endif()

if (NOT WIN32)
install(TARGETS vkd DESTINATION lib)
endif()
install(TARGETS vkd-app DESTINATION bin)
install(TARGETS vkd-photo DESTINATION bin)

if(APPLE)
    install(CODE "
      
        execute_process(COMMAND \"/usr/bin/install_name_tool\" \"-add_rpath\" \"@executable_path/\" \"${CMAKE_BINARY_DIR}/bundle/${BUNDLE_DIR}/bin/vkd-app\")
        execute_process(COMMAND \"/usr/bin/install_name_tool\" \"-add_rpath\" \"@executable_path/../lib\" \"${CMAKE_BINARY_DIR}/bundle/${BUNDLE_DIR}/bin/vkd-app\")

        execute_process(COMMAND \"/usr/bin/install_name_tool\" \"-add_rpath\" \"@executable_path/\" \"${CMAKE_BINARY_DIR}/bundle/${BUNDLE_DIR}/bin/vkd-photo\")
        execute_process(COMMAND \"/usr/bin/install_name_tool\" \"-add_rpath\" \"@executable_path/../lib\" \"${CMAKE_BINARY_DIR}/bundle/${BUNDLE_DIR}/bin/vkd-photo\")

        execute_process(COMMAND \"/usr/bin/install_name_tool\" \"-change\" \"libglslang.11.dylib\" \"@rpath/libglslang.11.dylib\" \"${CMAKE_BINARY_DIR}/bundle/${BUNDLE_DIR}/bin/vkd-app\")
        execute_process(COMMAND \"/usr/bin/install_name_tool\" \"-change\" \"libglslang.11.dylib\" \"@rpath/libglslang.11.dylib\" \"${CMAKE_BINARY_DIR}/bundle/${BUNDLE_DIR}/bin/vkd-photo\")
        execute_process(COMMAND \"/usr/bin/install_name_tool\" \"-change\" \"libglslang.11.dylib\" \"@rpath/libglslang.11.dylib\" \"${CMAKE_BINARY_DIR}/bundle/${BUNDLE_DIR}/lib/libvkd.dylib\")
        execute_process(COMMAND \"/usr/bin/install_name_tool\" \"-change\" \"libSPIRV.dylib\" \"@rpath/libSPIRV.dylib\" \"${CMAKE_BINARY_DIR}/bundle/${BUNDLE_DIR}/bin/vkd-app\")
        execute_process(COMMAND \"/usr/bin/install_name_tool\" \"-change\" \"libSPIRV.dylib\" \"@rpath/libSPIRV.dylib\" \"${CMAKE_BINARY_DIR}/bundle/${BUNDLE_DIR}/bin/vkd-photo\")
        execute_process(COMMAND \"/usr/bin/install_name_tool\" \"-change\" \"libSPIRV.dylib\" \"@rpath/libSPIRV.dylib\" \"${CMAKE_BINARY_DIR}/bundle/${BUNDLE_DIR}/lib/libvkd.dylib\")

        #execute_process(COMMAND \"codesign --sign - ${CMAKE_BINARY_DIR}/bundle/${BUNDLE_DIR}/bin/spacetrain.app\")

       " COMPONENT Bundle)
endif(APPLE)

