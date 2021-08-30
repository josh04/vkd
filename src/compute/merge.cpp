#include "merge.hpp"
#include <random>
#include "command_buffer.hpp"
#include "device.hpp"
#include "pipeline.hpp"
#include "shader.hpp"
#include "vulkan.hpp"
#include "kernel.hpp"
#include "image.hpp"
#include "buffer.hpp"

namespace vkd {
    REGISTER_NODE("merge", "merge", Merge);

    void Merge::init() {
        _command_buffer = CommandBuffer::make(_device);

        _size = {0, 0};
        
        _merge = std::make_shared<Kernel>(_device, param_hash_name());
        _merge->init("shaders/compute/merge.comp.spv", "main", {16, 16, 1});
        register_params(*_merge);
        
        _blank = std::make_shared<Kernel>(_device, param_hash_name());
        _blank->init("shaders/compute/blank.comp.spv", "main", {16, 16, 1});
        register_params(*_blank);

        _compute_complete = create_semaphore(_device->logical_device());

        auto image = _inputs[0]->get_output_image();
        if (!image) {
            throw GraphException("Input image not found");
        }
        _size = image->dim();

        _image = std::make_shared<vkd::Image>(_device);
        _image->create_image(VK_FORMAT_R32G32B32A32_SFLOAT, _size.x, _size.y, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT );
        _image->allocate(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        _image->create_view(VK_IMAGE_ASPECT_COLOR_BIT);

        auto buf = vkd::begin_immediate_command_buffer(_device->logical_device(), _device->command_pool());
        _image->set_layout(buf, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        vkd::flush_command_buffer(_device->logical_device(), _device->queue(), _device->command_pool(), buf);
        _first_run = true;
        
        _blank->set_push_arg_by_name("_blank", glm::vec4(0.0, 0.0, 0.0, 0.0));

        _blank->set_arg(0, _image);
        _blank->update();
    }

    void Merge::post_init()
    {
    }

    bool Merge::update(ExecutionType type) {
        bool update = false || _first_run;
        _first_run = false;
        for (auto&& pmap : _params) {
            for (auto&& el : pmap.second) {
                if (el.second->changed()) {
                    update = true;
                }
            }
        }

        if (update) {
            _command_buffer->begin();
            _blank->dispatch(*_command_buffer.get(), _size.x, _size.y);

            for (auto&& i : _inputs) {
                _merge->set_arg(0, _image);
                _merge->set_arg(1, i->get_output_image());
                _merge->update();

                _merge->dispatch(*_command_buffer.get(), _size.x, _size.y);
            }
            _command_buffer->end();
        }

        return update;
    }

    void Merge::execute(ExecutionType type, VkSemaphore wait_semaphore, Fence * fence) {
        _command_buffer->submit(wait_semaphore, _compute_complete, fence);
    }
}