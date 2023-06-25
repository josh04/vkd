#include <cstring>
#include "image.hpp"
#include "vulkan.hpp"
#include "memory.hpp"
#include "sampler.hpp"
#include "buffer.hpp"
#include "command_buffer.hpp"
#include "memory/memory_manager.hpp"
#include "memory/memory_pool.hpp"


namespace vkd {
    
    std::shared_ptr<Image> Image::float_image(const std::shared_ptr<Device>& device, glm::ivec2 size, VkImageUsageFlags usage_flags) {
        auto im = std::make_shared<vkd::Image>(device);
        im->create_image(VK_FORMAT_R32G32B32A32_SFLOAT, size, usage_flags | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
        im->allocate(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        im->create_view(VK_IMAGE_ASPECT_COLOR_BIT);
        im->deallocate();

        {
            auto buf = CommandBuffer::make_immediate(device);
            im->allocate(buf->get());
        }

        im->deallocate();

        return im;
    }

    Image::~Image() {
        if (_ui_desc_set != VK_NULL_HANDLE) {
            ImGui_ImplVulkan_RemoveTexture(_ui_desc_set);
        }
        _ui_desc_set = VK_NULL_HANDLE;

        if (!_no_dealloc) {
            deallocate();
        }
    }

    void Image::set_format(VkFormat format, glm::ivec2 sz, VkImageUsageFlags usage_flags) {
        _format = format;
        _width = sz.x;
        _height = sz.y;
        _usage_flags = usage_flags;
    }

    void Image::create_image(VkFormat format, glm::ivec2 sz, VkImageUsageFlags usage_flags) {
        _format = format;
        _width = sz.x;
        _height = sz.y;
        _usage_flags = usage_flags;
        VkImageCreateInfo image_create_info{};
        memset(&image_create_info, 0, sizeof(VkImageCreateInfo));
        image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_create_info.imageType = VK_IMAGE_TYPE_2D;
        image_create_info.format = format;
        image_create_info.extent = { (uint32_t)sz.x, (uint32_t)sz.y, 1 };
        image_create_info.mipLevels = 1;
        image_create_info.arrayLayers = 1;
        image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
        image_create_info.tiling = _tiling;
        image_create_info.usage = usage_flags;

        image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        _layout = VK_IMAGE_LAYOUT_UNDEFINED;

        VK_CHECK_RESULT(vkCreateImage(_device->logical_device(), &image_create_info, nullptr, &_image));

        _sampler = ScopedSampler::make(_device);

        update_debug_name();
    }

    void Image::update_debug_name() {
        if (_image != VK_NULL_HANDLE)
        {
            _device->set_debug_utils_object_name(_debug_name + " (image)", VK_OBJECT_TYPE_IMAGE, (uint64_t)_image);
        }
        if (_sampler != VK_NULL_HANDLE)
        {
            _device->set_debug_utils_object_name(_debug_name + " (sampler)", VK_OBJECT_TYPE_SAMPLER, (uint64_t)_sampler->get());
        }
        if (_view != VK_NULL_HANDLE)
        {
            _device->set_debug_utils_object_name(_debug_name + " (view)", VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)_view);
        }
        if (_ui_desc_set != VK_NULL_HANDLE)
        {
            _device->set_debug_utils_object_name(_debug_name + " (ui desc set)", VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t)_ui_desc_set);
        }
    }

    void Image::allocate(VkMemoryPropertyFlags memory_property_flags) {
        VkMemoryRequirements mem_req{};
        memset(&mem_req, 0, sizeof(VkMemoryRequirements));

        vkGetImageMemoryRequirements(_device->logical_device(), _image, &mem_req);
        uint32_t mem_index = find_memory_index(_device->memory_properties(), mem_req.memoryTypeBits, memory_property_flags);
        
        _allocated_size = mem_req.size;
        _memory_flags = memory_property_flags;
        
        _memory = _device->pool().allocate(mem_req.size, memory_property_flags, mem_index);

        VK_CHECK_RESULT(vkBindImageMemory(_device->logical_device(), _image, _memory, 0));
    }

