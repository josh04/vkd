#include "vulkan.hpp"
#include "buffer.hpp"
#include "device.hpp"
#include "memory.hpp"
#include "image.hpp"
#include "command_buffer.hpp"
#include "memory/memory_manager.hpp"
#include "memory/memory_pool.hpp"

namespace vkd {
    Buffer::~Buffer() {
        deallocate();
    }

    void Buffer::init(size_t size, VkBufferUsageFlags flags, VkMemoryPropertyFlags mem_prop_flags) {
        _requested_size = size;
        _buffer_usage_flags = flags;
        _memory_flags = mem_prop_flags;

        allocate();
    }

    void Buffer::allocate() {
        _create(_requested_size, _buffer_usage_flags, _memory_flags);
        _allocated = true;
    }

    void Buffer::deallocate() { 
        if (_buffer) { 
            vkDestroyBuffer(_device->logical_device(), _buffer, nullptr); 
            _buffer = VK_NULL_HANDLE;
        }
        if (_memory) { 
            _device->pool().deallocate(_memory);
            _memory = VK_NULL_HANDLE;
        }

        _allocated = false;
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
        VkBufferCreateInfo buffer_info = {};
        buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_info.size = size;
        buffer_info.usage = buffer_usage_flags;

        VK_CHECK_RESULT(vkCreateBuffer(_device->logical_device(), &buffer_info, nullptr, &_buffer));
		VkMemoryRequirements mem_reqs;
        vkGetBufferMemoryRequirements(_device->logical_device(), _buffer, &mem_reqs);
        auto mem_index = find_memory_index(_device->memory_properties(), mem_reqs.memoryTypeBits, mem_prop_flags);

        _size = mem_reqs.size;

        _memory = _device->pool().allocate(mem_reqs.size, mem_prop_flags, mem_index);
        VK_CHECK_RESULT(vkBindBufferMemory(_device->logical_device(), _buffer, _memory, 0));

        _descriptor.buffer = buffer();
        _descriptor.offset = 0;
        _descriptor.range = _requested_size;
    }

    void Buffer::copy(Buffer& src, size_t sz, VkCommandBuffer buf) {
        VkBufferCopy copy_region = {};
        copy_region.size = sz;
        vkCmdCopyBuffer(buf, src.buffer(), _buffer, 1, &copy_region);
    }

    void Buffer::copy(VkCommandBuffer buf, Image& src, int offset, int xStart, int yStart, uint32_t width, uint32_t height) {

        /*


void vkCmdCopyImageToBuffer(
    VkCommandBuffer                             commandBuffer,
    VkImage                                     srcImage,
    VkImageLayout                               srcImageLayout,
    VkBuffer                                    dstBuffer,
    uint32_t                                    regionCount,
    const VkBufferImageCopy*                    pRegions);

typedef struct VkBufferImageCopy {
    VkDeviceSize                bufferOffset;
    uint32_t                    bufferRowLength;
    uint32_t                    bufferImageHeight;
    VkImageSubresourceLayers    imageSubresource;
    VkOffset3D                  imageOffset;
    VkExtent3D                  imageExtent;
} VkBufferImageCopy;

        */
        VkBufferImageCopy region;
        region.bufferOffset = offset;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        
        region.imageOffset = {xStart, yStart, 0};
        region.imageExtent = {
            width,
            height,
            1
        };

        vkCmdCopyImageToBuffer(buf, src.image(), src.layout(), _buffer, 1, &region);
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
        init(size, extra_flags | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
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
        init(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    }

    void IndexBuffer::create(size_t size) {
        init(size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    }

    // following some awkward paths from the og sample here
    void UniformBuffer::create(size_t sz) {
        memset(&_descriptor, 0, sizeof(VkDescriptorBufferInfo));
        init(sz, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    }

    void StorageBuffer::create(size_t sz, VkBufferUsageFlags extra_flags) {
        memset(&_descriptor, 0, sizeof(VkDescriptorBufferInfo));
        init(sz, extra_flags | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    }
}