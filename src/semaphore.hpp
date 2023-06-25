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
	class TimelineSemaphore;
	using TimelineSemaphorePtr = std::shared_ptr<TimelineSemaphore>;
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

		virtual TimelineSemaphore * timeline() { return nullptr; }

		static SemaphorePtr make(const std::shared_ptr<Device>& device) {
			auto ret = std::make_shared<Semaphore>(device);
			ret->create();
			return ret;
		}

		void create() { _semaphore = create_semaphore(_device->logical_device()); }

		const auto& get() const { return _semaphore; }
	protected:
		VkSemaphore _semaphore = VK_NULL_HANDLE;
		std::shared_ptr<Device> _device = nullptr;
	};

    static VkSemaphore create_timeline_semaphore(VkDevice logical_device) {
		VkSemaphoreTypeCreateInfo timeline_create_info;
		timeline_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
		timeline_create_info.pNext = NULL;
		timeline_create_info.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
		timeline_create_info.initialValue = 0;

		VkSemaphoreCreateInfo semaphore_create_info = {};
		semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		semaphore_create_info.pNext = &timeline_create_info;

		VkSemaphore semaphore;
		VK_CHECK_RESULT(vkCreateSemaphore(logical_device, &semaphore_create_info, nullptr, &semaphore));
        return semaphore;
    }

	class TimelineSemaphore : public Semaphore {
	public:
        TimelineSemaphore(const std::shared_ptr<Device>& device) : Semaphore(device) {}
        ~TimelineSemaphore() {}
        TimelineSemaphore(TimelineSemaphore&&) = delete;
        TimelineSemaphore(const TimelineSemaphore&) = delete;

		TimelineSemaphore * timeline() override { return this; }

		static TimelineSemaphorePtr make(const std::shared_ptr<Device>& device) {
			auto ret = std::make_shared<TimelineSemaphore>(device);
			ret->create();
			return ret;
		}

		void create() { 
			_semaphore = create_timeline_semaphore(_device->logical_device()); 

			ext_vkWaitSemaphoresKHR = reinterpret_cast<PFN_vkWaitSemaphoresKHR>(vkGetDeviceProcAddr(_device->logical_device(), "vkWaitSemaphoresKHR"));
			ext_vkSignalSemaphoreKHR = reinterpret_cast<PFN_vkSignalSemaphoreKHR>(vkGetDeviceProcAddr(_device->logical_device(), "vkSignalSemaphoreKHR"));
			ext_vkGetSemaphoreCounterValueKHR = reinterpret_cast<PFN_vkGetSemaphoreCounterValueKHR>(vkGetDeviceProcAddr(_device->logical_device(), "vkGetSemaphoreCounterValueKHR"));
		}

		uint64_t value_from_device() const {
			uint64_t value = 0;
			ext_vkGetSemaphoreCounterValueKHR(_device->logical_device(), _semaphore, &value);
			return value;
		}

		uint64_t value() {
			return _tracking_count;
		}

		void signal() {
			signal(increment());
		}

		void signal(uint64_t signal) {
			VkSemaphoreSignalInfo signal_info;
			signal_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO;
			signal_info.pNext = NULL;
			signal_info.semaphore = _semaphore;
			signal_info.value = signal;

			ext_vkSignalSemaphoreKHR(_device->logical_device(), &signal_info);
		}

		void wait() const {
			wait(_tracking_count);
		}

		void wait(uint64_t wait) const {
			VkSemaphoreWaitInfo wait_info;
			wait_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
			wait_info.pNext = NULL;
			wait_info.flags = 0;
			wait_info.semaphoreCount = 1;
			wait_info.pSemaphores = &_semaphore;
			wait_info.pValues = &wait;

			ext_vkWaitSemaphoresKHR(_device->logical_device(), &wait_info, UINT64_MAX);
		}

		uint64_t increment() { return ++_tracking_count; }

	private:
		std::atomic_uint64_t _tracking_count = 0;
					
		PFN_vkWaitSemaphoresKHR ext_vkWaitSemaphoresKHR = nullptr;
		PFN_vkSignalSemaphoreKHR ext_vkSignalSemaphoreKHR = nullptr;
		PFN_vkGetSemaphoreCounterValueKHR ext_vkGetSemaphoreCounterValueKHR = nullptr;

	};
}