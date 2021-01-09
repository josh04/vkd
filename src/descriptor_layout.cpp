#include "descriptor_layout.hpp"
#include "device.hpp"

namespace vulkan {
    
    void DescriptorLayout::add(VkDescriptorType type, uint32_t count, VkShaderStageFlags stage) {
        VkDescriptorSetLayoutBinding layout_binding = {};
        layout_binding.descriptorType = type;
        layout_binding.descriptorCount = count;
        layout_binding.stageFlags = stage;
        layout_binding.binding = _bindings.size();
        layout_binding.pImmutableSamplers = nullptr;
        _bindings.push_back(layout_binding);
    }

    void DescriptorLayout::create() {
        VkDescriptorSetLayoutCreateInfo desc_layout_create_info = {};
        desc_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        desc_layout_create_info.pNext = nullptr;
        desc_layout_create_info.bindingCount = _bindings.size();
        desc_layout_create_info.pBindings = _bindings.data();

        VK_CHECK_RESULT(vkCreateDescriptorSetLayout(_device->logical_device(), &desc_layout_create_info, nullptr, &_layout));
        _created_bindings = _bindings;
    }
}