#pragma once
        
#include <memory>
#include <vector>
#include "vulkan.hpp"

namespace vkd {
    class DescriptorLayout {
	public:
		DescriptorLayout(std::shared_ptr<Device> device) : _device(device) {}
		~DescriptorLayout();
		DescriptorLayout(DescriptorLayout&&) = delete;
		DescriptorLayout(const DescriptorLayout&) = delete;

		void add(VkDescriptorType type, uint32_t count, VkShaderStageFlags stage = VK_SHADER_STAGE_ALL);
		void create();

		void clear() { _bindings.clear(); }
		auto get() { return _layout; }
		auto bindings() { return _created_bindings; }

	private:
		std::shared_ptr<Device> _device = nullptr;
		std::vector<VkDescriptorSetLayoutBinding> _bindings;
		std::vector<VkDescriptorSetLayoutBinding> _created_bindings;
		VkDescriptorSetLayout _layout = VK_NULL_HANDLE;
	};
}