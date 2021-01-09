#include <array>
#include "framebuffer.hpp"
#include "image.hpp"
#include "renderpass.hpp"

namespace vulkan {

    void Framebuffer::create(Image& colour, Image& depth, uint32_t width, uint32_t height) {
        std::array<VkImageView, 2> attachments;
        
        attachments[0] = colour.view();
        attachments[1] = depth.view();

        VkFramebufferCreateInfo framebuffer_create_info = {};
        framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_create_info.pNext = NULL;
        framebuffer_create_info.renderPass = _renderpass->get();
        framebuffer_create_info.attachmentCount = 2;
        framebuffer_create_info.pAttachments = attachments.data();
        framebuffer_create_info.width = width;
        framebuffer_create_info.height = height;
        framebuffer_create_info.layers = 1;

        VK_CHECK_RESULT(vkCreateFramebuffer(_device->logical_device(), &framebuffer_create_info, nullptr, &_framebuffer));
    }
}

