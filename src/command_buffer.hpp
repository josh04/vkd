#pragma once 
#include <vector>
#include "vulkan.hpp"
#include "fence.hpp"
#include "semaphore.hpp"

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

	void submit_immediate_command_buffer(VkDevice device, VkQueue queue, VkCommandBuffer buf);

	//void submit_command_buffer(VkQueue queue, VkCommandBuffer buf, VkPipelineStageFlags wait_stage_mask, VkSemaphore wait = VK_NULL_HANDLE, VkSemaphore signal = VK_NULL_HANDLE, const Fence * fence = nullptr);
	void submit_compute_buffer(Device& device, VkCommandBuffer buf, VkSemaphore wait = VK_NULL_HANDLE, VkSemaphore signal = VK_NULL_HANDLE, const Fence * fence = nullptr);
	void submit_compute_buffer(Device& device, VkCommandBuffer buf, std::nullptr_t, std::nullptr_t, const Fence * fence = nullptr);
	void submit_compute_buffer(Device& device, VkCommandBuffer buf, const SemaphorePtr& wait = nullptr, const SemaphorePtr& signal = nullptr, const Fence * fence = nullptr);

	void submit_compute_buffer_timeline(VkQueue queue, VkCommandBuffer buf, VkPipelineStageFlags wait_stage_mask, TimelineSemaphore& semaphore);
	void submit_compute_buffer_timeline(Device& device, VkCommandBuffer buf, TimelineSemaphore& semaphore);
	void submit_compute_buffer_timeline(Device& device, VkCommandBuffer buf, TimelineSemaphorePtr& semaphore);

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
		static CommandBufferPtr make_immediate(const std::shared_ptr<Device>& device);

		class ScopedRecord {
		public:
			ScopedRecord(CommandBuffer& cmd) : cmd(cmd) {
				cmd.begin();
			}
			~ScopedRecord() {
				cmd.end();
			}
			ScopedRecord(CommandBuffer&&) = delete;
			ScopedRecord(const CommandBuffer&) = delete;

		private:
			CommandBuffer& cmd;
		};
		
		ScopedRecord record() {
			return ScopedRecord(*this);
		}

		void begin();
		void end();

		void flush();

		void submit(const SemaphorePtr& wait = nullptr, const SemaphorePtr& signal = nullptr, Fence * fence = nullptr);
		void submit_timeline(TimelineSemaphorePtr& signal);
		void submit_timeline(TimelineSemaphore& signal);

		const SemaphorePtr& signal() const { return _last_submitted_signal; }

		auto get() const { return _buf; }

		void add_desc_set(std::shared_ptr<DescriptorSet> desc_set) { _desc_sets.push_back(desc_set); }

		const auto& device() const { return _device; }
		void debug_name(const std::string& debugName) { _debug_name = debugName; update_debug_name(); }
	protected:
		void update_debug_name();
		std::string _debug_name = "Anonymous Command Buffer";
		std::shared_ptr<Device> _device = nullptr;
		VkCommandBuffer _buf = VK_NULL_HANDLE;
		SemaphorePtr _default_signal = VK_NULL_HANDLE;
		SemaphorePtr _last_submitted_signal = nullptr;

		std::vector<std::shared_ptr<DescriptorSet>> _desc_sets;

		bool _flush_on_destruct = false;
	};
}