#include <iostream>
#include "instance.hpp"
#include "vulkan.hpp"
#include <algorithm>

namespace vulkan {
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

#ifdef __APPLE__
        _enabled_instance_extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#endif

        // Enabled requested instance extensions
        if (_enabled_instance_extensions.size() > 0) 
        {
            for (auto&& enabledExtension : _enabled_instance_extensions) 
            {
                // Output message if requested extension is not available
                if (std::find(_supported_instance_extensions.begin(), _supported_instance_extensions.end(), enabledExtension) == _supported_instance_extensions.end())
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
                std::cerr << "Validation layer VK_LAYER_KHRONOS_validation not present, validation is disabled" << std::endl;;
                _validation = false;
            }
        }

        if (VkResult res = vkCreateInstance(&instanceCreateInfo, nullptr, &_instance)) {
            throw std::runtime_error(std::string("Failed to create vulkan instance. err: ") + std::to_string(res));
        }
    }
}