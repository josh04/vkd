#include "whitebalance.hpp"
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
    REGISTER_NODE("whitebalance", "whitebalance", WhiteBalance);

    void WhiteBalance::_check_param_sanity()
    {
        glm::vec4 wb = _wb_param->as<glm::vec4>().get();

        float div = wb.x + wb.y + wb.z;
        if (std::fabs(div - 1.0f) > 0.00001f) {
            wb.x = wb.x / div;
            wb.y = wb.y / div;
            wb.z = wb.z / div;
        }

        _wb_param->as<glm::vec4>().set(wb);
    }


    void WhiteBalance::init() {
        

        _size = {0, 0};
        
        _whitebalance = std::make_shared<Kernel>(_device, param_hash_name());
        _whitebalance->init("shaders/compute/whitebalance.comp.spv", "main", Kernel::default_local_sizes);
        register_params(*_whitebalance);
        
        _wb_param = _whitebalance->get_param_by_name("white_balance");
        _wb_param->as<glm::vec4>().set_default(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
        _wb_param->as<glm::vec4>().min(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
        _wb_param->as<glm::vec4>().max(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
        _wb_param->tag("sliders");
        _wb_param->tag("vec3");

        
        auto image = _image_node->get_output_image();
        _size = image->dim();

        _image = Image::float_image(_device, _size, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

        auto buf = vkd::begin_immediate_command_buffer(_device->logical_device(), _device->command_pool());
        _image->set_layout(buf, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        vkd::flush_command_buffer(_device->logical_device(), _device->queue(), _device->command_pool(), buf);

        _whitebalance->set_arg(0, image);
        _whitebalance->set_arg(1, _image);
    }

    void WhiteBalance::allocate(VkCommandBuffer buf) {
        _image->allocate(buf);
    }

    void WhiteBalance::deallocate() { 
        _image->deallocate();
    }

    bool WhiteBalance::update(ExecutionType type) {
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

    void WhiteBalance::execute(ExecutionType type, Stream& stream) {
        command_buffer().begin();
        _whitebalance->dispatch(command_buffer(), _size.x, _size.y);
        command_buffer().end();

        stream.submit(command_buffer());
    }
}