#pragma once
#include <memory>
#include "vulkan.hpp"
#include "surface.hpp"

namespace vkd {
    class Instance;
    class Device;
    class Image;
    class swapchain {
    public:
        swapchain(std::shared_ptr<Instance> instance, 
                  std::shared_ptr<Device> device,
                  std::shared_ptr<Surface> surface);
        ~swapchain();

        void create(uint32_t& width, uint32_t& height, bool vsync);

        auto count() const { return _images.size(); }
        Image& image(int i) { return *_images[i]; }

        uint32_t next_image(VkSemaphore present_complete) const;

        void present(VkSemaphore render_complete, uint32_t index) const;

        void ui();
    private:
        VkSwapchainKHR _swapchain = VK_NULL_HANDLE;

        std::shared_ptr<Instance> _instance = nullptr ;
        std::shared_ptr<Device> _device = nullptr;
        std::shared_ptr<Surface> _surface = nullptr;

        std::vector<std::shared_ptr<Image>> _images;
        //std::vector<VkImage> _images;
        //std::vector<VkImageView> _views;

        // Vulkan logical device funcs
        PFN_vkCreateSwapchainKHR ext_vkCreateSwapchainKHR;
        PFN_vkDestroySwapchainKHR ext_vkDestroySwapchainKHR;
        PFN_vkGetSwapchainImagesKHR ext_vkGetSwapchainImagesKHR;
        PFN_vkAcquireNextImageKHR ext_vkAcquireNextImageKHR;
        PFN_vkQueuePresentKHR ext_vkQueuePresentKHR;
    };
    
}