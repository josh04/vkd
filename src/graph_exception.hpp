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
    class RebakeException : public std::runtime_error {
    public:
        using std::runtime_error::runtime_error;
    };
    class ServiceException : public std::runtime_error {
    public:
        using std::runtime_error::runtime_error;
    };
    class ImageException : public std::runtime_error {
    public:
        using std::runtime_error::runtime_error;
    };
    class PendingException : public UpdateException {
    public:
        using vkd::UpdateException::UpdateException;
    };
	class OcioException : public std::runtime_error {
    public:
        using std::runtime_error::runtime_error;
	};
	class ExecutionException : public std::runtime_error {
    public:
        using std::runtime_error::runtime_error;
	};
}