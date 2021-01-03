
set(IMGUI_SOURCE
    imgui_widgets.cpp
    imgui.cpp
    imgui_draw.cpp
    imgui_demo.cpp
)

add_library(imgui STATIC ${IMGUI_SOURCE})
