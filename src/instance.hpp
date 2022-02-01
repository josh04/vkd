#pragma once
#include <vector>
#include <string>
#include "vulkan/vulkan.h"

#include "vkd_dll.h"

namespace vkd {
    class VKDEXPORT Instance {
    public:
        Instance() = default;
        ~Instance() = default;

        void init(bool validation);

        auto get() const { return _instance; }
        auto instance() const { return _instance; }

        const auto& supported_instance_extensions() const { return _supported_instance_extensions; }
        const auto& enabled_instance_extensions() const { return _enabled_instance_extensions; }
        const auto& instance_layer_properties() const { return _instance_layer_properties; }

        VkPhysicalDevice get_physical_device() const { return _physical_devices[0]; }

        const auto& physical_device_props() const { return _physical_device_props; }
        const auto& physical_device_feats() const { return _physical_device_feats; }
        const auto& physical_device_mem_props() const { return _physical_device_mem_props; }
    private:
	    static constexpr uint32_t api_version = VK_API_VERSION_1_0;
        VkInstance _instance;

        bool _validation = false;
	    std::vector<std::string> _supported_instance_extensions;
	    std::vector<std::string> _enabled_instance_extensions;
        std::vector<VkLayerProperties> _instance_layer_properties;

        std::vector<VkPhysicalDevice> _physical_devices;
        std::vector<VkPhysicalDeviceProperties> _physical_device_props;
        std::vector<VkPhysicalDeviceFeatures> _physical_device_feats;
        std::vector<VkPhysicalDeviceMemoryProperties> _physical_device_mem_props;
    };
}