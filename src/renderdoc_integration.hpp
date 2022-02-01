#pragma once

#include "vulkan/vulkan.h"

#include "vkd_dll.h"

namespace vkd {
#ifdef _WIN32
    VKDEXPORT void renderdoc_init(VkInstance instance, HWND window);
#endif

    VKDEXPORT bool renderdoc_enabled();
    VKDEXPORT void renderdoc_capture();
    VKDEXPORT void renderdoc_start_frame();
    VKDEXPORT void renderdoc_end_frame();
}