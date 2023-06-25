#include "median.hpp"
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
    REGISTER_NODE("median", "median", Median);

    void Median::init() {
        

        _size = {0, 0};
        
        _median = std::make_shared<Kernel>(_device, param_hash_name());
        _median->init("shaders/compute/median.comp.spv", "main", Kernel::default_local_sizes);
        register_params(*_median);
        
        
        auto image = _image_node->get_output_image();
        _size = image->dim();

        _image = std::make_shared<vkd::Image>(_device);
        _image->create_image(VK_FORMAT_R32G32B32A32_SFLOAT, _size, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT );
        _image->allocate(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        _image->create_view(VK_IMAGE_ASPECT_COLOR_BIT);

        auto buf = vkd::begin_immediate_command_buffer(_device->logical_device(), _device->command_pool());
        _image->set_layout(buf, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        vkd::flush_command_buffer(_device->logical_device(), _device->queue(), _device->command_pool(), buf);

        _median->set_arg(0, image);
        _median->set_arg(1, _image);
        
        command_buffer().begin();
        _median->dispatch(command_buffer(), _size.x, _size.y);
        command_buffer().end();
    }

    bool Median::update(ExecutionType type) {
        bool update = false;

        for (auto&& pmap : _params) {
            for (auto&& el : pmap.second) {
                if (el.second->changed()) {
                    update = true;
                }
            }
        }

        if (update) {
            command_buffer().begin();
            _median->dispatch(command_buffer(), _size.x, _size.y);
            command_buffer().end();
        }

        return update;
    }

    void Median::execute(ExecutionType type, Stream& stream) {
        stream.submit(command_buffer());
    }
}