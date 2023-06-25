#include "descriptor_set.hpp"
#include "device.hpp"
#include "buffer.hpp"
#include "image.hpp"
#include "descriptor_sets.hpp"
#include "graph_exception.hpp"

namespace vkd {
    DescriptorSet::~DescriptorSet() {
	    VK_CHECK_RESULT_NO_THROW(vkFreeDescriptorSets(_device->logical_device(), _pool->get(), 1, &_desc_set));
	}

    void DescriptorSet::add_buffer(BufferPtr buffer) {
        if (buffer == nullptr) { throw std::runtime_error("Null buffer passed to DescriptorSet"); }
        auto buf = std::make_unique<VkDescriptorBufferInfo>();
        *buf = buffer->descriptor();
        _bindings.emplace_back(Binding{std::move(buf), nullptr});
        _buffer_storage.emplace_back(std::move(buffer));
    }

    void DescriptorSet::add_buffer(BufferPtr buffer, uint32_t offset, uint32_t range) {
        if (buffer == nullptr) { throw std::runtime_error("Null buffer passed to DescriptorSet"); }
        auto buf = std::make_unique<VkDescriptorBufferInfo>();
        buf->buffer = buffer->get();
        buf->offset = offset;
        buf->range = range;
        _bindings.emplace_back(Binding{std::move(buf), nullptr});
        _buffer_storage.emplace_back(std::move(buffer));
    }

    void DescriptorSet::add_image(ImagePtr image, const ScopedSamplerPtr& sampler) {
        if (image == nullptr) { throw std::runtime_error("Null image passed to DescriptorSet"); }
        add_image(std::move(image), sampler->get());
    }

    void DescriptorSet::add_image(ImagePtr image, VkSampler sampler) {
        if (image == nullptr) { throw std::runtime_error("Null image passed to DescriptorSet"); }
        auto im = std::make_unique<VkDescriptorImageInfo>();
        im->imageLayout = image->layout();
        im->imageView = image->view();
        im->sampler = sampler;
        _bindings.emplace_back(Binding{nullptr, std::move(im)});
        _image_storage.emplace_back(std::move(image));
    }
    
    void DescriptorSet::create() {
        VkDescriptorSetAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.descriptorPool = _pool->get();
        alloc_info.descriptorSetCount = 1;
        auto layout = _layout->get();
        alloc_info.pSetLayouts = &layout;

        VK_CHECK_RESULT(vkAllocateDescriptorSets(_device->logical_device(), &alloc_info, &_desc_set));

        update_debug_name();
        update();
    }

    void DescriptorSet::update_debug_name() {
        if (_desc_set != VK_NULL_HANDLE)
        {
            _device->set_debug_utils_object_name(_debug_name, VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t)_desc_set);
        }
    }

    void DescriptorSet::update() {
        auto&& layout_bindings = _layout->bindings();
        std::vector<VkWriteDescriptorSet> write_desc_sets(layout_bindings.size());
        int i = 0;
        if (layout_bindings.size() != _bindings.size()) {
            throw UpdateException("bindings not sufficient");
        }
        for (auto&& bind : layout_bindings) {
            auto&& write_desc_set = write_desc_sets[i];
            write_desc_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write_desc_set.dstSet = _desc_set;
            write_desc_set.descriptorCount = 1;
            write_desc_set.descriptorType = bind.descriptorType;

            write_desc_set.pBufferInfo = _bindings[i].buffer.get();
            write_desc_set.pImageInfo = _bindings[i].image.get();

            write_desc_set.dstBinding = i;

            ++i;
        }

        vkUpdateDescriptorSets(_device->logical_device(), write_desc_sets.size(), write_desc_sets.data(), 0, nullptr);
    }

    void DescriptorSet::flush() {
        _bindings.clear();
        _buffer_storage.clear();
        _image_storage.clear();
    }
}