#pragma once
#include "vulkan.hpp"

namespace vulkan {
    static VkSemaphore create_semaphore(VkDevice logical_device) {
        // Semaphores (Used for correct command ordering)
		VkSemaphoreCreateInfo semaphore_create_info = {};
		semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		semaphore_create_info.pNext = nullptr;

		VkSemaphore semaphore;
		VK_CHECK_RESULT(vkCreateSemaphore(logical_device, &semaphore_create_info, nullptr, &semaphore));
        return semaphore;
    }
}