#pragma once

#include <string>
#include <memory>
#include "vulkan/vulkan.h"

namespace vkd {
    class Device;
    class Image;
    std::shared_ptr<Image> load_ktx(std::shared_ptr<Device> device, const std::string& path, VkImageUsageFlags usage_flags, VkImageLayout layout);

}