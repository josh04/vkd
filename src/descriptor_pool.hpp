#pragma once
        
#include <memory>
#include <vector>
#include "vulkan.hpp"

namespace vulkan {
    class Device;
	class DescriptorPool {
	public:
		DescriptorPool(std::shared_ptr<Device> device) : _device(device) {}
		~DescriptorPool() = default;
		DescriptorPool(DescriptorPool&&) = delete;
		DescriptorPool(const DescriptorPool&) = delete;

		void add_storage_image(uint32_t count);
		void add_combined_image_sampler(uint32_t count);
		void add_uniform_buffer(uint32_t count);
		void add_storage_buffer(uint32_t count);
		void create(uint32_t max_sets);

		auto get() const { return _desc_pool; }

	private:
		std::shared_ptr<Device> _device = nullptr;
		std::vector<VkDescriptorPoolSize> _pool_elements;
        VkDescriptorPool _desc_pool;
	};
}