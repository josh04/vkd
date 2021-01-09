#pragma once

#include "vulkan.hpp"

namespace vulkan {
    void viewport_and_scissor(VkCommandBuffer buf, uint32_t view_width, uint32_t view_height, uint32_t scissor_width, uint32_t scissor_height);
    void viewport_and_scissor_with_offset(VkCommandBuffer buf, uint32_t view_offset_x, uint32_t view_offset_y, uint32_t view_width, uint32_t view_height, uint32_t scissor_width, uint32_t scissor_height);
}