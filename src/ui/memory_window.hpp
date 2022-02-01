#pragma once
        
#include <memory>
#include <chrono>
#include <array>

#include "memory/memory_manager.hpp"

namespace vkd {
    class Device;
    class MemoryWindow {
    public:
        MemoryWindow() = default;
        ~MemoryWindow() = default;
        MemoryWindow(MemoryWindow&&) = delete;
        MemoryWindow(const MemoryWindow&) = delete;

        void draw(Device& d);
        void open(bool set) { _open = set; }
        bool open() const { return _open; }
        
        template<class Archive>
        void serialize(Archive& archive, const uint32_t version) {
            if (version == 0) {
                archive(_open);
            }
        }
    private:
        bool _open = false;
        
    };
}