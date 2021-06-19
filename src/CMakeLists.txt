set(LIBVKD_SOURCE
    buffer.cpp
    command_buffer.cpp
    compute_pipeline.cpp
    descriptor_layout.cpp
    descriptor_pool.cpp
    descriptor_set.cpp
    depth_helper.cpp
    device.cpp
    engine_node.cpp
    framebuffer.cpp
    gl3w.c
    glerror.c
    image.cpp
    instance.cpp
    load_ktx.cpp
    parameter.cpp
    pipeline.cpp
    renderpass.cpp
    shader.cpp
    surface.cpp
    swapchain.cpp
    ui.cpp
    vertex_input.cpp
    viewport_and_scissor.cpp
    vulkan.cpp
    vulkan_enum.cpp

    compute/bilateral.cpp
    compute/gaussian.cpp
    compute/colour_squares.cpp
    compute/exposure.cpp
    compute/kernel.cpp
    compute/particles.cpp
    compute/sand.cpp
    graph/fake_node.cpp
    graph/graph.cpp
    inputs/exr.cpp
    inputs/ffmpeg.cpp
    render/draw_fullscreen.cpp
    render/draw_particles.cpp
    render/draw_triangle.cpp
    render/draw_ui.cpp
    ui/bin.cpp
    ui/nodes.cpp
    ui/timeline.cpp
    ui/performance.cpp
    ui/preferences.cpp
    ui/main_ui.cpp
)

set(VKD_SOURCE
    main.cpp
)

add_library(vkd SHARED ${LIBVKD_SOURCE})

target_include_directories(vkd PUBLIC ../include .)
add_dependencies(vkd spirv-shaders copy_tex)
target_link_libraries(vkd PUBLIC spirv-reflect imgui sdl2 platform_folders ${CMAKE_DL_LIBS} ktx glm::glm Vulkan::Vulkan )
target_link_libraries(vkd PUBLIC FFMPEG::avcodec FFMPEG::avformat FFMPEG::avutil FFMPEG::swscale )
if(NOT APPLE)
target_link_libraries(vkd PUBLIC xcb X11-xcb SDL2 ${X11_LIBRARIES})
else()
target_link_libraries(vkd PUBLIC ${OPENGL_LIBRARIES})
endif()

if(NOT APPLE)
target_compile_definitions(vkd PUBLIC VK_USE_PLATFORM_XCB_KHR=1)
else()
target_compile_definitions(vkd PUBLIC VK_USE_PLATFORM_MACOS_MVK=1)
endif()

add_executable(vkd-app ${VKD_SOURCE})
target_link_libraries(vkd-app PUBLIC vkd)
