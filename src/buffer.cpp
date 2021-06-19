#include "vulkan.hpp"
#include "buffer.hpp"
#include "device.hpp"
#include "memory.hpp"
#include "command_buffer.hpp"

namespace vkd {
    Buffer::~Buffer() {
        if (_buffer) { vkDestroyBuffer(_device->logical_device(), _buffer, nullptr); }
        if (_memory) { vkFreeMemory(_device->logical_device(), _memory, nullptr); }
    }

    VkBufferMemoryBarrier Buffer::barrier_info(VkPipelineStageFlags src_stage_mask, VkPipelineStageFlags dst_stage_mask) {
        VkBufferMemoryBarrier buffer_memory_barrier = {};
        buffer_memory_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        buffer_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        buffer_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        buffer_memory_barrier.offset = 0;
        buffer_memory_barrier.size = VK_WHOLE_SIZE;
        buffer_memory_barrier.buffer = _buffer;

        buffer_memory_barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        buffer_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

        return buffer_memory_barrier;
    }

    void Buffer::barrier(VkCommandBuffer buf, VkPipelineStageFlags src_stage_mask, VkPipelineStageFlags dst_stage_mask) {

        VkBufferMemoryBarrier buffer_memory_barrier = barrier_info(src_stage_mask, dst_stage_mask);

        vkCmdPipelineBarrier(
            buf,
            src_stage_mask,
            dst_stage_mask,
            0,
            0, nullptr,
            1, &buffer_memory_barrier,
            0, nullptr);
    }

    void Buffer::_create(size_t size, VkBufferUsageFlags buffer_usage_flags, VkMemoryPropertyFlags mem_prop_flags) {
        _requested_size = size;
        VkBufferCreateInfo buffer_info = {};
        buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_info.size = size;
        buffer_info.usage = buffer_usage_flags;

        VK_CHECK_RESULT(vkCreateBuffer(_device->logical_device(), &buffer_info, nullptr, &_buffer));
		VkMemoryRequirements mem_reqs;
        vkGetBufferMemoryRequirements(_device->logical_device(), _buffer, &mem_reqs);

		VkMemoryAllocateInfo memory_alloc_info = {};
		memory_alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memory_alloc_info.memoryTypeIndex = find_memory_index(_device->memory_properties(), mem_reqs.memoryTypeBits, mem_prop_flags);
        memory_alloc_info.allocationSize = mem_reqs.size;
        _size = mem_reqs.size;

        VK_CHECK_RESULT(vkAllocateMemory(_device->logical_device(), &memory_alloc_info, nullptr, &_memory));
        VK_CHECK_RESULT(vkBindBufferMemory(_device->logical_device(), _buffer, _memory, 0));

        _descriptor.buffer = buffer();
        _descriptor.offset = 0;
        _descriptor.range = _size;
    }

    void Buffer::copy(Buffer& src, size_t sz, VkCommandBuffer buf) {
        VkBufferCopy copy_region = {};
        copy_region.size = sz;
        vkCmdCopyBuffer(buf, src.buffer(), _buffer, 1, &copy_region);
    }

    void Buffer::stage(const std::vector<std::pair<void *, size_t>>& data) {

        auto vstage = std::make_shared<StagingBuffer>(_device);
        vstage->create(size());

        size_t sz = 0;
        auto vs = (uint8_t*)vstage->map();
        for (auto&& pair : data) {
            memcpy(vs, pair.first, pair.second);
            vs += pair.second;
            sz += pair.second;
        }
        vstage->unmap();

        auto buf = begin_immediate_command_buffer(_device->logical_device(), _device->command_pool());

        copy(*vstage, sz, buf);

        flush_command_buffer(_device->logical_device(), _device->queue(), _device->command_pool(), buf);

    }

    void StagingBuffer::create(size_t size, VkBufferUsageFlags extra_flags) {
        _create(size, extra_flags | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    }

    void * StagingBuffer::map() {
        void * data = nullptr;
        VK_CHECK_RESULT(vkMapMemory(_device->logical_device(), _memory, 0, _size, 0, &data));
        return data;
    }

    void StagingBuffer::unmap() {
        vkUnmapMemory(_device->logical_device(), _memory);
    }

    void VertexBuffer::create(size_t size) {
        _create(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    }

    void IndexBuffer::create(size_t size) {
        _create(size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    }

    // following some awkward paths from the og sample here
    void UniformBuffer::create(size_t sz) {
        memset(&_descriptor, 0, sizeof(VkDescriptorBufferInfo));
        _create(sz, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    }

    void StorageBuffer::create(size_t sz, VkBufferUsageFlags extra_flags) {
        memset(&_descriptor, 0, sizeof(VkDescriptorBufferInfo));
        _create(sz, extra_flags | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    }
}