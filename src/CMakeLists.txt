
set(DEMO_SOURCE
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
    main.cpp
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
    inputs/exr.cpp
    inputs/ffmpeg.cpp
    render/draw_fullscreen.cpp
    render/draw_particles.cpp
    render/draw_triangle.cpp
    render/draw_ui.cpp
    ui/nodes.cpp
    ui/timeline.cpp
)

add_executable(demo ${DEMO_SOURCE})

target_include_directories(demo PUBLIC ../include .)
add_dependencies(demo spirv-shaders copy_tex)
target_link_libraries(demo PUBLIC spirv-reflect imgui sdl2 ${CMAKE_DL_LIBS} ktx glm::glm Vulkan::Vulkan )
target_link_libraries(demo PUBLIC FFMPEG::avcodec FFMPEG::avformat FFMPEG::avutil FFMPEG::swscale )
if(NOT APPLE)
target_link_libraries(demo PUBLIC xcb X11-xcb SDL2 ${X11_LIBRARIES})
else()
target_link_libraries(demo PUBLIC ${OPENGL_LIBRARIES})
endif()

if(NOT APPLE)
target_compile_definitions(demo PUBLIC VK_USE_PLATFORM_XCB_KHR=1)
else()
target_compile_definitions(demo PUBLIC VK_USE_PLATFORM_MACOS_MVK=1)
endif()