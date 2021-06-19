#pragma once

#include <string>

#include "image.hpp"

namespace vkd {
    std::shared_ptr<Image> load_ktx(std::shared_ptr<Device> device, const std::string& path, VkImageUsageFlags usage_flags, VkImageLayout layout);

}