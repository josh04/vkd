#include "fence.hpp"
#include "device.hpp"

namespace vkd {
    VkFence create_fence(VkDevice logical_device, bool signalled) {
        VkFenceCreateInfo fence_create_info{};
        memset(&fence_create_info, 0, sizeof(VkFenceCreateInfo));
        fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_create_info.flags = (signalled ? VK_FENCE_CREATE_SIGNALED_BIT : 0);
            
        VkFence fence;
        VK_CHECK_RESULT(vkCreateFence(logical_device, &fence_create_info, nullptr, &fence));
        return fence;
    }

    Fence::~Fence() {
        if (_fence != VK_NULL_HANDLE) {
            vkDestroyFence(_device->logical_device(), _fence, nullptr);
        }
    }

    std::unique_ptr<Fence> Fence::create(const std::shared_ptr<Device>& device, bool signalled) {
        auto fence = std::make_unique<Fence>();
        fence->_create(device, signalled);
        return std::move(fence);
    }

    void Fence::wait() const {
        VK_CHECK_RESULT_TIMEOUT(vkWaitForFences(_device->logical_device(), 1, &_fence, VK_TRUE, default_timeout));
    }

    void Fence::reset() const {
        VK_CHECK_RESULT(vkResetFences(_device->logical_device(), 1, &_fence));
    }

    void Fence::_create(const std::shared_ptr<Device>& device, bool signalled) {
        _device = device;
        _fence = create_fence(_device->logical_device(), signalled);
    }
}