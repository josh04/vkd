#pragma once
#include "vulkan.hpp"
#include "device.hpp"

namespace vkd {
    static VkSemaphore create_semaphore(VkDevice logical_device) {
        // Semaphores (Used for correct command ordering)
		VkSemaphoreCreateInfo semaphore_create_info = {};
		semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		semaphore_create_info.pNext = nullptr;

		VkSemaphore semaphore;
		VK_CHECK_RESULT(vkCreateSemaphore(logical_device, &semaphore_create_info, nullptr, &semaphore));
        return semaphore;
    }
	class Semaphore;
	using SemaphorePtr = std::shared_ptr<Semaphore>;
	class Semaphore {
	public:
        Semaphore(const std::shared_ptr<Device>& device) : _device(device) {}
        ~Semaphore() {
			if (_semaphore != VK_NULL_HANDLE) {
				vkDestroySemaphore(_device->logical_device(), _semaphore, nullptr);
			}
		}
        Semaphore(Semaphore&&) = delete;
        Semaphore(const Semaphore&) = delete;

		static SemaphorePtr make(const std::shared_ptr<Device>& device) {
			auto ret = std::make_shared<Semaphore>(device);
			ret->create();
			return ret;
		}

		void create() { _semaphore = create_semaphore(_device->logical_device()); }

		const auto& get() const { return _semaphore; }
	private:
		VkSemaphore _semaphore = VK_NULL_HANDLE;
		std::shared_ptr<Device> _device = nullptr;
	};
}