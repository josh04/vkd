#pragma once
#include <stdexcept>
#include "vulkan/vulkan.h"

namespace vulkan {

	static uint32_t find_memory_index(const VkPhysicalDeviceMemoryProperties& props, uint32_t compatible_mask, VkMemoryPropertyFlags desired_properties) {
		for (uint32_t i = 0; i < props.memoryTypeCount; i++) {
			if ((compatible_mask & 1) == 1) {
				if ((props.memoryTypes[i].propertyFlags & desired_properties) == desired_properties) {
					return i;
				}
			}
			compatible_mask >>= 1;
		}

		throw std::runtime_error("Could not find a matching memory type");
	}
}