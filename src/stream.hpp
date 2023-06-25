#pragma once

#include "device.hpp"
#include "semaphore.hpp"

#include "command_buffer.hpp"

namespace vkd {
    class Stream {
    public:
        Stream(const std::shared_ptr<Device>& device) : _device(device) {}
        ~Stream() = default;
        Stream(Stream&&) = delete;
        Stream(Stream&) = delete;

        void init() {
            _semaphore = TimelineSemaphore::make(_device);
        }

        void submit(CommandBufferPtr& buf) {
            submit(*buf);
        }

        void submit(CommandBuffer& buf) {
            if (buf.device() != _device) {
                throw ExecutionException("Command buffer queued on stream with conflicting device.");
            }
            buf.submit_timeline(_semaphore);
        }

        void submit(VkCommandBuffer buf) {
            submit_compute_buffer_timeline(*_device, buf, _semaphore);
        }

        void flush() const {
            _semaphore->wait();
        }

        TimelineSemaphore& semaphore() { return *_semaphore; }
        const TimelineSemaphore& semaphore() const { return *_semaphore; }

    private:
        std::shared_ptr<Device> _device = nullptr;
        std::shared_ptr<TimelineSemaphore> _semaphore = nullptr;
    };
    using StreamPtr = std::shared_ptr<Stream>;
}