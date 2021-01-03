#include "vulkan.hpp"

#include "imgui/imgui.h"

#include "vulkan/vulkan.h"

#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <mutex>

namespace vulkan {

    namespace {
        bool _enableValidation = false;
	    constexpr uint32_t apiVersion = VK_API_VERSION_1_0;

	    std::vector<std::string> _supportedInstanceExtensions;
	    std::vector<std::string> _enabledInstanceExtensions;
        std::vector<VkLayerProperties> _instanceLayerProperties;        
        std::vector<VkPhysicalDevice> _physicalDevices;
        std::vector<VkPhysicalDeviceProperties> _physicalDeviceProps;
        std::vector<VkPhysicalDeviceFeatures> _physicalDeviceFeats;
        std::vector<VkPhysicalDeviceMemoryProperties> _physicalDeviceMemProps;

        std::vector<VkQueueFamilyProperties> _logicalDeviceQueueFamilyProps;
        std::vector<VkExtensionProperties> _logicalDeviceExtensionProps;

        VkInstance _instance;
    }

    VkResult createInstance(bool enableValidation) {
        _enableValidation = enableValidation;

        std::string name = "VkDemo";

        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = name.c_str();
        appInfo.pEngineName = name.c_str();
        appInfo.apiVersion = apiVersion;

        std::vector<const char*> instanceExtensions = { VK_KHR_SURFACE_EXTENSION_NAME };

        // Enable surface extensions depending on os
    #if defined(_WIN32)
        instanceExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
    #elif defined(VK_USE_PLATFORM_ANDROID_KHR)
        instanceExtensions.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
    #elif defined(_DIRECT2DISPLAY)
        instanceExtensions.push_back(VK_KHR_DISPLAY_EXTENSION_NAME);
    #elif defined(VK_USE_PLATFORM_DIRECTFB_EXT)
        instanceExtensions.push_back(VK_EXT_DIRECTFB_SURFACE_EXTENSION_NAME);
    #elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
        instanceExtensions.push_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
    #elif defined(VK_USE_PLATFORM_XCB_KHR)
        instanceExtensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
    #elif defined(VK_USE_PLATFORM_IOS_MVK)
        instanceExtensions.push_back(VK_MVK_IOS_SURFACE_EXTENSION_NAME);
    #elif defined(VK_USE_PLATFORM_MACOS_MVK)
        instanceExtensions.push_back(VK_MVK_MACOS_SURFACE_EXTENSION_NAME);
    #endif

        // Get extensions supported by the instance and store for later use
        uint32_t extCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
        if (extCount > 0)
        {
            std::vector<VkExtensionProperties> extensions(extCount);
            if (vkEnumerateInstanceExtensionProperties(nullptr, &extCount, &extensions.front()) == VK_SUCCESS)
            {
                for (VkExtensionProperties extension : extensions)
                {
                    _supportedInstanceExtensions.push_back(extension.extensionName);
                }
            }
        }

        // Enabled requested instance extensions
        if (_enabledInstanceExtensions.size() > 0) 
        {
            for (auto&& enabledExtension : _enabledInstanceExtensions) 
            {
                // Output message if requested extension is not available
                if (std::find(_supportedInstanceExtensions.begin(), _supportedInstanceExtensions.end(), enabledExtension) == _supportedInstanceExtensions.end())
                {
                    std::cerr << "Enabled instance extension \"" << enabledExtension << "\" is not present at instance level" << std::endl;
                }
                instanceExtensions.push_back(enabledExtension.c_str());
            }
        }

        VkInstanceCreateInfo instanceCreateInfo = {};
        instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceCreateInfo.pNext = NULL;
        instanceCreateInfo.pApplicationInfo = &appInfo;
        if (instanceExtensions.size() > 0)
        {
            if (_enableValidation)
            {
                instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            }
            instanceCreateInfo.enabledExtensionCount = (uint32_t)instanceExtensions.size();
            instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();
        }

        uint32_t instanceLayerCount;
        vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);
        _instanceLayerProperties.resize(instanceLayerCount);
        vkEnumerateInstanceLayerProperties(&instanceLayerCount, _instanceLayerProperties.data());

        if (_enableValidation)
        {
            // The VK_LAYER_KHRONOS_validation contains all current validation functionality.
            // Note that on Android this layer requires at least NDK r20
            std::string validationLayerName = "VK_LAYER_KHRONOS_validation";
            // Check if this layer is available at instance level
            bool validationLayerPresent = false;
            for (VkLayerProperties layer : _instanceLayerProperties) {
                if (validationLayerName == layer.layerName) {
                    validationLayerPresent = true;
                    break;
                }
            }
            if (validationLayerPresent) {
                std::vector<const char *> arr = {validationLayerName.c_str()};
                instanceCreateInfo.ppEnabledLayerNames = arr.data();
                instanceCreateInfo.enabledLayerCount = 1;
            } else {
                std::cerr << "Validation layer VK_LAYER_KHRONOS_validation not present, validation is disabled";
                _enableValidation = false;
            }
        }

        if (vkCreateInstance(&instanceCreateInfo, nullptr, &_instance) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create vulkan instance");
        }

        // If requested, we enable the default validation layers for debugging
        if (_enableValidation)
        {
            // The report flags determine what type of messages for the layers will be displayed
            // For validating (debugging) an application the error and warning bits should suffice
            VkDebugReportFlagsEXT debugReportFlags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
            // Additional flags include performance info, loader and layer debug messages, etc.
            //vks::debug::setupDebugging(_instance, debugReportFlags, VK_NULL_HANDLE);
        }

