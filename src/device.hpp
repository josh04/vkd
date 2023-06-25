#pragma once
#include <vector>
#include <memory>
#include <string>
#include "vulkan/vulkan.h"

#include "instance.hpp"
#include "memory/memory_manager.hpp"

#include "vkd_dll.h"

namespace vkd {
    class HostCache;
    class MemoryManager;
    class MemoryPool;
    class VKDEXPORT Device {
    public:
        Device(std::shared_ptr<Instance> instance);
        ~Device();
        Device(Device&&) = delete;
        Device(const Device&) = delete;
        Device& operator=(const Device&) = delete;

        void create(VkPhysicalDevice physical_device);
        VkCommandPool create_command_pool(uint32_t queue_index);

        auto instance() const { return _instance; }
        auto physical_device() const { return _physical_device; }
        auto logical_device() const { return _logical_device; }
        auto queue() const { return _queue; }
        auto queue_index() const { return _queue_index; }
        auto& queue_mutex() { return _queue_mutex; }
        auto compute_queue() const { return _queue; }
        auto compute_queue_index() const { return _queue_index; }
        auto command_pool() { return _command_pool; }

        const auto& queue_family_props() const { return _logicalDeviceQueueFamilyProps; }
        const auto& device_extension_props() const { return _device_extension_props; }
        const auto& device_supported_extensions() const { return _device_supported_extensions; }
        const auto& device_extensions_enabled() const { return _device_extensions_enabled; }
        const auto& memory_properties() const { return _memory_properties; }

        const auto& features() const { return _physical_features.features; }
        const auto& ext_8bit_features() const { return _8bit_features; }
        const auto& ext_16bit_features() const { return _16bit_features; }

        auto& host_cache() { return *_host_cache; }
        auto& memory_manager() { return *_memory_manager; }
        auto& pool() { return *_memory_pool; }

        void set_debug_utils_object_name(const std::string& name, VkObjectType type, uint64_t object);
    private:
        void populate_physical_device_props(VkPhysicalDevice device);
        std::shared_ptr<Instance> _instance = nullptr;
        VkPhysicalDevice _physical_device = VK_NULL_HANDLE;
        VkDevice _logical_device = VK_NULL_HANDLE;
        static constexpr uint32_t _queue_index = 0;
        std::mutex _queue_mutex;
        VkQueue _queue = VK_NULL_HANDLE;
        VkCommandPool _command_pool = VK_NULL_HANDLE;

        std::vector<VkQueueFamilyProperties> _logicalDeviceQueueFamilyProps;
        std::vector<VkExtensionProperties> _device_extension_props;
		std::vector<std::string> _device_extensions_enabled;
        std::vector<std::string> _device_supported_extensions;

        VkPhysicalDeviceMemoryProperties _memory_properties;
        VkPhysicalDeviceFeatures2 _physical_features;
        VkPhysicalDevice8BitStorageFeaturesKHR _8bit_features;
        VkPhysicalDevice16BitStorageFeatures _16bit_features;

        PFN_vkGetPhysicalDeviceFeatures2KHR ext_vkGetPhysicalDeviceFeatures2KHR = nullptr;

        std::unique_ptr<HostCache> _host_cache; // these three are not default initialised to nullptr to avoid
        std::unique_ptr<MemoryManager> _memory_manager; // compile issue on
        std::unique_ptr<MemoryPool> _memory_pool; // clang
    };
}