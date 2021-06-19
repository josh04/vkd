#include <string>
#include "vulkan/vulkan.h"

namespace vkd {
    std::string error_string(VkResult errorCode);
    std::string physical_device_to_string(VkPhysicalDeviceType type);

    std::string physical_device_features_string(const VkPhysicalDeviceFeatures& feats);
    std::string physical_device_8bit_features_string(const VkPhysicalDevice8BitStorageFeaturesKHR& feats);
    std::string physical_device_16bit_features_string(const VkPhysicalDevice16BitStorageFeatures& feats);
    std::string memory_property_flags_to_string(VkMemoryPropertyFlags flags);
    std::string memory_heap_flags_to_string(VkMemoryHeapFlags flags);
    std::string queue_flags_to_string(VkQueueFlags flags);

    std::string format_to_string(VkFormat format);
}