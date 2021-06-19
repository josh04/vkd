
#include "viewport_and_scissor.hpp"

namespace vkd {
    void viewport_and_scissor(VkCommandBuffer buf, uint32_t view_width, uint32_t view_height, uint32_t scissor_width, uint32_t scissor_height) {
        // Update dynamic viewport state
        VkViewport viewport = {};
        viewport.width = (float)view_width;
        viewport.height = (float)view_height;
        viewport.minDepth = (float) 0.0f;
        viewport.maxDepth = (float) 1.0f;
        vkCmdSetViewport(buf, 0, 1, &viewport);

        // Update dynamic scissor state
        VkRect2D scissor = {};
        scissor.extent.width = scissor_width;
        scissor.extent.height = scissor_height;
        scissor.offset.x = 0;
        scissor.offset.y = 0;
        vkCmdSetScissor(buf, 0, 1, &scissor);
    }

    void viewport_and_scissor_with_offset(VkCommandBuffer buf, uint32_t view_offset_x, uint32_t view_offset_y, uint32_t view_width, uint32_t view_height, uint32_t scissor_width, uint32_t scissor_height) {
        // Update dynamic viewport state
        VkViewport viewport = {};
        viewport.x = (float)view_offset_x;
        viewport.y = (float)view_offset_y;
        viewport.width = (float)view_width;
        viewport.height = (float)view_height;
        viewport.minDepth = (float) 0.0f;
        viewport.maxDepth = (float) 1.0f;
        vkCmdSetViewport(buf, 0, 1, &viewport);

        // Update dynamic scissor state
        VkRect2D scissor = {};
        scissor.extent.width = scissor_width;
        scissor.extent.height = scissor_height;
        scissor.offset.x = 0;
        scissor.offset.y = 0;
        vkCmdSetScissor(buf, 0, 1, &scissor);
    }
}