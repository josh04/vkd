
set(IMGUI_SOURCE
    imgui_widgets.cpp
    imgui.cpp
    imgui_draw.cpp
    imgui_demo.cpp
    imgui_tables.cpp
    imnodes.cpp
    ImSequencer.cpp
    imgui_impl_vulkan.cpp
)

add_library(imgui STATIC ${IMGUI_SOURCE})

target_include_directories(imgui PUBLIC ..)
