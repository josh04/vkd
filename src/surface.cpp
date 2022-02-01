#include "surface.hpp"
#include "SDL2/SDL_syswm.h"
#include "vulkan.hpp"
#include "device.hpp"
#include "renderdoc_integration.hpp"

#if defined(unix) && !defined(__APPLE__)
#include <X11/Xlib-xcb.h>
#endif

namespace vkd {
    Surface::Surface(std::shared_ptr<Instance> instance) : _instance(instance) {
        ext_vkGetPhysicalDeviceSurfaceSupportKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceSupportKHR>(vkGetInstanceProcAddr(instance->instance(), "vkGetPhysicalDeviceSurfaceSupportKHR"));
        ext_vkGetPhysicalDeviceSurfaceCapabilitiesKHR =  reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR>(vkGetInstanceProcAddr(instance->instance(), "vkGetPhysicalDeviceSurfaceCapabilitiesKHR"));
        ext_vkGetPhysicalDeviceSurfaceFormatsKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceFormatsKHR>(vkGetInstanceProcAddr(instance->instance(), "vkGetPhysicalDeviceSurfaceFormatsKHR"));
        ext_vkGetPhysicalDeviceSurfacePresentModesKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfacePresentModesKHR>(vkGetInstanceProcAddr(instance->instance(), "vkGetPhysicalDeviceSurfacePresentModesKHR"));
    }

    void Surface::init(SDL_Window * window, SDL_Renderer * renderer, const std::shared_ptr<Device>& device) {
        _device = device;
        SDL_SysWMinfo wminfo;
        SDL_VERSION(&wminfo.version);
        if (!SDL_GetWindowWMInfo(window, &wminfo)) {
            throw std::runtime_error("Failed to get SDL window information.");
        }

#if defined(unix) && !defined(__APPLE__)
        VkXcbSurfaceCreateInfoKHR surfaceCreateInfo = {};
        memset(&surfaceCreateInfo, 0, sizeof(VkXcbSurfaceCreateInfoKHR));
        surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
        surfaceCreateInfo.connection = XGetXCBConnection(wminfo.info.x11.display);
        surfaceCreateInfo.window = wminfo.info.x11.window;
        VK_CHECK_RESULT(vkCreateXcbSurfaceKHR(_instance->instance(), &surfaceCreateInfo, nullptr, &_surface));
#elif defined(__APPLE__)
        VkMacOSSurfaceCreateInfoMVK surfaceCreateInfo = {};
        memset(&surfaceCreateInfo, 0, sizeof(VkMacOSSurfaceCreateInfoMVK));
        surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK;
        const void *swapchain = SDL_RenderGetMetalLayer(renderer);
        surfaceCreateInfo.pView = swapchain;
        VK_CHECK_RESULT(vkCreateMacOSSurfaceMVK(_instance->instance(), &surfaceCreateInfo, nullptr, &_surface));
#elif defined(_WIN32)
        // put here
        VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
        memset(&surfaceCreateInfo, 0, sizeof(VkWin32SurfaceCreateInfoKHR));
        surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
                
        HWND platformWindow = wminfo.info.win.window;
        HINSTANCE platformHandle = wminfo.info.win.hinstance;

        surfaceCreateInfo.hinstance = (HINSTANCE)platformHandle;
        surfaceCreateInfo.hwnd = (HWND)platformWindow;
        VK_CHECK_RESULT(vkCreateWin32SurfaceKHR(_instance->instance(), &surfaceCreateInfo, nullptr, &_surface));

        renderdoc_init(_instance->get(), surfaceCreateInfo.hwnd);
#endif


        // Get physical device surface properties and formats
        VK_CHECK_RESULT(ext_vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device->physical_device(), _surface, &_surface_caps));

        // Get available present modes
        uint32_t present_mode_count = 0;
        VK_CHECK_RESULT(ext_vkGetPhysicalDeviceSurfacePresentModesKHR(device->physical_device(), _surface, &present_mode_count, NULL));
        assert(present_mode_count > 0);
        _present_modes.resize(present_mode_count);
        VK_CHECK_RESULT(ext_vkGetPhysicalDeviceSurfacePresentModesKHR(device->physical_device(), _surface, &present_mode_count, _present_modes.data()));

        uint32_t formatCount = 0;
        VK_CHECK_RESULT(ext_vkGetPhysicalDeviceSurfaceFormatsKHR(device->physical_device(), _surface, &formatCount, NULL));

        std::vector<VkSurfaceFormatKHR> surface_format(formatCount);
        VK_CHECK_RESULT(ext_vkGetPhysicalDeviceSurfaceFormatsKHR(device->physical_device(), _surface, &formatCount, surface_format.data()));

        // If the surface format list only includes one entry with VK_FORMAT_UNDEFINED,
        // there is no preferred format, so we assume VK_FORMAT_B8G8R8A8_UNORM
        if ((formatCount == 1) && (surface_format[0].format == VK_FORMAT_UNDEFINED)) {
            _colour_format = VK_FORMAT_B8G8R8A8_UNORM;
            _colour_space = surface_format[0].colorSpace;
        } else {
            // iterate over the list of available surface format and
            // check for the presence of VK_FORMAT_B8G8R8A8_UNORM
            bool found_B8G8R8A8_UNORM = false;
            for (auto&& surface_format : surface_format) {
                if (surface_format.format == VK_FORMAT_B8G8R8A8_UNORM) {
                    _colour_format = surface_format.format;
                    _colour_space = surface_format.colorSpace;
                    found_B8G8R8A8_UNORM = true;
                    break;
                }
            }

            // in case VK_FORMAT_B8G8R8A8_UNORM is not available
            // select the first available color format
            if (!found_B8G8R8A8_UNORM) {
                _colour_format = surface_format[0].format;
                _colour_space = surface_format[0].colorSpace;
            }
        }

    }

    bool Surface::supported(uint32_t queue_family_index) {
        VkBool32 ret = 0;
        VK_CHECK_RESULT(ext_vkGetPhysicalDeviceSurfaceSupportKHR(_device->physical_device(), queue_family_index, _surface, &ret));
        return ret;
    }

}