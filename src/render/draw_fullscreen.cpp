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
#include "ocio/ocio_functional.hpp"
#include "image.hpp"
#include "compute/kernel.hpp"
#include "ocio/ocio_static.hpp"

namespace vkd {
    REGISTER_NODE("draw", "draw", DrawFullscreen);


    void DrawFullscreen::pre_init() {
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
        _vertex_buffer->debug_name("UI Fullscreen (vertex)");
        _vertex_buffer->create(varray_size);
        _index_buffer = std::make_shared<IndexBuffer>(_device);
        _index_buffer->debug_name("UI Fullscreen (index)");
        _index_buffer->create(iarray_size);

        auto vstage = std::make_shared<StagingBuffer>(_device);
        vstage->create(varray_size);
        vstage->debug_name("UI Fullscreen (vertex stage)");
        auto istage = std::make_shared<StagingBuffer>(_device);
        istage->create(iarray_size);
        istage->debug_name("UI Fullscreen (index stage)");

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
        _desc_pool->add_combined_image_sampler(3);
        _desc_pool->create(3);

        _desc_set_layout = std::make_shared<DescriptorLayout>(_device);
        _desc_set_layout->add(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
        _desc_set_layout->create();
        
        //_sampler = ScopedSampler::make(_device);

        _pipeline = std::make_shared<GraphicsPipeline>(_device, _pipeline_cache, _renderpass);
		auto shader_stages = std::make_unique<Shader>(_device);
		shader_stages->add(VK_SHADER_STAGE_VERTEX_BIT, "shaders/fullscreen/fullscreen.vert.spv", "main");
		shader_stages->add(VK_SHADER_STAGE_FRAGMENT_BIT, "shaders/fullscreen/fullscreen.frag.spv", "main");

		auto vertex_input = std::make_unique<VertexInput>();
		vertex_input->add_binding(0, sizeof(FSVertex), VK_VERTEX_INPUT_RATE_VERTEX);
		vertex_input->add_attribute(0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(FSVertex, position));
		vertex_input->add_attribute(0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(FSVertex, texcoord));

        _pipeline->create(_desc_set_layout->get(), std::move(shader_stages), std::move(vertex_input));
        
        _ocio_params = std::make_unique<OcioParams>(*this);
    }

    void DrawFullscreen::init() {
        auto image = _image_node->get_output_image();
        _input_image = image;
        glm::ivec2 size = {256, 256};
        if (image) {
            size = image->dim();
        } else {
            console << "Warning: output node could not get image from attached node." << std::endl;
        }

        _image = Image::float_image(_device, size, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
        _image->debug_name("UI Fullscreen");

        {
            auto buf = CommandBuffer::make_immediate(_device);
            _image->allocate(buf->get());
        }

        _desc_sets[_desc_set_to_use] = std::make_shared<DescriptorSet>(_device, _desc_set_layout, _desc_pool);
        _desc_sets[_desc_set_to_use]->debug_name(std::string("UI Fullscreen (desc set) ") + std::to_string(_desc_set_to_use));
        _desc_sets[_desc_set_to_use]->add_image(_image, _image->sampler());
        _desc_sets[_desc_set_to_use]->create();
        _desc_set = _desc_sets[_desc_set_to_use];
        _desc_set_to_use = (_desc_set_to_use + 1) % 3;

    }
    
    bool DrawFullscreen::update(ExecutionType type) {

        auto image = _image_node->get_output_image();
        if (image) {
            if (_input_image != image) {
                _image = Image::float_image(_device, image->dim(), VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
                _image->debug_name("UI Fullscreen");
                _input_image = image;
                _ocio_kernel = nullptr;
                _desc_set = nullptr;
            }
            return true;
        } else {
            console << "Warning: output node could not get image from attached node." << std::endl;
        }

        return false; 

/*
        if (ocio_functional::working_space_index() != _current_working_space_index) {
            _current_working_space_index = ocio_functional::working_space_index();
            _ocio_images.clear();

            try {
                _ocio_kernel = ocio_functional::make_shader(*this, _image_node->get_output_image(), _image_node->get_output_image(), ocio_functional::working_space_index(), _current_working_space_index, _ocio_images);
            } catch (OCIO::Exception& e) {
                std::stringstream strm;
                strm << "OCIO Shader build failed: " << e.what();
                throw GraphException(strm.str());
            }
        }*/
    }

    void DrawFullscreen::commands(VkCommandBuffer buf, uint32_t width, uint32_t height) {
        if (!(_image && _image->allocated())) {
            return;
        }
        //if (!_desc_set) {
        /*} else {
            _desc_set->flush();
            _desc_set->add_image(_image, _image->sampler());
            _desc_set->update();

        }*/

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

        _offset_w_h = {offx, offy, vw, vh};
        
        _pipeline->bind(buf, _desc_set);

        // Bind triangle vertex buffer (contains position and colors)
        std::array<VkDeviceSize, 1> offsets = { 0 };
        auto vb = _vertex_buffer->buffer();
        vkCmdBindVertexBuffers(buf, 0, 1, &vb, offsets.data());

        // Bind triangle index buffer
        vkCmdBindIndexBuffer(buf, _index_buffer->buffer(), 0, VK_INDEX_TYPE_UINT32);

        // Draw indexed triangle
        vkCmdDrawIndexed(buf, _index_buffer->requested_size() / sizeof(uint32_t), 1, 0, 0, 1);
    }


    void DrawFullscreen::allocate(VkCommandBuffer buf) {
    }

    void DrawFullscreen::deallocate() {
    }

    void DrawFullscreen::execute(ExecutionType type, Stream& stream) {

        _ocio_params->update();

        if (!_ocio_kernel || _ocio_params->working_space->changed() || _ocio_params->display_space->changed()) {
            _ocio_images.clear();

            try {
                _ocio_kernel = ocio_functional::make_shader(*this, _image_node->get_output_image(), _image, _ocio_params->working_space_index(), _ocio_params->display_space_index(), _ocio_images);
            } catch (OcioException& e) {
                std::stringstream strm;
                strm << "Display OCIO shader build failed: " << e.what();
                std::cerr << strm.str() << std::endl;
                _ocio_kernel = nullptr;
            } catch (OCIO::Exception& e) {
                std::stringstream strm;
                strm << "Display OCIO shader build failed: " << e.what();
                std::cerr << strm.str() << std::endl;
                _ocio_kernel = nullptr;
            }
        } else if (_ocio_kernel) {
            _ocio_kernel->set_arg(0, _image_node->get_output_image());
            _ocio_kernel->set_arg(1, _image);
        }

        if (_ocio_kernel && _image_node->get_output_image() && _image_node->get_output_image() == _input_image) {
            auto sz = _image_node->get_output_image()->dim();
            {
                auto scope = command_buffer().record();
                _ocio_kernel->dispatch(command_buffer(), sz.x, sz.y);
            }

            stream.submit(command_buffer());
        }
    }

}