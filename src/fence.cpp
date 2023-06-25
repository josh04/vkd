#include "fence.hpp"
#include "device.hpp"
#include "command_buffer.hpp"

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

    void Fence::submit() const {
        submit_compute_buffer(*_device, VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE, this);
    }

    void Fence::pre_submit() const {
        if (_state == State::Submitted) {
            wait();
        }
        if (_state == State::Waited) {
            reset();
        }
    }

    void Fence::wait() const {
        if (_state == State::Waited) {
            //console << "Warning: double fence wait" << std::endl;
            return;
        } else if (_state != State::Submitted) {
            throw std::runtime_error("Attempting to wait on fence that has not been submitted.");
        }
        double wait_time = 0.5;
        VkResult res = VK_SUCCESS;
        for (int i = 0; i < 20; ++i) {
            res = vkWaitForFences(_device->logical_device(), 1, &_fence, VK_TRUE, default_timeout);
            VK_CHECK_RESULT_TIMEOUT(res);
            if (res == VK_SUCCESS) {
                break;
            }
            console << "Slow: Timeout waiting on Fence (" << wait_time * (i + 1) << ")." << std::endl;
        }
        if (res == VK_TIMEOUT) {
            console << "Slow: Fence wait failed after 10s, issues likely inbound." << std::endl;
        }
        _state = State::Waited;
    }

    void Fence::reset() const {
        if (_state == State::Submitted) {
            wait();
        } else if (_state == State::Reset) {
            return;
        }
        VK_CHECK_RESULT(vkResetFences(_device->logical_device(), 1, &_fence));
        _state = State::Reset;
    }

    void Fence::submit_wait_reset() const {
        submit();
        wait();
        reset();
    }

    void Fence::auto_wait(const std::shared_ptr<Device>& device) {
        auto fence = Fence::create(device, false);
        fence->submit_wait_reset();
    }

    void Fence::_create(const std::shared_ptr<Device>& device, bool signalled) {
        _device = device;
        _fence = create_fence(_device->logical_device(), signalled);
        if (signalled) {
            _state = State::Submitted;
        } else {
            _state = State::Reset;
        }
    }
}