#pragma once
#include <memory>
#include "vulkan.hpp"

namespace vkd {
    class Image;
    class Device;
    class Renderpass;
    class Framebuffer {
    public:
        Framebuffer(std::shared_ptr<Device> device, std::shared_ptr<Renderpass> renderpass) : _device(device), _renderpass(renderpass) {}
        ~Framebuffer() = default;
        Framebuffer(Framebuffer&&) = delete;
        Framebuffer(const Framebuffer&) = delete;

        void create(Image& colour, Image& depth, uint32_t width, uint32_t height);
        auto framebuffer() const { return _framebuffer; }
    private:
        std::shared_ptr<Device> _device;
        std::shared_ptr<Renderpass> _renderpass;
        VkFramebuffer _framebuffer;
    };
}