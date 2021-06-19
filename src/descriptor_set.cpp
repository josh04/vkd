#include "descriptor_set.hpp"
#include "device.hpp"
#include "buffer.hpp"
#include "image.hpp"
#include "descriptor_sets.hpp"

namespace vkd {
    DescriptorSet::~DescriptorSet() {
	    VK_CHECK_RESULT(vkFreeDescriptorSets(_device->logical_device(), _pool->get(), 1, &_desc_set));
	}

    void DescriptorSet::add_buffer(Buffer& buffer) {
        auto buf = std::make_unique<VkDescriptorBufferInfo>();
        *buf = buffer.descriptor();
        _bindings.emplace_back(Binding{std::move(buf), nullptr});
    }

    void DescriptorSet::add_buffer(Buffer& buffer, uint32_t offset, uint32_t range) {
        auto buf = std::make_unique<VkDescriptorBufferInfo>();
        buf->buffer = buffer.get();
        buf->offset = offset;
        buf->range = range;
        _bindings.emplace_back(Binding{std::move(buf), nullptr});
    }

    void DescriptorSet::add_image(Image& image, VkSampler sampler) {
        auto im = std::make_unique<VkDescriptorImageInfo>();
        im->imageLayout = image.layout();
        im->imageView = image.view();
        im->sampler = sampler;
        _bindings.emplace_back(Binding{nullptr, std::move(im)});
    }
    
    void DescriptorSet::create() {
        VkDescriptorSetAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.descriptorPool = _pool->get();
        alloc_info.descriptorSetCount = 1;
        auto layout = _layout->get();
        alloc_info.pSetLayouts = &layout;

        VK_CHECK_RESULT(vkAllocateDescriptorSets(_device->logical_device(), &alloc_info, &_desc_set));

        update();
    }

    void DescriptorSet::update() {
        auto&& layout_bindings = _layout->bindings();
        std::vector<VkWriteDescriptorSet> write_desc_sets(layout_bindings.size());
        int i = 0;
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
}