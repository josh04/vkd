#pragma once
#include <vector>
#include <memory>
#include <string>
#include "vulkan/vulkan.h"

#include "instance.hpp"

namespace vulkan {
    class Device {
    public:
        Device(std::shared_ptr<Instance> instance) : _instance(instance) {}
        ~Device();
        Device(Device&&) = delete;
        Device(const Device&) = delete;

        void create(VkPhysicalDevice physical_device);
        VkCommandPool create_command_pool(uint32_t queue_index);

        auto instance() const { return _instance; }
        auto physical_device() const { return _physical_device; }
        auto logical_device() const { return _logical_device; }
        auto queue() const { return _queue; }
        auto queue_index() const { return _queue_index; }
        auto compute_queue() const { return _queue; }
        auto compute_queue_index() const { return _queue_index; }
        auto command_pool() { return _command_pool; }

        const auto& queue_family_props() const { return _logicalDeviceQueueFamilyProps; }
        const auto& device_extension_props() const { return _device_extension_props; }
        const auto& device_supported_extensions() const { return _device_supported_extensions; }
        const auto& device_extensions_enabled() const { return _device_extensions_enabled; }
        const auto& memory_properties() const { return _memory_properties; }
    private:
        void populate_physical_device_props(VkPhysicalDevice device);
        std::shared_ptr<Instance> _instance;
        VkPhysicalDevice _physical_device;
        VkDevice _logical_device;
        static constexpr uint32_t _queue_index = 0;
        VkQueue _queue;
        VkCommandPool _command_pool;

        std::vector<VkQueueFamilyProperties> _logicalDeviceQueueFamilyProps;
        std::vector<VkExtensionProperties> _device_extension_props;
		std::vector<std::string> _device_extensions_enabled;
        std::vector<std::string> _device_supported_extensions;

        VkPhysicalDeviceMemoryProperties _memory_properties;

    };
}