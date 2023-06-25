#include "exposure.hpp"
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
    REGISTER_NODE("exposure", "exposure", Exposure);

    void Exposure::init() {
        

        _size = {0, 0};
        
        _exposure = std::make_shared<Kernel>(_device, param_hash_name());
        _exposure->init("shaders/compute/exposure.comp.spv", "main", Kernel::default_local_sizes);
        register_params(*_exposure);

        auto exp = _exposure->get_param_by_name("exposure");
        exp->as<float>().set_default(0.0f);
        exp->as<float>().min(-10.0f);
        exp->as<float>().max(10.0f);

        auto gamma = _exposure->get_param_by_name("gamma");
        gamma->as<float>().set_default(1.0f);
        gamma->as<float>().min(0.0001f);
        gamma->as<float>().max(10.0f);
        //_exposure->set_push_arg_by_name(VK_NULL_HANDLE, "gamma", 1.0f);

        
        auto image = _image_node->get_output_image();
        _size = image->dim();

        _image = Image::float_image(_device, _size, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

        _exposure->set_arg(0, image);
        _exposure->set_arg(1, _image);
    }

    void Exposure::allocate(VkCommandBuffer buf) {
        _image->allocate(buf);
    }

    void Exposure::deallocate() { 
        _image->deallocate();
    }

    bool Exposure::update(ExecutionType type) {
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

    void Exposure::execute(ExecutionType type, Stream& stream) {
        command_buffer().begin();
        _exposure->dispatch(command_buffer(), _size.x, _size.y);
        command_buffer().end();
        stream.submit(command_buffer());
    }
}