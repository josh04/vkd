#pragma once
#include "device.hpp"
#include "vulkan/vulkan.h"
#include "glm/glm.hpp"

typedef void* ImTextureID;
extern ImTextureID ImGui_ImplVulkan_AddTexture(VkSampler sampler, VkImageView image_view, VkImageLayout image_layout);
extern void ImGui_ImplVulkan_RemoveTexture(ImTextureID tex);

namespace vkd {
    class Buffer;
    class Image {
    public:
        Image(std::shared_ptr<Device> device) : _device(device) {}
        Image(std::shared_ptr<Device> device, VkImage image, VkImageView view) : _device(device), _image(image), _view(view) {}
        virtual ~Image();
        Image(Image&&) = delete;
        Image(const Image&) = delete;

        static auto float_image(const std::shared_ptr<Device>& device, glm::uvec2 size) {
            auto im = std::make_shared<vkd::Image>(device);
            im->create_image(VK_FORMAT_R32G32B32A32_SFLOAT, size.x, size.y, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT );
            im->allocate(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            im->create_view(VK_IMAGE_ASPECT_COLOR_BIT);
            return im;
        }

        static constexpr size_t size_in_memory(glm::uvec2 size, const VkFormat format) {
            size_t sz = size.x * size.y;
            if (format == VK_FORMAT_R32G32B32A32_SFLOAT) {
                sz *= 4 * 4;
            } else {
                //static_assert(false);
            }
            return sz;
        }

        size_t size_in_memory() const { return size_in_memory(dim(), _format); }

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

        auto ui_desc_set() { 
            if (_ui_desc_set == VK_NULL_HANDLE) {
                _ui_desc_set = (VkDescriptorSet)ImGui_ImplVulkan_AddTexture(_sampler, _view, _layout);
            }
            return _ui_desc_set; 
        }
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

        VkDescriptorSet _ui_desc_set = VK_NULL_HANDLE;
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