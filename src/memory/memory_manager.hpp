#pragma once
        
#include <memory>
#include <mutex>

#include "vulkan.hpp"

namespace vkd {
    class MemoryManager {
    public:
        MemoryManager() = default;
        ~MemoryManager() = default;
        MemoryManager(MemoryManager&&) = delete;
        MemoryManager(const MemoryManager&) = delete;

        void add_device_buffer(int64_t sz) {
            std::lock_guard<std::mutex> lock(_counter_mutex);
            _c.device_buffer_memory += sz;
            _c.device_buffer_count++;
        }

        void remove_device_buffer(int64_t sz) {
            std::lock_guard<std::mutex> lock(_counter_mutex);
            _c.device_buffer_memory -= sz;
            _c.device_buffer_count--;
        }

        void add_device_image(int64_t sz) {
            std::lock_guard<std::mutex> lock(_counter_mutex);
            _c.device_image_memory += sz;
            _c.device_image_count++;
        }

        void remove_device_image(int64_t sz) {
            std::lock_guard<std::mutex> lock(_counter_mutex);
            _c.device_image_memory -= sz;
            _c.device_image_count--;
        }

        void add_host_buffer(int64_t sz) {
            std::lock_guard<std::mutex> lock(_counter_mutex);
            _c.host_buffer_memory += sz;
            _c.host_buffer_count++;
        }

        void remove_host_buffer(int64_t sz) {
            std::lock_guard<std::mutex> lock(_counter_mutex);
            _c.host_buffer_memory -= sz;
            _c.host_buffer_count--;
        }

        struct Counters {
            int64_t device_buffer_memory = 0;
            int64_t device_buffer_count = 0;
            int64_t device_image_memory = 0;
            int64_t device_image_count = 0;
            int64_t host_buffer_memory = 0;
            int64_t host_buffer_count = 0;
        };

        Counters get_report() const {
            std::lock_guard<std::mutex> lock(_counter_mutex);
            return _c;
        }

        int64_t device_memory_used() const { 
            std::lock_guard<std::mutex> lock(_counter_mutex);
            return _c.device_buffer_memory + _c.device_image_memory;
        }
    private:
        mutable std::mutex _counter_mutex;
        Counters _c;
    };
}