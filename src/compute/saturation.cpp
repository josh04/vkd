#include "saturation.hpp"
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
    REGISTER_NODE("saturation", "saturation", Saturation);

    void Saturation::kernel_init() {
        _kernel = std::make_shared<Kernel>(_device, param_hash_name());
        _kernel->init("shaders/compute/saturation.comp.spv", "main", {16, 16, 1});
        register_params(*_kernel);
        
        _sat_param = _kernel->get_param_by_name("saturation");
        _sat_param->as<float>().min(0.0f);
        _sat_param->as<float>().max(10.0f);
        _sat_param->as<float>().set_default(1.0f);
    }
    void Saturation::kernel_params() {
        _kernel->set_arg(0, get_input_image());
        _kernel->set_arg(1, get_output_image());
    }
}