#include <algorithm>
#include <iostream>

#include "memory_pool.hpp"
#include "device.hpp"

namespace vkd {

    VkDeviceMemory MemoryPool::allocate(VkDeviceSize size, VkMemoryPropertyFlags memory_property_flags, uint32_t memory_type_index) {
        int i = 0;
        for (auto&& entry : _pool) {
            if (size > entry.size || entry.memory_type_index != memory_type_index || entry.memory_property_flags != memory_property_flags) {
                i++;
                continue;
            }

            if (std::abs((int64_t)size - (int64_t)entry.size) < 5000000) {
                auto mem = entry.mem;
                _pool.erase(_pool.begin() + i);
                return mem;
            }

            i++;
        }

        VkDeviceMemory mem = VK_NULL_HANDLE;

        // or alloc
        VkMemoryAllocateInfo mem_alloc_info{};
        memset(&mem_alloc_info, 0, sizeof(VkMemoryAllocateInfo));

        mem_alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        mem_alloc_info.allocationSize = size;
        mem_alloc_info.memoryTypeIndex = memory_type_index;
        
        if (memory_property_flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
            _device.memory_manager().add_host_buffer(size);
        } else {
            _device.memory_manager().add_device_image(size);
        }

        std::cout << "Pool allocating " << size / (1024.0 * 1024.0) << "mb allocation." << std::endl;
        VK_CHECK_RESULT(vkAllocateMemory(_device.logical_device(), &mem_alloc_info, nullptr, &mem));
        _allocs[mem] = {size, memory_property_flags, memory_type_index};


        return mem;
    }

    bool MemoryPool::deallocate(VkDeviceMemory mem) {
        auto search = _allocs.find(mem);
        if (search == _allocs.end()) {
            return false;
        }

        _pool.push_back({mem, search->second.size, search->second.memory_property_flags, search->second.memory_type_index});
        std::sort(_pool.begin(), _pool.end(), [](const Alloc& lhs, const Alloc& rhs) { return lhs.size < rhs.size; });

        return true;
    }

    bool MemoryPool::_destroy(VkDeviceMemory mem) {
        auto search = _allocs.find(mem);
        if (search == _allocs.end()) {
            return false;
        }

        vkFreeMemory(_device.logical_device(), mem, nullptr); 

        if (search->second.memory_property_flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
            _device.memory_manager().remove_host_buffer(search->second.size);
        } else {
            _device.memory_manager().remove_device_image(search->second.size);
        }

        _allocs.erase(mem);

        return true;
    }

    void MemoryPool::trim(size_t limit) {
        auto current_mem = _device.memory_manager().device_memory_used();

        while (current_mem > limit) {
            if (_pool.empty()) {
                std::cerr << "Warning: Pool emptied without reaching allocation limit." << std::endl;
                return;
            }
            auto rbeg = _pool.back();
            _pool.pop_back();
            std::cout << "Pool trim deallocating " << rbeg.size / (1024.0 * 1024.0) << "mb allocation." << std::endl;
            _destroy(rbeg.mem);
            current_mem = _device.memory_manager().device_memory_used();
        }
    }
}