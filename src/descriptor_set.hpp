#pragma once
        
#include <memory>
#include <vector>

#include "vulkan.hpp"
#include "sampler.hpp"
#include "image_types.hpp"
#include "buffer_types.hpp"

namespace vkd {
    class Device;
    class DescriptorLayout;
    class DescriptorPool;

	class DescriptorSet {
	public:
		DescriptorSet(std::shared_ptr<Device> device, std::shared_ptr<DescriptorLayout> layout, std::shared_ptr<DescriptorPool> pool) : _device(device), _layout(layout), _pool(pool) {}
        ~DescriptorSet();
		DescriptorSet(DescriptorSet&&) = delete;
		DescriptorSet(const DescriptorSet&) = delete;

		void create();
		void add_buffer(BufferPtr buffer);
		void add_buffer(BufferPtr buffer, uint32_t offset, uint32_t range);
    	void add_image(ImagePtr image, const ScopedSamplerPtr& sampler);
		void add_image(ImagePtr image, VkSampler sampler);
        void update();
		void flush();

		auto get() const { return _desc_set; }

        void debug_name(const std::string& debugName) { _debug_name = debugName; update_debug_name(); }
    protected:
        void update_debug_name();
        std::string _debug_name = "Anonymous Desc Set";
	
		std::shared_ptr<Device> _device = nullptr;
		std::shared_ptr<DescriptorLayout> _layout = nullptr;
		std::shared_ptr<DescriptorPool> _pool = nullptr;

		struct Binding {
			std::unique_ptr<VkDescriptorBufferInfo> buffer; 
			std::unique_ptr<VkDescriptorImageInfo> image;
		};
		std::vector<Binding> _bindings;

		VkDescriptorSet _desc_set = VK_NULL_HANDLE;

		std::vector<BufferPtr> _buffer_storage;
		std::vector<ImagePtr> _image_storage;
	};
}