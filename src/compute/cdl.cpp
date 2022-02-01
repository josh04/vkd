#include "cdl.hpp"
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
    REGISTER_NODE("cdl", "cdl", CDL);

    void CDL::kernel_init() {
        _kernel = std::make_shared<Kernel>(_device, param_hash_name());
        _kernel->init("shaders/compute/cdl.comp.spv", "main", {16, 16, 1});
        register_params(*_kernel);
        
        _slop_param = _kernel->get_param_by_name("slope");
        _slop_param->as<glm::vec4>().min(glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
        _slop_param->as<glm::vec4>().max(glm::vec4(3.0f, 3.0f, 3.0f, 1.0f));
        _slop_param->as<glm::vec4>().set_default(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
        _slop_param->tag("sliders");
        _slop_param->tag("vec3");
        
        auto offset_param = _kernel->get_param_by_name("offset");
        offset_param->as<glm::vec4>().min(glm::vec4(-1.0f, -1.0f, -1.0f, -1.0f));
        offset_param->as<glm::vec4>().max(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
        offset_param->as<glm::vec4>().set_default(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
        offset_param->tag("sliders");
        offset_param->tag("vec3");
        
        auto power_param = _kernel->get_param_by_name("power");
        power_param->as<glm::vec4>().min(glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
        power_param->as<glm::vec4>().max(glm::vec4(5.0f, 5.0f, 5.0f, 1.0f));
        power_param->as<glm::vec4>().set_default(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
        power_param->tag("sliders");
        power_param->tag("vec3");

        auto slop_param_master = _kernel->get_param_by_name("slope_master");
        slop_param_master->as<float>().min(-3.0f);
        slop_param_master->as<float>().max(3.0f);
        slop_param_master->as<float>().set_default(0.0f);
        auto offset_param_master = _kernel->get_param_by_name("offset_master");
        offset_param_master->as<float>().min(-1.0f);
        offset_param_master->as<float>().max(1.0f);
        offset_param_master->as<float>().set_default(0.0f);
        auto power_param_master = _kernel->get_param_by_name("power_master");
        power_param_master->as<float>().min(-3.0f);
        power_param_master->as<float>().max(3.0f);
        power_param_master->as<float>().set_default(0.0f);

        auto sat_param = _kernel->get_param_by_name("saturation");
        sat_param->as<float>().min(0.0f);
        sat_param->as<float>().max(4.0f);
        sat_param->as<float>().set_default(1.0f);
        
        _slope_picker = make_param<glm::ivec2>(*this, "slope picker", 0, {"picker"});
        _slope_picker->as<glm::ivec2>().set(glm::ivec2{-1, -1});
        _fence = Fence::create(_device, false);
    }

    void CDL::kernel_params() {
        _kernel->set_arg(0, get_input_image());
        _kernel->set_arg(1, get_output_image());
    }
    
    void CDL::execute(ExecutionType type, const SemaphorePtr& wait_semaphore, Fence * fence) {
        _fence->reset();
        {
            auto scope = _command_buffer->record();
            auto sz = kernel_dim();
            _kernel->dispatch(*_command_buffer, sz.x, sz.y);
        }
        
        _fence->submit();
        command_buffer().submit(wait_semaphore, semaphore(), fence);


        glm::ivec2 loc = _slope_picker->as<glm::ivec2>().get();
        if (_slope_picker->changed_last() && loc.x != -1 && loc.y != -1) {
            _fence->wait();
            glm::vec4 pixel = get_input_image()->sample<glm::vec4>(loc);
            _slop_param->as<glm::vec4>().set_force(glm::vec4{0.5f/pixel.x, 0.5f/pixel.y, 0.5f/pixel.z, 1.0f});
            _slope_picker->as<glm::ivec2>().set_force(glm::ivec2{-1, -1});
        }
    }

    void CDL::post_execute(ExecutionType type) {
    }
}