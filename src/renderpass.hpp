#pragma once
#include <array>

#include "vulkan/vulkan.h"
#include "device.hpp"
#include "vulkan.hpp"

namespace vkd {
    class Renderpass {
    public:
        Renderpass(std::shared_ptr<Device> device) : _device(device) {}
        ~Renderpass();
        Renderpass(Renderpass&&) = delete;
        Renderpass(const Renderpass&) = delete;

        void create(const VkFormat colour_format, const VkFormat& depth_format);
        void begin(VkCommandBuffer buf, VkFramebuffer framebuffer, uint32_t width, uint32_t height);
        void end(VkCommandBuffer buf);

        auto get() const { return _renderpass; }
    private:
        std::shared_ptr<Device> _device = nullptr;
        VkRenderPass _renderpass;
    };

}