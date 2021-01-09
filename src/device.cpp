#include <iostream>
#include "device.hpp"
#include "vulkan.hpp"
#ifdef __APPLE__
#include "vulkan/vulkan_beta.h"
#endif
#include <algorithm>

namespace vulkan {
    Device::~Device() {
        vkResetCommandPool(_logical_device, _command_pool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
    }

    void Device::create(VkPhysicalDevice physical_device) {
        _physical_device = physical_device;

        populate_physical_device_props(physical_device);

		const float defaultQueuePriority = 0.0f;

        std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
        VkDeviceQueueCreateInfo queue_create_info = {};
        memset(&queue_create_info, 0, sizeof(VkDeviceQueueCreateInfo));
        queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.queueFamilyIndex = 0; // only one queue family itt touch wood
        queue_create_info.queueCount = 1;
        queue_create_info.pQueuePriorities = &defaultQueuePriority;

        queue_create_infos.push_back(queue_create_info);

		VkDeviceCreateInfo device_create_info = {};
		device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		device_create_info.queueCreateInfoCount = (uint32_t)queue_create_infos.size();
		device_create_info.pQueueCreateInfos = queue_create_infos.data();
        VkPhysicalDeviceFeatures enabled_features{};
		device_create_info.pEnabledFeatures = &enabled_features;
		
        /*
		// If a pNext(Chain) has been passed, we need to add it to the device creation info
		VkPhysicalDeviceFeatures2 physicalDeviceFeatures2{};
		if (pNextChain) {
			physicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
			physicalDeviceFeatures2.features = enabledFeatures;
			physicalDeviceFeatures2.pNext = pNextChain;
			deviceCreateInfo.pEnabledFeatures = nullptr;
			deviceCreateInfo.pNext = &physicalDeviceFeatures2;
		}
        */
/*
		// Enable the debug marker extension if it is present (likely meaning a debugging tool is present)
		if (extensionSupported(VK_EXT_DEBUG_MARKER_EXTENSION_NAME))
		{
			deviceExtensions.push_back(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
			enableDebugMarkers = true;
		}
*/

		// If the device will be used for presenting to a display via a swapchain we need to request the swapchain extension
		_device_extensions_enabled.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

#ifdef __APPLE__
		_device_extensions_enabled.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
#endif
        std::vector<const char *> devExts;
        for (const auto& enabledExtension : _device_extensions_enabled) {
            if ((std::find(_device_supported_extensions.begin(), _device_supported_extensions.end(), enabledExtension) == _device_supported_extensions.end())) {
                std::cerr << "Enabled device extension \"" << enabledExtension << "\" is not present at device level" << std::endl;
            } else {
                devExts.push_back(enabledExtension.c_str());
            }
        }

        device_create_info.enabledExtensionCount = (uint32_t)devExts.size();
        device_create_info.ppEnabledExtensionNames = (devExts.size() ? devExts.data() : nullptr);

        VkDevice logicalDevice = {0};

		VkResult result = vkCreateDevice(_physical_device, &device_create_info, nullptr, &_logical_device);
		if (result != VK_SUCCESS) {
			throw std::runtime_error("Failed to create vulkan logical device.");
		}
        
        vkGetDeviceQueue(_logical_device, _queue_index, 0, &_queue);

        _command_pool = create_command_pool(queue_index());
    }

    VkCommandPool Device::create_command_pool(uint32_t queue_index) {
        VkCommandPoolCreateInfo command_pool_info = {};
        command_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        command_pool_info.queueFamilyIndex = queue_index;
        command_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        VkCommandPool pool;
        VK_CHECK_RESULT(vkCreateCommandPool(_logical_device, &command_pool_info, nullptr, &pool));
        return pool;
    }

    void Device::populate_physical_device_props(VkPhysicalDevice device) {
		// Queue family properties, used for setting up requested queues upon device creation
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(_physical_device, &queueFamilyCount, nullptr);
		if (queueFamilyCount == 0) {
            throw std::runtime_error("Queue family property count was zero.");
        }

		_logicalDeviceQueueFamilyProps.resize(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(_physical_device, &queueFamilyCount, _logicalDeviceQueueFamilyProps.data());

		// Get list of supported extensions
		uint32_t extCount = 0;
		vkEnumerateDeviceExtensionProperties(_physical_device, nullptr, &extCount, nullptr);
		if (extCount > 0) {
			_device_extension_props.resize(extCount);
            VK_CHECK_RESULT(vkEnumerateDeviceExtensionProperties(_physical_device, nullptr, &extCount, &_device_extension_props.front()));
            for (auto&& ext : _device_extension_props) {
                _device_supported_extensions.push_back(ext.extensionName);
            }
        }

	    vkGetPhysicalDeviceMemoryProperties(_physical_device, &_memory_properties);
    }
}