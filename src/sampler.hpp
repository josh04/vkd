#pragma once
#include "vulkan.hpp"
#include "device.hpp"

namespace vkd {
    static VkSampler create_sampler(VkDevice device) {
		// Font texture Sampler
		VkSamplerCreateInfo sampler_create_info = {};
		sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		sampler_create_info.maxAnisotropy = 1.0f;
		sampler_create_info.magFilter = VK_FILTER_LINEAR;
		sampler_create_info.minFilter = VK_FILTER_LINEAR;
		sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sampler_create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sampler_create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sampler_create_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		VkSampler sampler;
		VK_CHECK_RESULT(vkCreateSampler(device, &sampler_create_info, nullptr, &sampler));
		return sampler;
    }

	class ScopedSampler;
	using ScopedSamplerPtr = std::unique_ptr<ScopedSampler>;
	class ScopedSampler {
	public:
		ScopedSampler(const std::shared_ptr<Device>& device) : _device(device), _sampler(create_sampler(device->logical_device())) {

		}

		~ScopedSampler() {
			vkDestroySampler(_device->logical_device(), _sampler, nullptr);
		}

		static ScopedSamplerPtr make(const std::shared_ptr<Device>& device) {
			return std::make_unique<ScopedSampler>(device);
		}

		auto get() const { return _sampler; }

	private:
		std::shared_ptr<Device> _device = nullptr;
		VkSampler _sampler = VK_NULL_HANDLE;
	};
}
