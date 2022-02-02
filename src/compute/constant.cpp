#include "constant.hpp"
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
    REGISTER_NODE("constant", "constant", Constant);

    void Constant::init() {
        _compute_command_buffer = create_command_buffer(_device->logical_device(), _device->command_pool());

        _size = {0, 0};
        
        _constant = Kernel::make(*this, "shaders/compute/constant.comp.spv", "main", {16, 16, 1});
        
        _compute_complete = Semaphore::make(_device);
        
        _size_param = make_param<glm::ivec2>(*this, "size", 0);
        _size_param->as<glm::ivec2>().set_default({1920, 1080});
        _size_param->as<glm::ivec2>().soft_min(glm::ivec2{0, 0});
        _size_param->as<glm::ivec2>().soft_max(glm::ivec2{16384, 16284});

        _size = _size_param->as<glm::ivec2>().get();

        _image = Image::float_image(_device, _size, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
        _constant->set_arg(0, _image);
    }

    void Constant::allocate(VkCommandBuffer buf) {
        _image->allocate(buf);
    }

    void Constant::deallocate() { 
        _image->deallocate();
    }

    void Constant::post_init() {
    }

    bool Constant::update(ExecutionType type) {
        bool update = false;

        for (auto&& pmap : _params) {
            for (auto&& el : pmap.second) {
                if (el.second->changed()) {
                    update = true;
                }
            }
        }

        if (update) {
            auto sz = _size_param->as<glm::ivec2>().get();
            if (sz != _image->dim()) {
                _size = sz;
                _image->set_format(VK_FORMAT_R32G32B32A32_SFLOAT, _size, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
            }
        }

        return update;
    }

    void Constant::execute(ExecutionType type, const SemaphorePtr& wait_semaphore, Fence * fence) {
        begin_command_buffer(_compute_command_buffer);
        _constant->dispatch(_compute_command_buffer, _size.x, _size.y);
        end_command_buffer(_compute_command_buffer);
        submit_compute_buffer(*_device, _compute_command_buffer, wait_semaphore, _compute_complete, fence);
    }
}