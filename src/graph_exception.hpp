#pragma once

#include <stdexcept>

namespace vkd {
    class GraphException : public std::runtime_error {
    public:
        using std::runtime_error::runtime_error;
    };
    class UpdateException : public std::runtime_error {
    public:
        using std::runtime_error::runtime_error;
    };
}