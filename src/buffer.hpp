#pragma once
#include <memory>
#include <vector>
#include "vulkan.hpp"

namespace vkd {
    class Device;
    class Image;
    class Buffer {
    public:
        Buffer(std::shared_ptr<Device> device) : _device(device) {}
        virtual ~Buffer();
        Buffer(Buffer&&) = delete;
        Buffer(const Buffer&) = delete;

        void copy(Buffer& src, size_t sz, VkCommandBuffer buf);
        void copy(VkCommandBuffer buf, Image& src, int offset, int xStart, int yStart, uint32_t width, uint32_t height);

        void stage(const std::vector<std::pair<void *, size_t>>& data);

        VkBufferMemoryBarrier barrier_info(VkPipelineStageFlags src_stage_mask, VkPipelineStageFlags dst_stage_mask);
        void barrier(VkCommandBuffer buf,  VkPipelineStageFlags src_stage_mask, VkPipelineStageFlags dst_stage_mask);

        auto buffer() { return _buffer; }
        auto get() { return _buffer; }
        auto memory() { return _memory; }
        auto size() { return _size; }
        auto requested_size() { return _requested_size; }
        auto& descriptor() { return _descriptor; }

        void init(size_t size, VkBufferUsageFlags flags, VkMemoryPropertyFlags mem_prop_flags);
        void allocate();
        void deallocate();

        void _create(size_t size, VkBufferUsageFlags flags, VkMemoryPropertyFlags mem_prop_flags);

        void debug_name(const std::string& debugName) { _debug_name = debugName; update_debug_name(); }
    protected:
        void update_debug_name();
        std::string _debug_name = "Anonymous Buffer";
        std::shared_ptr<Device> _device;

        size_t _size = 0;
        size_t _requested_size = 0;
        VkBuffer _buffer = VK_NULL_HANDLE;
        VkDeviceMemory _memory = VK_NULL_HANDLE;
		VkDescriptorBufferInfo _descriptor;
        VkMemoryPropertyFlags _memory_flags = 0;
        VkBufferUsageFlags _buffer_usage_flags = 0;

        bool _allocated = false;

    };
    
    class StagingBuffer : public Buffer {
    public:
        StagingBuffer(std::shared_ptr<Device> device) : Buffer(device) { _debug_name = "Anonymous Staging Buffer"; }
        ~StagingBuffer() = default;

        void create(size_t size, VkBufferUsageFlags extra_flags = 0);

        void * map();
        void unmap();
    private:

    };

    class AutoMapStagingBuffer : public StagingBuffer {
    public:

        enum class Mode {
            Upload,
            Download,
            Bidirectional
        };

        AutoMapStagingBuffer(std::shared_ptr<Device> device, Mode mode, size_t size) : StagingBuffer(device), _mode(mode) {
            _debug_name = "Anonymous AutoMap Staging Buffer";
            int extra_flags = 0;
            if (_mode == Mode::Upload) {
                extra_flags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            } else if (_mode == Mode::Download) {
                extra_flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            } else if (_mode == Mode::Bidirectional) {
                extra_flags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            }
            init(size, extra_flags, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            _mapped = map();
        }
        
        ~AutoMapStagingBuffer() {
            unmap();
        }

        static auto make(const std::shared_ptr<Device>& device, Mode mode, size_t size) {
            auto buf = std::make_shared<AutoMapStagingBuffer>(device, mode, size);
            return buf;
        }

        void * get() const { return _mapped; }
    private:
        void * _mapped = nullptr;
        Mode _mode = Mode::Upload;
    };

    class VertexBuffer : public Buffer {
    public:
        VertexBuffer(std::shared_ptr<Device> device) : Buffer(device) {
            _debug_name = "Anonymous Vertex Buffer";
        }

        ~VertexBuffer() = default;

        void create(size_t size);
    private:

    };

    class IndexBuffer : public Buffer {
    public:
        IndexBuffer(std::shared_ptr<Device> device) : Buffer(device) {
            _debug_name = "Anonymous Index Buffer";
        }
        ~IndexBuffer() = default;

        void create(size_t size);
    private:

    };
    class UniformBuffer : public StagingBuffer {
    public:
        UniformBuffer(std::shared_ptr<Device> device) : StagingBuffer(device) {
            _debug_name = "Anonymous Uniform Buffer";
        }
        ~UniformBuffer() = default;

        void create(size_t size);
    private:

    };
    class StorageBuffer : public StagingBuffer {
    public:
        StorageBuffer(std::shared_ptr<Device> device) : StagingBuffer(device) {
            _debug_name = "Anonymous Storage Buffer";
        }
        ~StorageBuffer() = default;

        static auto make(const std::shared_ptr<Device>& device, size_t size) {
            auto buf = std::make_shared<StorageBuffer>(device);
            buf->create(size);
            return buf;
        }

        void create(size_t size, VkBufferUsageFlags extra_flags = 0);
    private:

    };
}