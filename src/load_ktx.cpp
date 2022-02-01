#include "load_ktx.hpp"

#include <ktx.h>
#include <ktxvulkan.h>

#include "image.hpp"
#include "command_buffer.hpp"

namespace vkd {
    std::shared_ptr<Image> load_ktx(std::shared_ptr<Device> device, const std::string& path, VkImageUsageFlags usage_flags, VkImageLayout layout) {
        ktxTexture * target = nullptr;
		ktxResult result = ktxTexture_CreateFromNamedFile(path.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &target);			

        int32_t width = target->baseWidth;
		int32_t height = target->baseHeight;
		uint32_t mip_levels = target->numLevels;

		ktx_uint8_t *ktxTextureData = ktxTexture_GetData(target);
		ktx_size_t ktxTextureSize = ktxTexture_GetSize(target);

        auto image = std::make_shared<Image>(device);
        image->create_image(VK_FORMAT_R8G8B8A8_UNORM, {width, height}, usage_flags | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
        image->allocate(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        image->create_view(VK_IMAGE_ASPECT_COLOR_BIT);


        auto staging = std::make_shared<vkd::StagingImage>(device);
        staging->create_image(VK_FORMAT_R8G8B8A8_UNORM, {width, height});

        auto ptr = staging->map();
        memcpy(ptr, ktxTextureData, ktxTextureSize);
        staging->unmap();

        auto buf = vkd::begin_immediate_command_buffer(device->logical_device(), device->command_pool());

        image->copy(*staging, buf);

        image->set_layout(buf, layout, VK_IMAGE_ASPECT_COLOR_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

        vkd::flush_command_buffer(device->logical_device(), device->queue(), device->command_pool(), buf);


        return image;
    }
}