        // Physical device
        uint32_t gpuCount = 0;
        // Get number of available physical devices
        VK_CHECK_RESULT(vkEnumeratePhysicalDevices(_instance, &gpuCount, nullptr));
        assert(gpuCount > 0);
        // Enumerate devices
        _physicalDevices.resize(gpuCount);
        VkResult err = vkEnumeratePhysicalDevices(_instance, &gpuCount, _physicalDevices.data());
        if (err) {
            throw std::runtime_error(std::string("Could not enumerate physical devices: \n") + vulkan::error_string(err));
        }

        // GPU selection

        // Select physical device to be used for the Vulkan example
        // Defaults to the first device unless specified by command line
        uint32_t selectedDevice = 0;

        _physicalDeviceProps.resize(gpuCount);
        _physicalDeviceFeats.resize(gpuCount);
        _physicalDeviceMemProps.resize(gpuCount);
        int i = 0;
        for (auto&& device : _physicalDevices) {
			vkGetPhysicalDeviceProperties(device, &_physicalDeviceProps[i]);
	        vkGetPhysicalDeviceFeatures(device, &_physicalDeviceFeats[i]);
	        vkGetPhysicalDeviceMemoryProperties(device, &_physicalDeviceMemProps[i]);
            i++;
        }

        return VK_SUCCESS;
    }

    void getLogicalDeviceProps(VkPhysicalDevice device) {
		// Queue family properties, used for setting up requested queues upon device creation
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
		if (queueFamilyCount == 0) {
            throw std::runtime_error("Queue family property count was zero.");
        }
		_logicalDeviceQueueFamilyProps.resize(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, _logicalDeviceQueueFamilyProps.data());

		// Get list of supported extensions
		uint32_t extCount = 0;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extCount, nullptr);
		if (extCount > 0) {
			_logicalDeviceExtensionProps.resize(extCount);
            VK_CHECK_RESULT(vkEnumerateDeviceExtensionProperties(device, nullptr, &extCount, &_logicalDeviceExtensionProps.front()));
		}
    }

    void ui() {
        static std::once_flag doOnce;
        std::call_once(doOnce, [&]() {
            createInstance(true);
            getLogicalDeviceProps(_physicalDevices[0]);
        });

        ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_Once );
        ImGui::SetNextWindowSize(ImVec2(400, 500), ImGuiCond_Once );
        ImGui::Begin("Vulkan");


        if (ImGui::TreeNode("Supported Extensions")) {
            std::stringstream strm;
            for (auto&& ext : _supportedInstanceExtensions) {
                strm << ext << "\n";
            }
            ImGui::Text("%s", strm.str().c_str());
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Enabled Extensions")) {
            std::stringstream strm;
            for (auto&& ext : _enabledInstanceExtensions) {
                strm << ext << "\n";
            }
            ImGui::Text("%s", strm.str().c_str());
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Instance Layers")) {
            std::stringstream strm;
            for (auto&& layer : _instanceLayerProperties) {
                strm << layer.layerName << ": " << layer.description << "\n";
            }
            ImGui::Text("%s", strm.str().c_str());
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Physical Devices")) {
            int i = 0;
            for (auto&& device : _physicalDeviceProps) {
                std::stringstream strm;
                auto&& feats = _physicalDeviceFeats[i];
                auto&& memProps = _physicalDeviceMemProps[i];
                strm << device.deviceName 
                << " - Type: " << physical_device_to_string(device.deviceType)
                << " - API: " << (device.apiVersion >> 22) << "." << ((device.apiVersion >> 12) & 0x3ff) << "." << (device.apiVersion & 0xfff) << "\n";

                ImGui::Text("%s", strm.str().c_str());

                if (ImGui::TreeNode("Feats")) {
                    ImGui::Text("%s", physical_device_features_string(feats).c_str());
                    ImGui::TreePop();
                }

                std::stringstream strm2;
                strm2 << "Heaps: " << memProps.memoryHeapCount << " Types: " << memProps.memoryTypeCount << "\n";
                for (int j = 0; j < memProps.memoryHeapCount; ++j) {
                    strm2 << "Heap " << j << " - size: " << memProps.memoryHeaps[j].size 
                        << "\n flags: \n" << memory_heap_flags_to_string(memProps.memoryHeaps[j].flags);
                }

                for (int j = 0; j < memProps.memoryTypeCount; ++j) {
                    strm2 << "Type " << j 
                        << " - heap index: " << memProps.memoryTypes[j].heapIndex 
                        << "\n flags: \n" << memory_property_flags_to_string(memProps.memoryTypes[j].propertyFlags);
                }
                ImGui::Text("%s", strm2.str().c_str());
                
                i++;
            }

            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Logical Device Props")) {
            
            int i = 0;
            for (auto&& device : _physicalDevices) {
                std::stringstream strm;
                getLogicalDeviceProps(device);

                strm << "Device " << i << "\n\t Queue count: " 
                    << _logicalDeviceQueueFamilyProps[i].queueCount
                    << "\n\t minImageTransferGranularity: " << _logicalDeviceQueueFamilyProps[i].minImageTransferGranularity.width 
                    << "/" << _logicalDeviceQueueFamilyProps[i].minImageTransferGranularity.height << "/" << _logicalDeviceQueueFamilyProps[i].minImageTransferGranularity.depth
                    << "\n\t timestampValidBits: " << _logicalDeviceQueueFamilyProps[i].timestampValidBits
                    << "\n\t queueFlags: \n" << queue_flags_to_string(_logicalDeviceQueueFamilyProps[i].queueFlags) << "\n";

                if (ImGui::TreeNode("Extensions")) {
                    for (auto&& ext : _logicalDeviceExtensionProps) {
                        strm << ext.extensionName << ": " << ext.specVersion << "\n";
                    }
                    ImGui::TreePop();
                }

                ImGui::Text("%s", strm.str().c_str());
                i++;
            }


            ImGui::TreePop();
        }




        ImGui::End();
    }
}