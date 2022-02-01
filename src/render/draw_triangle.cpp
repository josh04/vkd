#include "draw_triangle.hpp"
#include "buffer.hpp"
#include "command_buffer.hpp"
#include "device.hpp"
#include "renderpass.hpp"
#include "pipeline.hpp"
#include "render/camera.hpp"
#include "shader.hpp"
#include "descriptor_sets.hpp"
#include "vertex_input.hpp"
#include "viewport_and_scissor.hpp"

namespace vkd {
    REGISTER_NODE("triangles", "draw triangle", DrawTriangle, 16.0f/9.0f);

    void DrawTriangle::init() {
       
        std::vector<Vertex> vertex_array = {
            { {  1.0f,  1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f } },
            { { -1.0f,  1.0f, 0.0f }, { 0.0f, 1.0f, 1.0f } },
            { {  0.0f, -1.0f, 0.0f }, { 1.0f, 0.0f, 1.0f } }
        };

        uint32_t varray_size = static_cast<uint32_t>(vertex_array.size()) * sizeof(Vertex);

        std::vector<uint32_t> index_array = { 0, 1, 2 };
        uint32_t iarray_size = static_cast<uint32_t>(index_array.size() * sizeof(uint32_t));

        _vertex_buffer = std::make_shared<VertexBuffer>(_device);
        _vertex_buffer->create(varray_size);
        _index_buffer = std::make_shared<IndexBuffer>(_device);
        _index_buffer->create(iarray_size);

        auto vstage = std::make_shared<StagingBuffer>(_device);
        vstage->create(varray_size);
        auto istage = std::make_shared<StagingBuffer>(_device);
        istage->create(iarray_size);

        void * vs = vstage->map();
        memcpy(vs, vertex_array.data(), varray_size);
        vstage->unmap();
        void * is = istage->map();
        memcpy(is, index_array.data(), iarray_size);
        istage->unmap();

        auto buf = begin_immediate_command_buffer(_device->logical_device(), _device->command_pool());

        _vertex_buffer->copy(*vstage, varray_size, buf);
        _index_buffer->copy(*istage, iarray_size, buf);

        flush_command_buffer(_device->logical_device(), _device->queue(), _device->command_pool(), buf);

        _uniform_buffer = std::make_shared<UniformBuffer>(_device);
        _uniform_buffer->create(varray_size);

        uint8_t * mem = (uint8_t *)_uniform_buffer->map();

        bool flip_y = true;
        projection_mat = get_perspective(75.0f, _aspect, 0.001f, 100.0f, flip_y);
        view_mat = get_view({0.0, 0.0, -4.0f}, {0.0, 0.0, 0.0}, flip_y);
        model_mat = glm::mat4(1.0f);

        memcpy(mem, &projection_mat, sizeof(glm::mat4));
        memcpy(mem + sizeof(glm::mat4), &model_mat, sizeof(glm::mat4));
        memcpy(mem + 2 * sizeof(glm::mat4), &view_mat, sizeof(glm::mat4));

        _uniform_buffer->unmap();

        _desc_pool = std::make_shared<DescriptorPool>(_device);
        _desc_pool->add_uniform_buffer(1);
        _desc_pool->create(1); // one for triangles, one for particles

        _desc_set_layout = std::make_shared<DescriptorLayout>(_device);
        _desc_set_layout->add(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT);
        _desc_set_layout->create();
        
        _desc_set = std::make_shared<DescriptorSet>(_device, _desc_set_layout, _desc_pool);
        _desc_set->add_buffer(*_uniform_buffer);
        _desc_set->create();
        
        _pipeline = std::make_shared<GraphicsPipeline>(_device, _pipeline_cache, _renderpass);
		auto shader_stages = std::make_unique<Shader>(_device);
		shader_stages->add(VK_SHADER_STAGE_VERTEX_BIT, "shaders/triangle/triangle.vert.spv", "main");
		shader_stages->add(VK_SHADER_STAGE_FRAGMENT_BIT, "shaders/triangle/triangle.frag.spv", "main");

		auto vertex_input = std::make_unique<VertexInput>();
		vertex_input->add_binding(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX);
		vertex_input->add_attribute(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position));
		vertex_input->add_attribute(0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color));

        _pipeline->create(_desc_set_layout->get(), std::move(shader_stages), std::move(vertex_input));
    }

    void DrawTriangle::post_init()
    {
    }

    void DrawTriangle::commands(VkCommandBuffer buf, uint32_t width, uint32_t height) {
        viewport_and_scissor(buf, width, height, width, height);
        _pipeline->bind(buf, _desc_set->get());

        // Bind triangle vertex buffer (contains position and colors)
        std::array<VkDeviceSize, 1> offsets = { 0 };
        auto vb = _vertex_buffer->buffer();
        vkCmdBindVertexBuffers(buf, 0, 1, &vb, offsets.data());

        // Bind triangle index buffer
        vkCmdBindIndexBuffer(buf, _index_buffer->buffer(), 0, VK_INDEX_TYPE_UINT32);

        // Draw indexed triangle
        vkCmdDrawIndexed(buf, _index_buffer->requested_size() / sizeof(uint32_t), 1, 0, 0, 1);
    }

    void DrawTriangle::execute(ExecutionType type, const SemaphorePtr& wait_semaphore, Fence * fence) {

    }
}