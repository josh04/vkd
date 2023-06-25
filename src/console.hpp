#pragma once

#include <sstream>
#include <iostream>
#include <string>
#include <mutex>

#include "vkd_dll.h"

namespace vkd {
    class VKDEXPORT Console {
    public:
        Console() = default;
        ~Console() = default;
        Console(Console&&) = delete;
        Console(const Console&) = delete;
        
        Console& operator<<(std::ostream& (*f)(std::ostream&)) {
            std::scoped_lock lock(_mutex);
            std::cout << buf().str() << std::endl;
            _storage += buf().str() + "\n";
            buf().str("");
            return *this;
        }

        template<typename T>
        Console& operator<<(T lhs) {
            buf() << lhs;
            return *this;
        }

        const std::string& log() const {
            std::scoped_lock lock(_mutex);
            return _storage; 
        }

        std::stringstream& buf();

    private:
        mutable std::mutex _mutex;
        std::string _storage = "";
    };

    VKDEXPORT extern Console console;
}