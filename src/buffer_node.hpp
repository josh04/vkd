#pragma once

#include <memory>

namespace vkd {
    class StorageBuffer;
    class AutoMapStagingBuffer;
    class HostBufferNode {
    public:
        virtual ~HostBufferNode() = default;

        virtual std::shared_ptr<AutoMapStagingBuffer> get_output_buffer() const = 0;
    }; 
    class HostImageNode {
    public:
        virtual ~HostImageNode() = default;

        virtual glm::uvec2 get_image_size() const = 0;
    };
    class DeviceBufferNode {
    public:
        virtual ~DeviceBufferNode() = default;

        virtual std::shared_ptr<StorageBuffer> get_output_buffer() const = 0;
    };
}