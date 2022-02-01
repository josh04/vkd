#pragma once

#include <thread>
#include "device.hpp"

namespace vkd {

    class Event {
    public:
        Fence(const std::shared_ptr<Device> device) : _device(device) {}
        
        ~Fence() { 
            if (_event) {
                vkDestroyEvent(_device->logical_device(), _event, nullptr);
            }
            _event = VK_NULL_HANDLE;
        }
        Event(const Event&) = delete;
        Event(Event&&) = delete;

        void create() {
            VkEventCreateInfo info;
            info.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
            info.pNext = nullptr;
            info.flags = 0; // no flags in 1.0; VK_EVENT_CREATE_DEVICE_ONLY_BIT_KHR later

            VK_CHECK_RESULT(vkCreateEvent(_device->logical_device(), &info, nullptr, &_event));
        }

        auto get() const { return _event; }

        void wait() {
            bool set = false;

            while (!set) {
                auto ret = vkGetEventStatus(_device->logical_device(), _event);
                if (ret == VK_EVENT_SET) {
                    break;
                } else if (ret == VK_EVENT_RESET) {
                    continue;
                } else {
                    VK_CHECK_RESULT(ret);
                }
            }
        }

    private:
        VkEvent _event;
        std::shared_ptr<Device> _device = nullptr;

    };
}