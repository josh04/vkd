#include "vertex_input.hpp"
#include "pipeline.hpp"

namespace vkd {
    void VertexInput::add_binding(uint32_t binding, uint32_t stride, VkVertexInputRate input_rate) {
        VkVertexInputBindingDescription vertex_binding_desc = {};
		vertex_binding_desc.binding = binding;
		vertex_binding_desc.stride = stride;
		vertex_binding_desc.inputRate = input_rate;
        _vertex_bind.push_back(vertex_binding_desc);
    }

    void VertexInput::add_attribute(uint32_t binding, uint32_t location, VkFormat format, uint32_t offset) {
		VkVertexInputAttributeDescription vertex_attr_desc = {};
		vertex_attr_desc.binding = binding;
		vertex_attr_desc.location = location;
		vertex_attr_desc.format = format;
		vertex_attr_desc.offset = offset;

        _vertex_attr.push_back(vertex_attr_desc);
    }
}