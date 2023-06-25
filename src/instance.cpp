#include <iostream>
#include "instance.hpp"
#include "vulkan.hpp"
#include <algorithm>

namespace vkd {

    Instance::~Instance()
    {
        if (ext_vkDestroyDebugUtilsMessengerEXT != nullptr && _debug_reporter) {
            ext_vkDestroyDebugUtilsMessengerEXT(_instance, _debug_reporter, nullptr);
        }

        vkDestroyInstance(_instance, nullptr);
    }

    void Instance::init(bool validation) {
        _validation = validation;

        std::string name = "VkDemo";

        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = name.c_str();
        appInfo.pEngineName = name.c_str();
        appInfo.apiVersion = api_version;

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
                    _supported_instance_extensions.push_back(extension.extensionName);
                }
            }
        }

//#ifdef __APPLE__
        _enabled_instance_extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
//#endif

        // Enabled requested instance extensions
        if (_enabled_instance_extensions.size() > 0) 
        {
            for (auto&& enabledExtension : _enabled_instance_extensions) 
            {
                // Output message if requested extension is not available
                if (std::find(_supported_instance_extensions.begin(), _supported_instance_extensions.end(), enabledExtension) == _supported_instance_extensions.end())
                {
                    console << "Enabled instance extension \"" << enabledExtension << "\" is not present at instance level" << std::endl;
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
            if (_validation)
            {
                instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            }
            instanceCreateInfo.enabledExtensionCount = (uint32_t)instanceExtensions.size();
            instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();
        }

        uint32_t instanceLayerCount;
        vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);
        _instance_layer_properties.resize(instanceLayerCount);
        vkEnumerateInstanceLayerProperties(&instanceLayerCount, _instance_layer_properties.data());

        std::vector<const char *> arr;
        std::string validationLayerName = "VK_LAYER_KHRONOS_validation";
        if (_validation)
        {
            // The VK_LAYER_KHRONOS_validation contains all current validation functionality.
            // Note that on Android this layer requires at least NDK r20
            // Check if this layer is available at instance level
            bool validationLayerPresent = false;
            for (VkLayerProperties layer : _instance_layer_properties) {
                if (validationLayerName == layer.layerName) {
                    validationLayerPresent = true;
                    break;
                }
            }
            if (validationLayerPresent) {
                arr = {validationLayerName.c_str()};
                instanceCreateInfo.ppEnabledLayerNames = arr.data();
                instanceCreateInfo.enabledLayerCount = 1;
            } else {
                console << "Validation layer VK_LAYER_KHRONOS_validation not present, validation is disabled" << std::endl;;
                _validation = false;
            }
        }

        if (VkResult res = vkCreateInstance(&instanceCreateInfo, nullptr, &_instance)) {
            throw std::runtime_error(std::string("Failed to create vulkan instance. err: ") + std::to_string(res));
        }


        // Physical device
        uint32_t gpuCount = 0;
        // Get number of available physical devices
        VK_CHECK_RESULT(vkEnumeratePhysicalDevices(_instance, &gpuCount, nullptr));
        assert(gpuCount > 0);
        // Enumerate devices
        _physical_devices.resize(gpuCount);
        VkResult err = vkEnumeratePhysicalDevices(_instance, &gpuCount, _physical_devices.data());
        if (err) {
            throw std::runtime_error(std::string("Could not enumerate physical devices: \n") + vkd::error_string(err));
        }

        // GPU selection

        // Select physical device to be used for the Vulkan example
        // Defaults to the first device unless specified by command line
        uint32_t selectedDevice = 0;

        _physical_device_props.resize(gpuCount);
        _physical_device_feats.resize(gpuCount);
        _physical_device_mem_props.resize(gpuCount);
        int i = 0;
        for (auto&& device : _physical_devices) {
			vkGetPhysicalDeviceProperties(device, &_physical_device_props[i]);
	        vkGetPhysicalDeviceFeatures(device, &_physical_device_feats[i]);
	        vkGetPhysicalDeviceMemoryProperties(device, &_physical_device_mem_props[i]);
            i++;
        }

        if (_validation) {
            VkDebugUtilsMessengerCreateInfoEXT debug_messenger_info{};
            debug_messenger_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            debug_messenger_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            debug_messenger_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            debug_messenger_info.pfnUserCallback = &Instance::debug_report;
            debug_messenger_info.pUserData = nullptr; // Optional

            ext_vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(_instance, "vkCreateDebugUtilsMessengerEXT");
            ext_vkSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(_instance, "vkSetDebugUtilsObjectNameEXT");
            ext_vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(_instance, "vkDestroyDebugUtilsMessengerEXT");
            if (ext_vkCreateDebugUtilsMessengerEXT != nullptr) {
                VkResult ret = ext_vkCreateDebugUtilsMessengerEXT(_instance, &debug_messenger_info, nullptr, &_debug_reporter);
                if (ret) {
                    throw std::runtime_error(std::string("Failed to enable debug layer handler\n"));
                }
            } else {
                console << "Failed to enable custom debug layer reporter with vkCreateDebugUtilsMessengerEXT\n";
            }
        }
    }
    
    VkResult Instance::set_debug_utils_object_name_with_device(VkDevice device, const VkDebugUtilsObjectNameInfoEXT * pNameInfo) const { 
        if (ext_vkSetDebugUtilsObjectNameEXT) {
            return ext_vkSetDebugUtilsObjectNameEXT(device, pNameInfo); 
        } else { 
            return VK_SUCCESS; 
        } 
    }
    
    VKAPI_ATTR VkBool32 VKAPI_CALL Instance::debug_report(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) {
        
        console << "validation layer: " << pCallbackData->pMessage << std::endl;

        return VK_FALSE;
    }
}