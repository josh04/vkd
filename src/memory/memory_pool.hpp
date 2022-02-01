#pragma once

#include <deque>
#include <vector>
#include <map>

#include "vulkan/vulkan.hpp"

namespace vkd {
    class Device;
    class MemoryPool {
    public:
        struct Alloc {
            VkDeviceMemory mem;
            VkDeviceSize size; 
            VkMemoryPropertyFlags memory_property_flags;
            uint32_t memory_type_index;
        };

        struct AllocInfo {
            VkDeviceSize size; 
            VkMemoryPropertyFlags memory_property_flags;
            uint32_t memory_type_index;
        };
        
		MemoryPool(Device& device) : _device(device) {}
		~MemoryPool() = default;
		MemoryPool(MemoryPool&&) = delete;
		MemoryPool(const MemoryPool&) = delete;

        VkDeviceMemory allocate(VkDeviceSize size, VkMemoryPropertyFlags memory_property_flags, uint32_t memory_type_index);
        bool deallocate(VkDeviceMemory mem);

        void trim(size_t limit);

        const auto pool() { return _pool; }
    private:
        bool _destroy(VkDeviceMemory mem);
        std::deque<Alloc> _pool;
        std::map<VkDeviceMemory, AllocInfo> _allocs;
        Device& _device;
    };
}