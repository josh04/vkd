#include "draw_fullscreen.hpp"
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
#include "sampler.hpp"

namespace vkd {
    REGISTER_NODE("draw", "draw", DrawFullscreen);

    void DrawFullscreen::init() {
       
        std::vector<FSVertex> vertex_array = {
            { { -1.0f, -1.0f }, { 0.0f, 0.0f } },
            { { -1.0f,  1.0f }, { 0.0f, 1.0f } },
            { {  1.0f, -1.0f }, { 1.0f, 0.0f } },

            { {  1.0f, -1.0f }, { 1.0f, 0.0f } },
            { { -1.0f,  1.0f }, { 0.0f, 1.0f } },
            { {  1.0f,  1.0f }, { 1.0f, 1.0f } }
        };

        uint32_t varray_size = static_cast<uint32_t>(vertex_array.size()) * sizeof(FSVertex);

        std::vector<uint32_t> index_array = { 0, 1, 2, 3, 4, 5 };
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

        _desc_pool = std::make_shared<DescriptorPool>(_device);
        _desc_pool->add_combined_image_sampler(1);
        _desc_pool->create(1);

        _desc_set_layout = std::make_shared<DescriptorLayout>(_device);
        _desc_set_layout->add(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
        _desc_set_layout->create();
        
        _sampler = create_sampler(_device->logical_device());

        
        _pipeline = std::make_shared<GraphicsPipeline>(_device, _pipeline_cache, _renderpass);
		auto shader_stages = std::make_unique<Shader>(_device);
		shader_stages->add(VK_SHADER_STAGE_VERTEX_BIT, "shaders/fullscreen/fullscreen.vert.spv", "main");
		shader_stages->add(VK_SHADER_STAGE_FRAGMENT_BIT, "shaders/fullscreen/fullscreen.frag.spv", "main");

		auto vertex_input = std::make_unique<VertexInput>();
		vertex_input->add_binding(0, sizeof(FSVertex), VK_VERTEX_INPUT_RATE_VERTEX);
		vertex_input->add_attribute(0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(FSVertex, position));
		vertex_input->add_attribute(0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(FSVertex, texcoord));

        _pipeline->create(_desc_set_layout->get(), std::move(shader_stages), std::move(vertex_input));
    }

    void DrawFullscreen::post_init()
    {
    }

    void DrawFullscreen::commands(VkCommandBuffer buf, uint32_t width, uint32_t height) {
        if (!_desc_set) {
            _desc_set = std::make_shared<DescriptorSet>(_device, _desc_set_layout, _desc_pool);
            _desc_set->add_image(*_image_node->get_output_image(), _sampler);
            _desc_set->create();
        }

        float ratio = width/(float)height;
        float imratio = _image_node->get_output_ratio();
        if (imratio == 0.0f) {
            imratio = ratio;
        }
        float dratio = imratio/ratio;

        int32_t vw = width, vh = height;
        int32_t offx = 0, offy = 0;

        if (dratio < 1.0f) {
            vw = height * imratio;
            offx = (width - vw) / 2;
        } else {
            vh = width / imratio;
            offy = (height - vh) / 2;
        }

        viewport_and_scissor_with_offset(buf, offx, offy, vw, vh, width, height);
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

    void DrawFullscreen::execute(ExecutionType type, VkSemaphore wait_semaphore, Fence * fence) {
        submit_compute_buffer(_device->compute_queue(), VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE, fence);
    }
}