#include "gaussian.hpp"
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
    REGISTER_NODE("gaussian", "blur", Gaussian);

    void Gaussian::init() {
        _compute_command_buffer = create_command_buffer(_device->logical_device(), _device->command_pool());

        _size = {0, 0};
        
        _horiz = std::make_shared<Kernel>(_device);
        _horiz->init("shaders/compute/gaussian_horiz.comp.spv", "main", {16, 16, 1});
        register_params(*_horiz);

        _vert = std::make_shared<Kernel>(_device);
        _vert->init("shaders/compute/gaussian_vert.comp.spv", "main", {16, 16, 1});
        register_params(*_vert);

        auto sigmaS = _horiz->get_param_by_name("sigma");
        sigmaS->as<float>().set(0.2f);
        sigmaS->as<float>().min(0.0001f);
        sigmaS->as<float>().max(10.0f);
        sigmaS = _vert->get_param_by_name("sigma");
        sigmaS->as<float>().set(0.2f);
        sigmaS->as<float>().min(0.0001f);
        sigmaS->as<float>().max(10.0f);

        auto half_window = _horiz->get_param_by_name("half_window");
        half_window->as<int>().set(5);
        half_window->as<int>().min(1);
        half_window->as<int>().max(200);
        half_window = _vert->get_param_by_name("half_window");
        half_window->as<int>().set(5);
        half_window->as<int>().min(1);
        half_window->as<int>().max(200);

        _compute_complete = create_semaphore(_device->logical_device());
    }

    bool Gaussian::update() {
        if (_size == glm::uvec2{0,0}) {
            auto image = _image_node->get_output_image();
            _size = image->dim();

            _stage = std::make_shared<vulkan::Image>(_device);
            _stage->create_image(VK_FORMAT_R32G32B32A32_SFLOAT, _size.x, _size.y, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT );
            _stage->allocate(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            _stage->create_view(VK_IMAGE_ASPECT_COLOR_BIT);

            _image = std::make_shared<vulkan::Image>(_device);
            _image->create_image(VK_FORMAT_R32G32B32A32_SFLOAT, _size.x, _size.y, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT );
            _image->allocate(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            _image->create_view(VK_IMAGE_ASPECT_COLOR_BIT);

            auto buf = vulkan::begin_immediate_command_buffer(_device->logical_device(), _device->command_pool());
            _stage->set_layout(buf, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
            _image->set_layout(buf, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
            vulkan::flush_command_buffer(_device->logical_device(), _device->queue(), _device->command_pool(), buf);

            _horiz->set_arg(0, image);
            _horiz->set_arg(1, _stage);
            _horiz->update();
            _vert->set_arg(0, _stage);
            _vert->set_arg(1, _image);
            _vert->update();
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
            _horiz->dispatch(_compute_command_buffer, _size.x, _size.y);
            _vert->dispatch(_compute_command_buffer, _size.x, _size.y);
            end_command_buffer(_compute_command_buffer);
        }

        return false;
    }

    void Gaussian::execute(VkSemaphore wait_semaphore) {
        submit_compute_buffer(_device->compute_queue(), _compute_command_buffer, wait_semaphore, _compute_complete);
    }
}