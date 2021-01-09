#pragma once

#include <vector>
#include <memory>

#include "vulkan.hpp"

namespace vulkan {
    class VertexInput {
    public:
        VertexInput() = default;
        ~VertexInput() = default;
        VertexInput(VertexInput&&) = delete;
        VertexInput(const VertexInput&) = delete;

        void add_binding(uint32_t binding, uint32_t stride, VkVertexInputRate input_rate);
        void add_attribute(uint32_t binding, uint32_t location, VkFormat format, uint32_t offset);

        auto get_binding_data() { return _vertex_bind.data(); }
        auto get_binding_size() { return _vertex_bind.size(); }
        auto get_attribute_data() { return _vertex_attr.data(); }
        auto get_attribute_size() { return _vertex_attr.size(); }
    private:
        std::vector<VkVertexInputBindingDescription> _vertex_bind;
        std::vector<VkVertexInputAttributeDescription> _vertex_attr;
    };
}