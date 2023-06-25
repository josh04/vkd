#include <stdexcept>
#include "swapchain.hpp"
#include "instance.hpp"
#include "device.hpp"
#include "image.hpp"
#include "vulkan_enum.hpp"

#include <sstream>
#include "imgui/imgui.h"

namespace vkd {

    swapchain::swapchain(std::shared_ptr<Instance> instance, 
                  std::shared_ptr<Device> device,
                  std::shared_ptr<Surface> surface)
                  : _instance(instance), _device(device), _surface(surface) {
        
        ext_vkCreateSwapchainKHR = reinterpret_cast<PFN_vkCreateSwapchainKHR>(vkGetDeviceProcAddr(_device->logical_device(), "vkCreateSwapchainKHR"));
        ext_vkDestroySwapchainKHR = reinterpret_cast<PFN_vkDestroySwapchainKHR>(vkGetDeviceProcAddr(_device->logical_device(), "vkDestroySwapchainKHR"));
        ext_vkGetSwapchainImagesKHR = reinterpret_cast<PFN_vkGetSwapchainImagesKHR>(vkGetDeviceProcAddr(_device->logical_device(), "vkGetSwapchainImagesKHR"));
        ext_vkAcquireNextImageKHR = reinterpret_cast<PFN_vkAcquireNextImageKHR>(vkGetDeviceProcAddr(_device->logical_device(), "vkAcquireNextImageKHR"));
        ext_vkQueuePresentKHR = reinterpret_cast<PFN_vkQueuePresentKHR>(vkGetDeviceProcAddr(_device->logical_device(), "vkQueuePresentKHR"));

        if (!ext_vkCreateSwapchainKHR
            || !ext_vkDestroySwapchainKHR
            || !ext_vkGetSwapchainImagesKHR
            || !ext_vkAcquireNextImageKHR
            || !ext_vkQueuePresentKHR) {
            throw std::runtime_error("Failed to acquire swapchain function pointer.");
        }
    }

    swapchain::~swapchain() {
        for (auto&& image : _images) {
            vkDestroyImageView(_device->logical_device(), image->view(), nullptr);
        }
        if (_swapchain != VK_NULL_HANDLE) {
            ext_vkDestroySwapchainKHR(_device->logical_device(), _swapchain, nullptr);
        }
    }

    uint32_t swapchain::next_image(VkSemaphore present_complete) const {
        uint32_t index = -1;
    	VK_CHECK_RESULT(ext_vkAcquireNextImageKHR(_device->logical_device(), _swapchain, UINT64_MAX, present_complete, (VkFence)nullptr, &index));
        return index;
    }

    void swapchain::present(VkSemaphore render_complete, uint32_t index) const {
        VkPresentInfoKHR present_info = {};
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present_info.pNext = NULL;
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &_swapchain;
        present_info.pImageIndices = &index;

        // Check if a wait semaphore has been specified to wait for before presenting the image
        if (render_complete != VK_NULL_HANDLE) {
            present_info.pWaitSemaphores = &render_complete;
            present_info.waitSemaphoreCount = 1;
        }
        
        VK_CHECK_RESULT(ext_vkQueuePresentKHR(_device->queue(), &present_info));
    }

    void swapchain::create(uint32_t& width, uint32_t& height, bool vsync) {
        VkSwapchainKHR oldchain = _swapchain;

        auto surface_caps = _surface->surface_caps();

        VkExtent2D swapchain_extent = {};
        // If width (and height) equals the special value 0xFFFFFFFF, the size of the surface will be set by the swapchain
        if (surface_caps.currentExtent.width == (uint32_t)-1)  {
            // If the surface size is undefined, the size is set to
            // the size of the images requested.
            swapchain_extent.width = width;
            swapchain_extent.height = height;
        } else {
            // If the surface size is defined, the swap chain size must match
            swapchain_extent = surface_caps.currentExtent;
            width = surface_caps.currentExtent.width;
            height = surface_caps.currentExtent.height;
        }


        // Select a present mode for the swapchain

        // The VK_PRESENT_MODE_FIFO_KHR mode must always be present as per spec
        // This mode waits for the vertical blank ("v-sync")
        VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;

        auto&& present_modes = _surface->present_modes();

        // If v-sync is not requested, try to find a mailbox mode
        // It's the lowest latency non-tearing present mode available
        if (!vsync) {
            for (size_t i = 0; i < present_modes.size(); i++) {
                if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
                    present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
                    break;
                }
                if ((present_mode != VK_PRESENT_MODE_MAILBOX_KHR) && (present_modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)) {
                    present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
                }
            }
        }

        // Determine the number of images
        uint32_t target_num_images = surface_caps.minImageCount + 1;
        if ((surface_caps.maxImageCount > 0) && (target_num_images > surface_caps.maxImageCount)) {
            target_num_images = surface_caps.maxImageCount;
        }

