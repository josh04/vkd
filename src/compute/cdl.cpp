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
#include "ocio/ocio_functional.hpp"

namespace vkd {
    REGISTER_NODE("cdl", "cdl", CDL);

    void CDL::kernel_init() {
        _kernel = std::make_shared<Kernel>(_device, param_hash_name());
        _kernel->init("shaders/compute/cdl.comp.spv", "main", Kernel::default_local_sizes);
        register_params(*_kernel);
        
        _slop_param = _kernel->get_param_by_name("slope");
        _slop_param->as<glm::vec4>().min(glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
        constexpr float slope_max = 1.5f;
        _slop_param->as<glm::vec4>().max(glm::vec4(slope_max, slope_max, slope_max, 1.0f));
        _slop_param->as<glm::vec4>().set_default(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
        _slop_param->tag("sliders");
        _slop_param->tag("vec3");
        
        auto offset_param = _kernel->get_param_by_name("offset");
        offset_param->as<glm::vec4>().min(glm::vec4(-1.0f, -1.0f, -1.0f, -1.0f));
        constexpr float offset_max = 1.0f;
        offset_param->as<glm::vec4>().max(glm::vec4(offset_max, offset_max, offset_max, 1.0f));
        offset_param->as<glm::vec4>().set_default(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
        offset_param->tag("sliders");
        offset_param->tag("vec3");
        
        auto power_param = _kernel->get_param_by_name("power");
        power_param->as<glm::vec4>().min(glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
        constexpr float power_max = 4.0f;
        power_param->as<glm::vec4>().max(glm::vec4(power_max, power_max, power_max, 1.0f));
        power_param->as<glm::vec4>().set_default(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
        power_param->tag("sliders");
        power_param->tag("vec3");

        auto slop_param_master = _kernel->get_param_by_name("slope_master");
        slop_param_master->as<float>().min(-slope_max);
        slop_param_master->as<float>().max(slope_max);
        slop_param_master->as<float>().set_default(0.0f);
        auto offset_param_master = _kernel->get_param_by_name("offset_master");
        offset_param_master->as<float>().min(-2.0f * offset_max);
        offset_param_master->as<float>().max(2.0f * offset_max);
        offset_param_master->as<float>().set_default(0.0f);
        auto power_param_master = _kernel->get_param_by_name("power_master");
        power_param_master->as<float>().min(-power_max);
        power_param_master->as<float>().max(power_max);
        power_param_master->as<float>().set_default(0.0f);

        auto sat_param = _kernel->get_param_by_name("saturation");
        sat_param->as<float>().min(0.0f);
        sat_param->as<float>().max(2.0f);
        sat_param->as<float>().set_default(1.0f);

        _ocio_params = std::make_unique<OcioParams>(*this);
        
        _slope_picker = make_param<glm::ivec2>(*this, "slope picker", 0, {"picker"});
        _slope_picker->as<glm::ivec2>().set_force(glm::ivec2{-1, -1});

        _ocio_in_space = ocio_functional::make_ocio_param(*this, "cdl colour space");

        auto sz = output_size();
        _intermediate_image = Image::float_image(_device, sz, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
    }

    void CDL::allocate(VkCommandBuffer buf) {
        get_output_image()->allocate(buf);
        _intermediate_image->allocate(buf);
    }

    void CDL::deallocate() { 
        get_output_image()->deallocate();
        _intermediate_image->deallocate();
    }

    void CDL::kernel_params() {
        _kernel->set_arg(0, get_output_image());
        _kernel->set_arg(1, _intermediate_image);
        
        kernel_update();
    }

    void CDL::kernel_pre_update(ExecutionType type) {
        _ocio_params->update();
    }

    void CDL::kernel_update() {
        if (_ocio_in_space->changed() || _ocio_params->working_space->changed() || !_ocio_in_transform || !_ocio_out_transform) {
            _ocio_images.clear();
            _ocio_in_transform = ocio_functional::make_shader(*this, get_input_image(), get_output_image(), _ocio_params->working_space_index(), _ocio_in_space->as<int>().get(), _ocio_images);
            _ocio_out_transform = ocio_functional::make_shader(*this, _intermediate_image, get_output_image(), _ocio_in_space->as<int>().get(), _ocio_params->working_space_index(), _ocio_images);
        }
    }
    
    void CDL::execute(ExecutionType type, Stream& stream) {

        glm::ivec2 loc = _slope_picker->as<glm::ivec2>().get();
        if (_slope_picker->changed_last() && loc.x != -1 && loc.y != -1) {

            {
                auto scope = _command_buffer->record();
                auto sz = kernel_dim();
                _ocio_in_transform->dispatch(*_command_buffer, sz.x, sz.y);
            }
            
            stream.submit(command_buffer());
            stream.flush();

            glm::vec4 pixel = get_output_image()->sample<glm::vec4>(loc);
            _slop_param->as<glm::vec4>().set_force(glm::vec4{0.5f/pixel.x, 0.5f/pixel.y, 0.5f/pixel.z, 1.0f});
            _slope_picker->as<glm::ivec2>().set_force(glm::ivec2{-1, -1});

            {
                auto scope = _command_buffer->record();
                auto sz = kernel_dim();
                _kernel->dispatch(*_command_buffer, sz.x, sz.y);
                _ocio_out_transform->dispatch(*_command_buffer, sz.x, sz.y);
            }

            stream.submit(command_buffer());
        } else {
            {
                auto scope = _command_buffer->record();
                auto sz = kernel_dim();
                _ocio_in_transform->dispatch(*_command_buffer, sz.x, sz.y);
                _kernel->dispatch(*_command_buffer, sz.x, sz.y);
                _ocio_out_transform->dispatch(*_command_buffer, sz.x, sz.y);
            }

            stream.submit(command_buffer());
        }
    }

    void CDL::post_execute(ExecutionType type) {
    }
}