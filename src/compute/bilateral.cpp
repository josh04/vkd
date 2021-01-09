#include "bilateral.hpp"
#include <random>
#include "command_buffer.hpp"
#include "device.hpp"
#include "pipeline.hpp"
#include "shader.hpp"
#include "vulkan.hpp"
#include "kernel.hpp"
#include "image.hpp"
#include "buffer.hpp"

namespace vulkan {
    REGISTER_NODE("bilateral", "bilateral", Bilateral);

    void Bilateral::init() {
        _compute_command_buffer = create_command_buffer(_device->logical_device(), _device->command_pool());

        _size = {0, 0};
        
        _blur = std::make_shared<Kernel>(_device);
        _blur->init("shaders/compute/bilateral.comp.spv", "main", {16, 16, 1});
        register_params(*_blur);

       auto sigmaS = _blur->get_param_by_name("sigma_s");
       sigmaS->as<float>().set(0.4f);
       sigmaS->as<float>().min(0.0001f);
       sigmaS->as<float>().max(1.0f);

       auto sigmaR = _blur->get_param_by_name("sigma_r");
       sigmaR->as<float>().set(0.2f);
       sigmaR->as<float>().min(0.0001f);
       sigmaR->as<float>().max(1.0f);

       auto half_window = _blur->get_param_by_name("halfWindow");
       half_window->as<int>().set(5);
       half_window->as<int>().min(1);
       half_window->as<int>().max(200);

        _compute_complete = create_semaphore(_device->logical_device());
    }

    bool Bilateral::update() {
        if (_size == glm::uvec2{0,0}) {
            auto image = _image_node->get_output_image();
            _size = image->dim();

            _image = std::make_shared<vulkan::Image>(_device);
            _image->create_image(VK_FORMAT_R32G32B32A32_SFLOAT, _size.x, _size.y, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT );
            _image->allocate(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            _image->create_view(VK_IMAGE_ASPECT_COLOR_BIT);

            auto buf = vulkan::begin_immediate_command_buffer(_device->logical_device(), _device->command_pool());
            _image->set_layout(buf, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
            vulkan::flush_command_buffer(_device->logical_device(), _device->queue(), _device->command_pool(), buf);

            _blur->set_arg(0, image);
            _blur->set_arg(1, _image);
            _blur->update();
        }

        bool update = false;
        for (auto&& pmap : _params) {
            for (auto&& el : pmap.second) {
                if (el.second->changed()) {
                    update = true;
                }
            }
        }

        if (update) {
            begin_command_buffer(_compute_command_buffer);
            _blur->dispatch(_compute_command_buffer, _size.x, _size.y);
            end_command_buffer(_compute_command_buffer);
        }

        return false;
    }

    void Bilateral::execute(VkSemaphore wait_semaphore) {
        submit_compute_buffer(_device->compute_queue(), _compute_command_buffer, wait_semaphore, _compute_complete);
    }
}