#pragma once

#include <stdexcept>

namespace vkd {
	class VkException : public std::runtime_error {
    public:
        using std::runtime_error::runtime_error;
	};
    class GraphException : public std::runtime_error {
    public:
        using std::runtime_error::runtime_error;
    };
    class UpdateException : public std::runtime_error {
    public:
        using std::runtime_error::runtime_error;
    };
	class OcioException : public std::runtime_error {
    public:
        using std::runtime_error::runtime_error;
	};
}