#include "single_kernel.hpp"
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

    void SingleKernel::init() {
        _command_buffer = CommandBuffer::make(_device);
        _compute_complete = Semaphore::make(_device);
        
        _input_image = _image_node->get_output_image();

        kernel_init();
        
        auto sz = output_size();
        _output_image = Image::float_image(_device, sz, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

        kernel_params();
    }

    void SingleKernel::allocate(VkCommandBuffer buf) {
        _output_image->allocate(buf);
    }

    void SingleKernel::deallocate() { 
        _output_image->deallocate();
    }

    void SingleKernel::post_init()
    {
    }

    bool SingleKernel::update(ExecutionType type) {
        bool update = false;

        for (auto&& pmap : _params) {
            for (auto&& el : pmap.second) {
                if (el.second->changed()) {
                    update = true;
                }
            }
        }

        if (update || _first_run) {
            _first_run = false;
        }

        return update;
    }

    void SingleKernel::execute(ExecutionType type, const SemaphorePtr& wait_semaphore, Fence * fence) {
        {
            auto scope = _command_buffer->record();
            auto sz = kernel_dim();
            _kernel->dispatch(*_command_buffer, sz.x, sz.y);
        }
        _command_buffer->submit(wait_semaphore, _compute_complete, fence);
    }
}