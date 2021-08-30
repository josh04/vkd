#pragma once 
#include <vector>
#include "vulkan.hpp"
#include "fence.hpp"

namespace vkd {
	class Kernel;
	class DescriptorSet;
    VkCommandBuffer create_command_buffer(VkDevice logical_device, VkCommandPool pool);

    void begin_command_buffer(VkCommandBuffer buf);
    void begin_command_buffer(VkCommandBuffer buf, VkCommandBufferUsageFlags flags);
    VkCommandBuffer begin_immediate_command_buffer(VkDevice logical_device, VkCommandPool pool);

    void end_command_buffer(VkCommandBuffer buf);

	// End the command buffer and submit it to the queue
	// Uses a fence to ensure command buffer has finished executing before deleting it
	void flush_command_buffer(VkDevice device, VkQueue queue, VkCommandPool pool, VkCommandBuffer buf);

	void submit_command_buffer(VkQueue queue, VkCommandBuffer buf, VkPipelineStageFlags wait_stage_mask, VkSemaphore wait = VK_NULL_HANDLE, VkSemaphore signal = VK_NULL_HANDLE, Fence * fence = nullptr);
	void submit_compute_buffer(VkQueue queue, VkCommandBuffer buf, VkSemaphore wait = VK_NULL_HANDLE, VkSemaphore signal = VK_NULL_HANDLE, Fence * fence = nullptr);

	class CommandBuffer;
	using CommandBufferPtr = std::unique_ptr<CommandBuffer>;
	class CommandBuffer {
		public:
			CommandBuffer() = default;
			~CommandBuffer();
			CommandBuffer(CommandBuffer&&) = delete;
			CommandBuffer(const CommandBuffer&) = delete;

			void create(const std::shared_ptr<Device>& device);

			static CommandBufferPtr make(const std::shared_ptr<Device>& device);

			void begin();

			void end();

			void flush();

			void submit(VkSemaphore wait = VK_NULL_HANDLE, VkSemaphore signal = VK_NULL_HANDLE, Fence * fence = nullptr);

			auto get() const { return _buf; }

			void add_desc_set(std::shared_ptr<DescriptorSet> desc_set) { _desc_sets.push_back(desc_set); }
		private:
			std::shared_ptr<Device> _device = nullptr;
			VkCommandBuffer _buf = VK_NULL_HANDLE;

			std::vector<std::shared_ptr<DescriptorSet>> _desc_sets;
	};
}