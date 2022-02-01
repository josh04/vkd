#include "custom.hpp"
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
    REGISTER_NODE("custom", "custom", Custom);

    void Custom::_make_shader(const std::shared_ptr<Image>& inp, const std::shared_ptr<Image>& outp, const std::string& custom_kernel) {
        std::string kernel_prefix = R"src(#version 450 
            
layout(rgba32f) uniform image2D inputTex;
layout(rgba32f) uniform image2D outputTex;

layout (local_size_x_id = 0, local_size_y_id = 1, local_size_z_id = 2) in;

layout (push_constant) uniform PushConstants {
    ivec4 vkd_offset;
} push;

)src";
        std::string kernel_postfix = R"src(

void main() 
{
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy) + push.vkd_offset.xy;
    
    vec4 inp = imageLoad(inputTex, coord);
    vec4 inp2 = CustomMain(inp);

    imageStore(outputTex, coord, inp2);
}
        
        )src";

        std::string kernel_text = kernel_prefix + custom_kernel + kernel_postfix;

        auto shader = std::make_unique<ManualComputeShader>(_device);
        shader->create(kernel_text, "main");

        _custom = Kernel::make(*this, std::move(shader), "CUSTOM_SHADER", {16, 16, 1});

        _custom->set_arg(0, inp);
        _custom->set_arg(1, outp);

    }

    void Custom::init() {
        _compute_command_buffer = create_command_buffer(_device->logical_device(), _device->command_pool());

        _size = {0, 0};
        
        _recompile = make_param<ParameterType::p_bool>(*this, "recompile", 0, {"button"});
        _custom_kernel = make_param<std::string>(*this, "kernel", 0, {"code"});
        _custom_kernel->as<std::string>().set_default(R"src(vec4 CustomMain(vec4 inp) {
    vec4 weights = vec4(0.2126, 0.7152, 0.0722, 0.0);
    float luma = dot(weights, inp);
    return vec4(luma, luma, luma, 1.0);
})src");
        
        _compute_complete = Semaphore::make(_device);
        
        auto image = _image_node->get_output_image();
        _size = image->dim();

        _image = Image::float_image(_device, _size, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

        _make_shader(image, _image, _custom_kernel->as<std::string>().get());
    }

    void Custom::allocate(VkCommandBuffer buf) {
        _image->allocate(buf);
    }

    void Custom::deallocate() { 
        _image->deallocate();
    }

    void Custom::post_init() {
    }

    bool Custom::update(ExecutionType type) {
        bool update = false;

        for (auto&& pmap : _params) {
            for (auto&& el : pmap.second) {
                if (el.second->changed()) {
                    update = true;
                }
            }
        }

        if (update) {
            if (_recompile->as<bool>().get()) {
                _recompile->as<bool>().set(false);
                auto image = _image_node->get_output_image();
                _make_shader(image, _image, _custom_kernel->as<std::string>().get());
            }
        }

        return update;
    }

    void Custom::execute(ExecutionType type, const SemaphorePtr& wait_semaphore, Fence * fence) {
        begin_command_buffer(_compute_command_buffer);
        _custom->dispatch(_compute_command_buffer, _size.x, _size.y);
        end_command_buffer(_compute_command_buffer);
        submit_compute_buffer(*_device, _compute_command_buffer, wait_semaphore, _compute_complete, fence);
    }
}