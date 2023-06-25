#pragma once
        
#include <memory>
#include <chrono>
#include <array>

#include "console.hpp"

namespace vkd {
    class Device;
    class ConsoleWindow {
    public:
        ConsoleWindow() = default;
        ~ConsoleWindow() = default;
        ConsoleWindow(ConsoleWindow&&) = delete;
        ConsoleWindow(const ConsoleWindow&) = delete;

        void draw();
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