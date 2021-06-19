#pragma once
#include <vector>
#include <string>
#include "vulkan/vulkan.h"

namespace vkd {
    class Instance {
    public:
        Instance() = default;
        ~Instance() = default;

        void init(bool validation);

        auto get() const { return _instance; }
        auto instance() const { return _instance; }

        const auto& supported_instance_extensions() const { return _supported_instance_extensions; }
        const auto& enabled_instance_extensions() const { return _enabled_instance_extensions; }
        const auto& instance_layer_properties() const { return _instance_layer_properties; }
    private:
	    static constexpr uint32_t api_version = VK_API_VERSION_1_0;
        VkInstance _instance;

        bool _validation = false;
	    std::vector<std::string> _supported_instance_extensions;
	    std::vector<std::string> _enabled_instance_extensions;
        std::vector<VkLayerProperties> _instance_layer_properties;
    };
}