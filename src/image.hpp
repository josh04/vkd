#pragma once
#include "device.hpp"
#include "vulkan/vulkan.h"
#include "glm/glm.hpp"

#include "buffer.hpp"
#include "command_buffer.hpp"
#include "sampler.hpp"

typedef void* ImTextureID;
extern ImTextureID ImGui_ImplVulkan_AddTexture(VkSampler sampler, VkImageView image_view, VkImageLayout image_layout);
extern void ImGui_ImplVulkan_RemoveTexture(ImTextureID tex);

namespace vkd {
    class Buffer;
    class Image {
    public:
        Image(std::shared_ptr<Device> device) : _device(device) {}
        Image(std::shared_ptr<Device> device, VkImage image, VkImageView view) : _device(device), _image(image), _view(view), _no_dealloc(true) {}
        virtual ~Image();
        Image(Image&&) = delete;
        Image(const Image&) = delete;

        static std::shared_ptr<Image> float_image(const std::shared_ptr<Device>& device, glm::ivec2 size, VkImageUsageFlags usage_flags = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

        static constexpr size_t size_in_memory(glm::ivec2 size, const VkFormat format) {
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

        void allocate(VkCommandBuffer buf);
        void set_format(VkFormat format, glm::ivec2 sz, VkImageUsageFlags usage_flags);
        void create_image(VkFormat format, glm::ivec2 size, VkImageUsageFlags usage_flags);
        void allocate(VkMemoryPropertyFlags memory_property_flags);
        void deallocate();
        bool allocated() const { return _allocated; }
        void create_view(VkImageAspectFlags aspect);

        void copy(Image& src, VkCommandBuffer buf);
        void copy(VkCommandBuffer buf, Buffer& src);

        template<typename T>
        T sample(glm::ivec2 loc) {
            if (!_allocated) {
                return T{};
            }
            loc.x = std::clamp(loc.x, 0, _width - 1);
            loc.y = std::clamp(loc.y, 0, _height - 1);
    
            auto buf2 = vkd::begin_immediate_command_buffer(_device->logical_device(), _device->command_pool());

            set_layout(buf2, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

            AutoMapStagingBuffer buf{_device, AutoMapStagingBuffer::Mode::Download, sizeof(T)};
            buf.barrier(buf2, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

            buf.copy(buf2, *this, 0, loc.x, loc.y, 1, 1);
        
            buf.barrier(buf2, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

            set_layout(buf2, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
            vkd::flush_command_buffer(_device->logical_device(), _device->queue(), _device->command_pool(), buf2);

            return *reinterpret_cast<T*>(buf.get());
        }

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
        const auto& sampler() const { return _sampler; }

        glm::ivec2 dim() const { return {_width, _height}; }

        auto ui_desc_set() { 
            if (_ui_desc_set == VK_NULL_HANDLE) {
                _ui_desc_set = (VkDescriptorSet)ImGui_ImplVulkan_AddTexture(_sampler->get(), _view, _layout);
                update_debug_name();
            }
            return _ui_desc_set; 
        }
        const auto& device() const { return _device; }

        void debug_name(const std::string& debugName) { _debug_name = debugName; update_debug_name(); }
    protected:
        void update_debug_name();

        static void insert_image_memory_barrier(
            VkCommandBuffer buf,
            VkImageMemoryBarrier image_memory_barrier,
            VkPipelineStageFlags src_stage_mask,
            VkPipelineStageFlags dst_stage_mask);

        std::shared_ptr<Device> _device;
        VkImage _image = VK_NULL_HANDLE;
        VkDeviceMemory _memory = VK_NULL_HANDLE;
        VkImageView _view = VK_NULL_HANDLE;
        VkFormat _format;
        int32_t _width;
        int32_t _height;

        VkImageLayout _layout;
        VkImageTiling _tiling = VK_IMAGE_TILING_OPTIMAL;

        VkImageUsageFlags _usage_flags = 0;
        VkImageAspectFlags _aspect = VK_IMAGE_ASPECT_COLOR_BIT;
        VkMemoryPropertyFlags _memory_flags = 0;

        size_t _allocated_size = 0;

        ScopedSamplerPtr _sampler = nullptr;

        VkDescriptorSet _ui_desc_set = VK_NULL_HANDLE;

        const bool _no_dealloc = false;

        bool _allocated = false;
        std::string _debug_name = "Anonymous Image";
    };

    class StagingImage : public Image {
    public:
        using Image::Image;

        void create_image(VkFormat format, glm::ivec2 size);
        void * map();
        void unmap();
    private:

    };
}