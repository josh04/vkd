#include "rotate.hpp"
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
    REGISTER_NODE("rotate", "rotate", Rotate);

    void Rotate::init() {
        

        _size = {0, 0};
        
        _rotate = std::make_shared<Kernel>(_device, param_hash_name());
        _rotate->init("shaders/compute/rotate.comp.spv", "main", Kernel::default_local_sizes);
        register_params(*_rotate);

        _mode = _rotate->get_param_by_name("mode");
        _mode->as<int>().set_default((int)Mode::None);
        _mode->as<int>().min((int)Mode::None);
        _mode->as<int>().max((int)Mode::Max - 1);

        
        auto image = _image_node->get_output_image();
        if (_mode->as<int>().get() == (int)Mode::Clockwise90 || _mode->as<int>().get() == (int)Mode::AntiClockwise90) {
            _size = {image->dim().y, image->dim().x};
        } else {
            _size = image->dim();
        }

        _image = Image::float_image(_device, _size, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

        _rotate->set_arg(0, image);
        _rotate->set_arg(1, _image);
    }

    void Rotate::allocate(VkCommandBuffer buf) {
        _image->allocate(buf);
    }

    void Rotate::deallocate() { 
        _image->deallocate();
    }

    bool Rotate::update(ExecutionType type) {
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

    void Rotate::execute(ExecutionType type, Stream& stream) {
        command_buffer().begin();
        _rotate->dispatch(command_buffer(), _size.x, _size.y);
        command_buffer().end();
        stream.submit(command_buffer());
    }
}