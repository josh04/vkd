#pragma once
        
#include <memory>
#include <vector>

#include "vulkan.hpp"
#include "vulkan.hpp"

namespace vulkan {
    class Device;
    class Buffer;
    class Image;
    class DescriptorLayout;
    class DescriptorPool;

	class DescriptorSet {
	public:
		DescriptorSet(std::shared_ptr<Device> device, std::shared_ptr<DescriptorLayout> layout, std::shared_ptr<DescriptorPool> pool) : _device(device), _layout(layout), _pool(pool) {}
        ~DescriptorSet();
		DescriptorSet(DescriptorSet&&) = delete;
		DescriptorSet(const DescriptorSet&) = delete;

		void create();
		void add_buffer(Buffer& buffer);
		void add_buffer(Buffer& buffer, uint32_t offset, uint32_t range);
		void add_image(Image& image, VkSampler sampler);
        void update();

		auto get() const { return _desc_set; }

	private:
	
		std::shared_ptr<Device> _device = nullptr;
		std::shared_ptr<DescriptorLayout> _layout = nullptr;
		std::shared_ptr<DescriptorPool> _pool = nullptr;

		struct Binding {
			std::unique_ptr<VkDescriptorBufferInfo> buffer; 
			std::unique_ptr<VkDescriptorImageInfo> image;
		};
		std::vector<Binding> _bindings;

		VkDescriptorSet _desc_set;
	};
}