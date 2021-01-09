#include "descriptor_pool.hpp"
#include "device.hpp"

namespace vulkan {
    
    void DescriptorPool::add_storage_image(uint32_t count) {
        if (count > 0) {
            _pool_elements.push_back(VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, count});
        }
    }

    void DescriptorPool::add_combined_image_sampler(uint32_t count) {
        if (count > 0) {
            _pool_elements.push_back(VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, count});
        }
    }

    void DescriptorPool::add_uniform_buffer(uint32_t count) {
        if (count > 0) {
            _pool_elements.push_back(VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, count});
        }
    }

    void DescriptorPool::add_storage_buffer(uint32_t count) {
        if (count > 0) {
            _pool_elements.push_back(VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, count});
        }
    }

    void DescriptorPool::create(uint32_t max_sets) {
        VkDescriptorPoolCreateInfo desc_pool_create_info = {};
        desc_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        desc_pool_create_info.pNext = nullptr;
        desc_pool_create_info.poolSizeCount = _pool_elements.size();
        desc_pool_create_info.pPoolSizes = _pool_elements.data();
        desc_pool_create_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT; // otherwise cannot call vkFreeDescriptorSet
        // Set the max. number of descriptor sets that can be requested from this pool (requesting beyond this limit will result in an error)
        desc_pool_create_info.maxSets = max_sets;

        VK_CHECK_RESULT(vkCreateDescriptorPool(_device->logical_device(), &desc_pool_create_info, nullptr, &_desc_pool));
    }
}