    void Image::allocate(VkCommandBuffer buf) {
        if (!_allocated) {
            create_image(_format, {_width, _height}, _usage_flags);
            allocate(_memory_flags);
            create_view(_aspect);

            _allocated = true;

            set_layout(buf, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        }
    }

    void Image::deallocate() {
        _sampler = nullptr;
        if (_view) { 
            vkDestroyImageView(_device->logical_device(), _view, nullptr); _view = VK_NULL_HANDLE; 
            }
        if (_image) { 
            vkDestroyImage(_device->logical_device(), _image, nullptr); _image = VK_NULL_HANDLE; 
            }
        if (_memory) {
            _device->pool().deallocate(_memory);
            _memory = VK_NULL_HANDLE;
        }

        _allocated = false;
    }

    void Image::create_view(VkImageAspectFlags aspect) {
        _aspect = aspect;
        VkImageViewCreateInfo image_view_create_info{};
        memset(&image_view_create_info, 0, sizeof(image_view_create_info));

        image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        image_view_create_info.image = _image;
        image_view_create_info.format = _format;
        image_view_create_info.components = {
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY
        };
        image_view_create_info.subresourceRange.baseMipLevel = 0;
        image_view_create_info.subresourceRange.levelCount = 1;
        image_view_create_info.subresourceRange.baseArrayLayer = 0;
        image_view_create_info.subresourceRange.layerCount = 1;
        image_view_create_info.subresourceRange.aspectMask = aspect;
        
        VK_CHECK_RESULT(vkCreateImageView(_device->logical_device(), &image_view_create_info, nullptr, &_view));
        _allocated = true; // TODO remove once everything is onboard with nu-alloc
        update_debug_name();
    }

    void Image::copy(Image& src, VkCommandBuffer buf) {
        VkImageCopy copy_region = {}; 
        auto layout = _layout;
        set_layout(buf, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        auto src_layout = src.layout();
        src.set_layout(buf, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        
        copy_region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copy_region.srcSubresource.baseArrayLayer = 0;
        copy_region.srcSubresource.mipLevel = 0;
        copy_region.srcSubresource.layerCount = 1;

        copy_region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copy_region.dstSubresource.baseArrayLayer = 0;
        copy_region.dstSubresource.mipLevel = 0;
        copy_region.dstSubresource.layerCount = 1;

        copy_region.dstOffset = {0, 0, 0};
        copy_region.srcOffset = {0, 0, 0};
        copy_region.extent = {(uint32_t)_width, (uint32_t)_height, 1};
        vkCmdCopyImage(buf, src.image(), src.layout(), _image, _layout, 1, &copy_region);

        if (src_layout != VK_IMAGE_LAYOUT_UNDEFINED && src_layout != VK_IMAGE_LAYOUT_PREINITIALIZED) {
            src.set_layout(buf, src_layout, VK_IMAGE_ASPECT_COLOR_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        }
        if (layout != VK_IMAGE_LAYOUT_UNDEFINED && layout != VK_IMAGE_LAYOUT_PREINITIALIZED) {
            set_layout(buf, layout, VK_IMAGE_ASPECT_COLOR_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        }

    }

    void Image::copy(VkCommandBuffer buf, Buffer& src) {
        auto layout = _layout;
        set_layout(buf, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        
        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;

        region.imageOffset = {0, 0, 0};
        region.imageExtent = {
            (uint32_t)_width,
            (uint32_t)_height,
            1
        };
        vkCmdCopyBufferToImage(
            buf,
            src.get(),
            _image,
            _layout,
            1,
            &region
        );
        if (layout != VK_IMAGE_LAYOUT_UNDEFINED && layout != VK_IMAGE_LAYOUT_PREINITIALIZED) {
            set_layout(buf, layout, VK_IMAGE_ASPECT_COLOR_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        }
    }

    void StagingImage::create_image(VkFormat format, glm::ivec2 size) {
        _tiling = VK_IMAGE_TILING_LINEAR;
        Image::create_image(format, size, VK_IMAGE_USAGE_TRANSFER_SRC_BIT);//, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        Image::allocate(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        //Image::create_view(VK_IMAGE_ASPECT_COLOR_BIT);
    }

    void * StagingImage::map() {
        void * data = nullptr;
        VK_CHECK_RESULT(vkMapMemory(_device->logical_device(), _memory, 0, _allocated_size, 0, &data));
        return data;
    }

    void StagingImage::unmap() {
        vkUnmapMemory(_device->logical_device(), _memory);
    }


    VkImageMemoryBarrier Image::barrier_info(VkPipelineStageFlags src_stage_mask, VkPipelineStageFlags dst_stage_mask) const {

        VkImageSubresourceRange subresource_range = {};
        subresource_range.aspectMask = _aspect;
        subresource_range.baseMipLevel = 0;
        subresource_range.levelCount = 1;
        subresource_range.layerCount = 1;

        VkImageMemoryBarrier image_memory_barrier = {};
        image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        
        image_memory_barrier.oldLayout = _layout;
        image_memory_barrier.newLayout = _layout;
        image_memory_barrier.image = _image;
        image_memory_barrier.subresourceRange = subresource_range;

        image_memory_barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        return image_memory_barrier;
    }

    void Image::barrier(VkCommandBuffer buf, VkPipelineStageFlags src_stage_mask, VkPipelineStageFlags dst_stage_mask) {
        VkImageMemoryBarrier image_memory_barrier = barrier_info(src_stage_mask, dst_stage_mask);
        vkCmdPipelineBarrier(
            buf,
            src_stage_mask,
            dst_stage_mask,
            0,
            0, nullptr,
            0, nullptr,
            1, &image_memory_barrier);

    }

    void Image::set_layout(
        VkCommandBuffer buf,
        VkImageLayout new_layout,
        VkImageSubresourceRange subresource_range,
        VkPipelineStageFlags src_stage_mask,
        VkPipelineStageFlags dst_stage_mask)
    {
        // Create an image barrier object
        
        VkImageMemoryBarrier image_memory_barrier = {};
        image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        
        image_memory_barrier.oldLayout = _layout;
        image_memory_barrier.newLayout = new_layout;
        image_memory_barrier.image = _image;
        image_memory_barrier.subresourceRange = subresource_range;

        image_memory_barrier.srcAccessMask = 0;
        image_memory_barrier.dstAccessMask = 0;

        // Source layouts (old)
        // Source access mask controls actions that have to be finished on the old layout
        // before it will be transitioned to the new layout
        switch (_layout) {
        case VK_IMAGE_LAYOUT_UNDEFINED:
            // Image layout is undefined (or does not matter)
            // Only valid as initial layout
            // No flags required, listed only for completeness
            image_memory_barrier.srcAccessMask = 0;
            break;

        case VK_IMAGE_LAYOUT_PREINITIALIZED:
            // Image is preinitialized
            // Only valid as initial layout for linear images, preserves memory contents
            // Make sure host writes have been finished
            image_memory_barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            // Image is a color attachment
            // Make sure any writes to the color buffer have been finished
            image_memory_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            // Image is a depth/stencil attachment
            // Make sure any writes to the depth/stencil buffer have been finished
            image_memory_barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            // Image is a transfer source
            // Make sure any reads from the image have been finished
            image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            break;

        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            // Image is a transfer destination
            // Make sure any writes to the image have been finished
            image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            // Image is read by a shader
            // Make sure any shader reads from the image have been finished
            image_memory_barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
            break;
        default:
            // Other source layouts aren't handled (yet)
            break;
        }

        // Target layouts (new)
        // Destination access mask controls the dependency for the new image layout
        switch (new_layout)
        {
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            // Image will be used as a transfer destination
            // Make sure any writes to the image have been finished
            image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            // Image will be used as a transfer source
            // Make sure any reads from the image have been finished
            image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            break;

        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            // Image will be used as a color attachment
            // Make sure any writes to the color buffer have been finished
            image_memory_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            // Image layout will be used as a depth/stencil attachment
            // Make sure any writes to depth/stencil buffer have been finished
            image_memory_barrier.dstAccessMask = image_memory_barrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            // Image will be read in a shader (sampler, input attachment)
            // Make sure any writes to the image have been finished
            if (image_memory_barrier.srcAccessMask == 0) {
                image_memory_barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
            }
            image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            break;
        default:
            // Other source layouts aren't handled (yet)
            break;
        }

        // Put barrier inside setup command buffer
        vkCmdPipelineBarrier(
            buf,
            src_stage_mask,
            dst_stage_mask,
            0,
            0, nullptr,
            0, nullptr,
            1, &image_memory_barrier);

        _layout = new_layout;
    }

    // Fixed sub resource on first mip level and layer
    
    void Image::set_layout(
        VkCommandBuffer buf,
        VkImageLayout new_layout,
		VkImageAspectFlags aspect_mask,
        VkPipelineStageFlags src_stage_mask,
        VkPipelineStageFlags dst_stage_mask) {
            
        VkImageSubresourceRange subresource_range = {};
        subresource_range.aspectMask = aspect_mask;
        subresource_range.baseMipLevel = 0;
        subresource_range.levelCount = 1;
        subresource_range.layerCount = 1;
        set_layout(buf, new_layout, subresource_range, src_stage_mask, dst_stage_mask);
    }

    void Image::insert_image_memory_barrier(
        VkCommandBuffer buf,
        VkImageMemoryBarrier image_memory_barrier,
        VkPipelineStageFlags src_stage_mask,
        VkPipelineStageFlags dst_stage_mask) {
        

        vkCmdPipelineBarrier(
            buf,
            src_stage_mask,
            dst_stage_mask,
            0,
            0, nullptr,
            0, nullptr,
            1, &image_memory_barrier);
    }
}