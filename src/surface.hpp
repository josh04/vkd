#pragma once
#include <memory>
#include "vulkan/vulkan.h"
#include "instance.hpp"
#include "SDL2/SDL.h"

namespace vkd {
    class Device;
    class Surface {
    public:
        Surface(std::shared_ptr<Instance> instance);
        ~Surface() = default;
        Surface(Surface&&) = delete;
        Surface(const Surface&) = delete;

        void init(SDL_Window * window, SDL_Renderer * renderer, const std::shared_ptr<Device>& device);

        VkSurfaceKHR get() { return _surface; }
        auto colour_format() const { return _colour_format; }
        auto colour_space() const { return _colour_space; }
        auto surface_caps() const { return _surface_caps; }
        const auto& present_modes() const { return _present_modes; }

        bool supported(uint32_t queue_family_index);
    private:
        std::shared_ptr<Instance> _instance = nullptr;
        std::shared_ptr<Device> _device = nullptr;
        VkSurfaceKHR _surface;
        VkFormat _colour_format;
        VkColorSpaceKHR _colour_space;

        VkSurfaceCapabilitiesKHR _surface_caps = {0};
        std::vector<VkPresentModeKHR> _present_modes;

        // Vulkan instance funcs
        PFN_vkGetPhysicalDeviceSurfaceSupportKHR ext_vkGetPhysicalDeviceSurfaceSupportKHR;
        PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR ext_vkGetPhysicalDeviceSurfaceCapabilitiesKHR; 
        PFN_vkGetPhysicalDeviceSurfaceFormatsKHR ext_vkGetPhysicalDeviceSurfaceFormatsKHR;
        PFN_vkGetPhysicalDeviceSurfacePresentModesKHR ext_vkGetPhysicalDeviceSurfacePresentModesKHR;

    };
}