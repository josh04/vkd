#include "catch.hpp"
#include "ocio/ocio.hpp"
#include "shader.hpp"
#include "compute/kernel.hpp"
#include "instance.hpp"
#include "device.hpp"
/*
TEST_CASE( "Test OCIO object creation", "[ocio]" ) {
    vkd::OcioTest ocio;
    ocio.load("aces_1.2/config.ocio");
}
*/
TEST_CASE("Test OCIO kernel compilation", "[ocio]") {
    std::string kernel_text = R"src(#version 450 // add this automatically

// Binding 0 : Position storage buffer
layout(binding = 0, rgba32f) uniform image2D inputTex;
layout(binding = 1, rgba32f) uniform image2D outputTex;

layout (local_size_x_id = 0, local_size_y_id = 1, local_size_z_id = 2) in;

layout (push_constant) uniform PushConstants {
    ivec4 vkd_offset;
} push;

vec4 OCIOMain(vec4 inp) 
{
     return inp;
}

void main() 
{
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy) + push.vkd_offset.xy;
    
    vec4 inp = imageLoad(inputTex, coord);
    vec4 inp2 = OCIOMain(inp);

    imageStore(outputTex, coord, inp2);
}
        
        )src";

        auto instance = vkd::createInstance(true);
        auto device = std::make_shared<vkd::Device>(instance);
        device->create(instance->get_physical_device());

        auto shader = std::make_unique<vkd::ManualComputeShader>(device);
        shader->create(kernel_text, "main");
        auto kernel = std::make_shared<vkd::Kernel>(device, "");
        kernel->init(std::move(shader), "OCIO_SHADER", {16, 16, 1});


}