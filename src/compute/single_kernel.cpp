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
        
        _input_image = _image_node->get_output_image();
        if (!_input_image) {
            throw GraphException("Input image was null in SingleKernel node");
        }

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

    bool SingleKernel::update(ExecutionType type) {
        kernel_pre_update(type);
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

        if (update) {
            kernel_update();
        }

        return update;
    }

    void SingleKernel::execute(ExecutionType type, Stream& stream) {
        {
            auto scope = _command_buffer->record();
            auto sz = kernel_dim();
            _kernel->dispatch(*_command_buffer, sz.x, sz.y);
        }
        stream.submit(_command_buffer);
    }
}