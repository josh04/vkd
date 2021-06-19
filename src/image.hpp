#pragma once
#include "device.hpp"
#include "vulkan/vulkan.h"
#include "glm/glm.hpp"

namespace vkd {
    class Buffer;
    class Image {
    public:
        Image(std::shared_ptr<Device> device) : _device(device) {}
        Image(std::shared_ptr<Device> device, VkImage image, VkImageView view) : _device(device), _image(image), _view(view) {}
        virtual ~Image() = default;
        Image(Image&&) = delete;
        Image(const Image&) = delete;

        VkImageMemoryBarrier barrier_info(VkPipelineStageFlags src_stage_mask, VkPipelineStageFlags dst_stage_mask) const;
        void barrier(VkCommandBuffer buf, VkPipelineStageFlags src_stage_mask, VkPipelineStageFlags dst_stage_mask);

        void create_image(VkFormat format, int32_t width, int32_t height, VkImageUsageFlags usage_flags);
        void allocate(VkMemoryPropertyFlags memory_property_flags);
        void create_view(VkImageAspectFlags aspect);

        void copy(Image& src, VkCommandBuffer buf);
        void copy(VkCommandBuffer buf, Buffer& src);

        void set_layout(
            VkCommandBuffer buf,
            VkImageLayout new_layout,
            VkImageSubresourceRange subresource_range,
            VkPipelineStageFlags src_stage_mask,
            VkPipelineStageFlags dst_stage_mask);
        void set_layout(
            VkCommandBuffer buf,
            VkImageLayout new_layout,
            VkImageAspectFlags aspect_mask,
            VkPipelineStageFlags src_stage_mask,
            VkPipelineStageFlags dst_stage_mask);

        auto image() const { return _image;}
        auto view() const { return _view; }
        auto layout() const { return _layout; }
        auto sampler() const { return _sampler; }

        glm::uvec2 dim() const { return {_width, _height}; }
    protected:
        static void insert_image_memory_barrier(
            VkCommandBuffer buf,
            VkImageMemoryBarrier image_memory_barrier,
            VkPipelineStageFlags src_stage_mask,
            VkPipelineStageFlags dst_stage_mask);

        std::shared_ptr<Device> _device;
        VkImage _image;
        VkDeviceMemory _memory;
        VkImageView _view;
        VkFormat _format;
        uint32_t _width;
        uint32_t _height;

        VkImageLayout _layout;
        VkImageTiling _tiling = VK_IMAGE_TILING_OPTIMAL;

        VkImageAspectFlags _aspect = VK_IMAGE_ASPECT_COLOR_BIT;

        size_t _allocated_size = 0;

        VkSampler _sampler = VK_NULL_HANDLE;
    };

    class StagingImage : public Image {
    public:
        using Image::Image;

        void create_image(VkFormat format, int32_t width, int32_t height);
        void * map();
        void unmap();
    private:

    };
}