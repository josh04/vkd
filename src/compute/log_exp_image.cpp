#include "log_exp_image.hpp"
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
    REGISTER_NODE("exp", "exp", Exp);

    void Exp::kernel_init() {
        _kernel = std::make_shared<Kernel>(_device, param_hash_name());
        _kernel->init("shaders/compute/exp_image.comp.spv", "main", Kernel::default_local_sizes);
        register_params(*_kernel);
    }

    void Exp::kernel_params() {
        _kernel->set_arg(0, get_input_image());
        _kernel->set_arg(1, get_output_image());
    }

    REGISTER_NODE("log", "log", Log);

    void Log::kernel_init() {
        _kernel = std::make_shared<Kernel>(_device, param_hash_name());
        _kernel->init("shaders/compute/log_image.comp.spv", "main", Kernel::default_local_sizes);
        register_params(*_kernel);
    }

    void Log::kernel_params() {
        _kernel->set_arg(0, get_input_image());
        _kernel->set_arg(1, get_output_image());
    }
}