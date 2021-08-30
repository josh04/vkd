#include "download.hpp"
#include <random>
#include "command_buffer.hpp"
#include "device.hpp"
#include "pipeline.hpp"
#include "shader.hpp"
#include "vulkan.hpp"
#include "compute/kernel.hpp"
#include "image.hpp"
#include "buffer.hpp"

namespace vkd {
    REGISTER_NODE("download", "download", Download);

    void Download::init() {
        _compute_command_buffer = create_command_buffer(_device->logical_device(), _device->command_pool());

        _quantise_luma = std::make_shared<Kernel>(_device, param_hash_name());
        _quantise_luma->init("shaders/output/quantise_luma.comp.spv", "main", {16, 16, 1});
        register_params(*_quantise_luma);
        _quantise_chroma_u = std::make_shared<Kernel>(_device, param_hash_name());
        _quantise_chroma_u->init("shaders/output/quantise_chroma_u.comp.spv", "main", {16, 16, 1});
        register_params(*_quantise_chroma_u);
        _quantise_chroma_v = std::make_shared<Kernel>(_device, param_hash_name());
        _quantise_chroma_v->init("shaders/output/quantise_chroma_v.comp.spv", "main", {16, 16, 1});
        register_params(*_quantise_chroma_v);

        _size = {0,0};

        _compute_complete = create_semaphore(_device->logical_device());

        auto image = _image_node->get_output_image();
        _size = image->dim();
        _buffer_size = _size.x * _size.y * 1.5f;

        _gpu_buffer = StorageBuffer::make(_device, _buffer_size);
        _host_buffer = AutoMapStagingBuffer::make(_device, _buffer_size);

        _quantise_luma->set_push_arg_by_name("_size", glm::ivec2(_size));

        //_quantise_luma->set_arg(0, _gpu_buffer);
        _quantise_luma->set_arg(0, image);
        _quantise_luma->set_arg(1, _gpu_buffer);
        _quantise_luma->update();

        _quantise_chroma_u->set_push_arg_by_name("_size", glm::ivec2(_size));
        _quantise_chroma_u->set_arg(0, image);
        _quantise_chroma_u->set_arg(1, _gpu_buffer);
        _quantise_chroma_u->update();

        _quantise_chroma_v->set_push_arg_by_name("_size", glm::ivec2(_size));
        _quantise_chroma_v->set_arg(0, image);
        _quantise_chroma_v->set_arg(1, _gpu_buffer);
        _quantise_chroma_v->update();

        begin_command_buffer(_compute_command_buffer);
        _quantise_luma->dispatch(_compute_command_buffer, _size.x / 4, _size.y);
        _quantise_chroma_u->dispatch(_compute_command_buffer, _size.x / 8, _size.y/2);
        _quantise_chroma_v->dispatch(_compute_command_buffer, _size.x / 8, _size.y/2);

        _gpu_buffer->barrier(_compute_command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        _host_buffer->copy(*_gpu_buffer, _buffer_size, _compute_command_buffer);
        _gpu_buffer->barrier(_compute_command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

        end_command_buffer(_compute_command_buffer);
    }

    void Download::post_init()
    {
    }

    bool Download::update(ExecutionType type) {
        bool update = false;
        for (auto&& pmap : _params) {
            for (auto&& el : pmap.second) {
                if (el.second->changed()) {
                    update = true;
                }
            }
        }

        return update;
    }

    void Download::execute(ExecutionType type, VkSemaphore wait_semaphore, Fence * fence) {
        submit_compute_buffer(_device->compute_queue(), _compute_command_buffer, wait_semaphore, _compute_complete, fence);
    }
}