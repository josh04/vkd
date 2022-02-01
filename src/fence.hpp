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
        Fence(const Fence&) = delete;
        Fence(Fence&&) = delete;

        static std::unique_ptr<Fence> create(const std::shared_ptr<Device>& device, bool signalled);

        auto get() const {
            return _fence;
        }

        enum class State {
            Submitted,
            Waited,
            Reset
        };

        void mark_submit() const { _state = State::Submitted; }

        void pre_submit() const;

        void submit() const;
        void wait() const;
        void reset() const; 

        void submit_wait_reset() const;
        static void auto_wait(const std::shared_ptr<Device>& device);
    private:
        static constexpr int64_t default_timeout = 500000000;

        void _create(const std::shared_ptr<Device>& device, bool signalled);

        std::shared_ptr<Device> _device = nullptr;
        VkFence _fence = VK_NULL_HANDLE;

        mutable State _state = State::Reset;
    };

    using FencePtr = std::unique_ptr<Fence>;
}