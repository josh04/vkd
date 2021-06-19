#pragma once
#include "vulkan.hpp"
#include "semaphore.hpp"

namespace vkd {
    static VkFence create_fence(VkDevice logical_device, bool signalled) {
        VkFenceCreateInfo fence_create_info{};
        memset(&fence_create_info, 0, sizeof(VkFenceCreateInfo));
        fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_create_info.flags = (signalled ? VK_FENCE_CREATE_SIGNALED_BIT : 0);
            
        VkFence fence;
        VK_CHECK_RESULT(vkCreateFence(logical_device, &fence_create_info, nullptr, &fence));
        return fence;
    }
}