        // Find the transformation of the surface
        VkSurfaceTransformFlagsKHR preTransform;
        if (surface_caps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
            // We prefer a non-rotated transform
            preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        } else {
            preTransform = surface_caps.currentTransform;
        }

        // Find a supported composite alpha format (not all devices support alpha opaque)
        VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        // Simply select the first composite alpha format available
        std::vector<VkCompositeAlphaFlagBitsKHR> compositeAlphaFlags = {
            VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
            VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
            VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
        };

        for (auto&& compositeAlphaFlag : compositeAlphaFlags) {
            if (surface_caps.supportedCompositeAlpha & compositeAlphaFlag) {
                compositeAlpha = compositeAlphaFlag;
                break;
            };
        }

        if (!_surface->supported(0)) {
            throw std::runtime_error("Queue does not support surface.");
        }

        VkSwapchainCreateInfoKHR swapchain_create_info = {};
        swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchain_create_info.pNext = NULL;
        swapchain_create_info.surface = _surface->get();
        swapchain_create_info.minImageCount = target_num_images;
        swapchain_create_info.imageFormat = _surface->colour_format();
        swapchain_create_info.imageColorSpace = _surface->colour_space();
        swapchain_create_info.imageExtent = { swapchain_extent.width, swapchain_extent.height };
        swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        swapchain_create_info.preTransform = (VkSurfaceTransformFlagBitsKHR)preTransform;
        swapchain_create_info.imageArrayLayers = 1;
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchain_create_info.queueFamilyIndexCount = 0;
        swapchain_create_info.pQueueFamilyIndices = NULL;
        swapchain_create_info.presentMode = present_mode;
        swapchain_create_info.oldSwapchain = oldchain;
        // Setting clipped to VK_TRUE allows the implementation to discard rendering outside of the surface area
        swapchain_create_info.clipped = VK_TRUE;
        swapchain_create_info.compositeAlpha = compositeAlpha;

        // Enable transfer source on swap chain images if supported
        if (surface_caps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
            swapchain_create_info.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        }

        // Enable transfer destination on swap chain images if supported
        if (surface_caps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
            swapchain_create_info.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        }

        VK_CHECK_RESULT(ext_vkCreateSwapchainKHR(_device->logical_device(), &swapchain_create_info, nullptr, &_swapchain));

        // If an existing swap chain is re-created, destroy the old swap chain
        // This also cleans up all the presentable images
        if (oldchain != VK_NULL_HANDLE) { 
            for (auto&& image : _images) {
                vkDestroyImageView(_device->logical_device(), image->view(), nullptr);
            }
            ext_vkDestroySwapchainKHR(_device->logical_device(), oldchain, nullptr);
        }
        uint32_t image_count = 0;
        VK_CHECK_RESULT(ext_vkGetSwapchainImagesKHR(_device->logical_device(), _swapchain, &image_count, NULL));

        // Get the swap chain images
        std::vector<VkImage> images(image_count);
        VK_CHECK_RESULT(ext_vkGetSwapchainImagesKHR(_device->logical_device(), _swapchain, &image_count, images.data()));

        // Get the swap chain buffers containing the image and imageview
        _images.clear();
        _images.resize(image_count);
        int i = 0;
        for (auto&& image : images) {
            VkImageView view;
            VkImageViewCreateInfo colorAttachmentView = {};
            colorAttachmentView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            colorAttachmentView.pNext = NULL;
            colorAttachmentView.format = _surface->colour_format();
            colorAttachmentView.components = {
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY
            };
            colorAttachmentView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            colorAttachmentView.subresourceRange.baseMipLevel = 0;
            colorAttachmentView.subresourceRange.levelCount = 1;
            colorAttachmentView.subresourceRange.baseArrayLayer = 0;
            colorAttachmentView.subresourceRange.layerCount = 1;
            colorAttachmentView.viewType = VK_IMAGE_VIEW_TYPE_2D;
            colorAttachmentView.flags = 0;

            colorAttachmentView.image = image;

            VK_CHECK_RESULT(vkCreateImageView(_device->logical_device(), &colorAttachmentView, nullptr, &view));
            _images[i] = std::make_shared<Image>(_device, image, view);
            ++i;

        }
        
    }

    void swapchain::ui() {

        if (ImGui::TreeNode("Swapchain")) {
            std::stringstream strm;

            strm << "Images: " << _images.size() << "\n";
            strm << "Colour format: " << format_to_string(_surface->colour_format()) << "\n";
            strm << "Colour space: " << _surface->colour_space() << "\n";

            ImGui::Text("%s", strm.str().c_str());

            ImGui::TreePop();
        }
    }
}