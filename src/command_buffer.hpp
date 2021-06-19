#pragma once 
#include "vulkan.hpp"
#include "fence.hpp"

namespace vkd {
	class Kernel;
    VkCommandBuffer create_command_buffer(VkDevice logical_device, VkCommandPool pool);

    void begin_command_buffer(VkCommandBuffer buf);
    void begin_command_buffer(VkCommandBuffer buf, VkCommandBufferUsageFlags flags);
    VkCommandBuffer begin_immediate_command_buffer(VkDevice logical_device, VkCommandPool pool);

    void end_command_buffer(VkCommandBuffer buf);

	// End the command buffer and submit it to the queue
	// Uses a fence to ensure command buffer has finished executing before deleting it
	void flush_command_buffer(VkDevice device, VkQueue queue, VkCommandPool pool, VkCommandBuffer buf);

	void submit_command_buffer(VkQueue queue, VkCommandBuffer buf, VkPipelineStageFlags wait_stage_mask, VkSemaphore wait = VK_NULL_HANDLE, VkSemaphore signal = VK_NULL_HANDLE, VkFence fence = VK_NULL_HANDLE);
	void submit_compute_buffer(VkQueue queue, VkCommandBuffer buf, VkSemaphore wait = VK_NULL_HANDLE, VkSemaphore signal = VK_NULL_HANDLE, VkFence fence = VK_NULL_HANDLE);

}