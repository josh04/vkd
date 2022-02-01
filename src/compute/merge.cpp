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

        _compute_complete = Semaphore::make(_device);

        auto image = _inputs[0]->get_output_image();
        if (!image) {
            throw GraphException("Input image not found");
        }
        _size = image->dim();

        _image = Image::float_image(_device, _size, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

        _first_run = true;
        
        _blank->set_push_arg_by_name("_blank", glm::vec4(0.0, 0.0, 0.0, 0.0));
        _blank->set_arg(0, _image);
    }

    void Merge::allocate(VkCommandBuffer buf) {
        _image->allocate(buf);
    }

    void Merge::deallocate() { 
        _image->deallocate();
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

        return update;
    }

    void Merge::execute(ExecutionType type, const SemaphorePtr& wait_semaphore, Fence * fence) {
        _command_buffer->begin();
        _blank->dispatch(*_command_buffer.get(), _size.x, _size.y);

        for (auto&& i : _inputs) {
            _merge->set_arg(0, _image);
            _merge->set_arg(1, i->get_output_image());

            _merge->dispatch(*_command_buffer.get(), _size.x, _size.y);
        }
        _command_buffer->end();
        _command_buffer->submit(wait_semaphore, _compute_complete, fence);
    }
}