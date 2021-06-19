#pragma once
#include <memory>
#include <vector>
#include "vulkan.hpp"

namespace vkd {
    class Device;
    class Buffer {
    public:
        Buffer(std::shared_ptr<Device> device) : _device(device) {}
        virtual ~Buffer();
        Buffer(Buffer&&) = delete;
        Buffer(const Buffer&) = delete;

        void copy(Buffer& src, size_t sz, VkCommandBuffer buf);

        void stage(const std::vector<std::pair<void *, size_t>>& data);

        VkBufferMemoryBarrier barrier_info(VkPipelineStageFlags src_stage_mask, VkPipelineStageFlags dst_stage_mask);
        void barrier(VkCommandBuffer buf,  VkPipelineStageFlags src_stage_mask, VkPipelineStageFlags dst_stage_mask);

        auto buffer() { return _buffer; }
        auto get() { return _buffer; }
        auto memory() { return _memory; }
        auto size() { return _size; }
        auto requested_size() { return _requested_size; }
        auto& descriptor() { return _descriptor; }

        void _create(size_t size, VkBufferUsageFlags flags, VkMemoryPropertyFlags mem_prop_flags);
    protected:
        std::shared_ptr<Device> _device;

        size_t _size = 0;
        size_t _requested_size = 0;
        VkBuffer _buffer;
        VkDeviceMemory _memory;
		VkDescriptorBufferInfo _descriptor;
    };

    class StagingBuffer : public Buffer {
    public:
        StagingBuffer(std::shared_ptr<Device> device) : Buffer(device) {}
        ~StagingBuffer() = default;

        void create(size_t size, VkBufferUsageFlags extra_flags = 0);

        void * map();
        void unmap();
    private:

    };
    class VertexBuffer : public Buffer {
    public:
        VertexBuffer(std::shared_ptr<Device> device) : Buffer(device) {}
        ~VertexBuffer() = default;

        void create(size_t size);
    private:

    };
    class IndexBuffer : public Buffer {
    public:
        IndexBuffer(std::shared_ptr<Device> device) : Buffer(device) {}
        ~IndexBuffer() = default;

        void create(size_t size);
    private:

    };
    class UniformBuffer : public StagingBuffer {
    public:
        UniformBuffer(std::shared_ptr<Device> device) : StagingBuffer(device) {}
        ~UniformBuffer() = default;

        void create(size_t size);
    private:

    };
    class StorageBuffer : public StagingBuffer {
    public:
        StorageBuffer(std::shared_ptr<Device> device) : StagingBuffer(device) {}
        ~StorageBuffer() = default;

        void create(size_t size, VkBufferUsageFlags extra_flags = 0);
    private:

    };
}