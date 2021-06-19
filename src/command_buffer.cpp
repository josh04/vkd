#include "command_buffer.hpp"
#include "compute/kernel.hpp"

namespace vkd {
    VkCommandBuffer create_command_buffer(VkDevice logical_device, VkCommandPool pool) {
        VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
        memset(&commandBufferAllocateInfo, 0, sizeof(VkCommandBufferAllocateInfo));
        commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocateInfo.commandPool = pool;
        commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandBufferAllocateInfo.commandBufferCount = 1; // need one per swapchain image, but we only have SURFACE
        VkCommandBuffer command_buffer;
        VK_CHECK_RESULT(vkAllocateCommandBuffers(logical_device, &commandBufferAllocateInfo, &command_buffer));
        return command_buffer;
    }

    void begin_command_buffer(VkCommandBuffer buf) {
		begin_command_buffer(buf, VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);
    }

    void begin_command_buffer(VkCommandBuffer buf, VkCommandBufferUsageFlags flags) {
		VkCommandBufferBeginInfo command_buffer_begin_info = {};
		command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		command_buffer_begin_info.pNext = nullptr;
		command_buffer_begin_info.flags = flags;

        VK_CHECK_RESULT(vkBeginCommandBuffer(buf, &command_buffer_begin_info));
    }

    VkCommandBuffer begin_immediate_command_buffer(VkDevice logical_device, VkCommandPool pool) {
		auto buf = create_command_buffer(logical_device, pool);
		begin_command_buffer(buf, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		return buf;
	}

    void end_command_buffer(VkCommandBuffer buf) {
		VK_CHECK_RESULT(vkEndCommandBuffer(buf));
    }

	// End the command buffer and submit it to the queue
	// Uses a fence to ensure command buffer has finished executing before deleting it
	void flush_command_buffer(VkDevice device, VkQueue queue, VkCommandPool pool, VkCommandBuffer buf) {
		assert(buf != VK_NULL_HANDLE);

        end_command_buffer(buf);

		VkSubmitInfo submit_info = {};
		memset(&submit_info, 0, sizeof(VkSubmitInfo));
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &buf;

		// Create fence to ensure that the command buffer has finished executing
		VkFenceCreateInfo fenceCreateInfo = {};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCreateInfo.flags = 0;
		VkFence fence = create_fence(device, false);

		// Submit to the queue
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submit_info, fence));
		// Wait for the fence to signal that command buffer has finished executing
#define DEFAULT_FENCE_TIMEOUT 100000000000
		VK_CHECK_RESULT(vkWaitForFences(device, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));

		vkDestroyFence(device, fence, nullptr);
		vkFreeCommandBuffers(device, pool, 1, &buf);
	}

	void submit_command_buffer(VkQueue queue, VkCommandBuffer buf, VkPipelineStageFlags wait_stage_mask, VkSemaphore wait, VkSemaphore signal, VkFence fence) {
		// Submit compute commands
		VkSubmitInfo submit_info = {};
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.commandBufferCount = (buf != VK_NULL_HANDLE) ? 1 : 0;
		submit_info.pCommandBuffers = (buf != VK_NULL_HANDLE) ? &buf : VK_NULL_HANDLE;
		submit_info.waitSemaphoreCount = (wait != VK_NULL_HANDLE) ? 1 : 0;
		submit_info.pWaitSemaphores = (wait != VK_NULL_HANDLE) ? &wait : nullptr;
		submit_info.pWaitDstStageMask = &wait_stage_mask;
		submit_info.signalSemaphoreCount = (signal != VK_NULL_HANDLE) ? 1 : 0;
		submit_info.pSignalSemaphores = &signal;
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submit_info, fence));
	}

	void submit_compute_buffer(VkQueue queue, VkCommandBuffer buf, VkSemaphore wait, VkSemaphore signal, VkFence fence) {
		submit_command_buffer(queue, buf, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, wait, signal, fence);
	}

}