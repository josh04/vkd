#pragma once
#include "vulkan.hpp"
#include "semaphore.hpp"

namespace vkd {
    class Device;
    VkFence create_fence(VkDevice logical_device, bool signalled);

    class Fence {
    public:
        Fence() = default;
        ~Fence();

        static std::unique_ptr<Fence> create(const std::shared_ptr<Device>& device, bool signalled);

        auto get() const {
            return _fence;
        }

        void wait() const;
        void reset() const; 
    private:
        static constexpr int64_t default_timeout = 500000000;

        void _create(const std::shared_ptr<Device>& device, bool signalled);

        std::shared_ptr<Device> _device = nullptr;;
        VkFence _fence = VK_NULL_HANDLE;
    };

    using FencePtr = std::unique_ptr<Fence>;
}