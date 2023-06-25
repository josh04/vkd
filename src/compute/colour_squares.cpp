#include "colour_squares.hpp"
#include "kernel.hpp"
#include "image.hpp"
#include "command_buffer.hpp"

namespace vkd {
    REGISTER_NODE("squares", "squares", ColourSquares, 1280, 720);

    ColourSquares::ColourSquares(int32_t width, int32_t height) : _width(width), _height(height) {}

    void ColourSquares::init() {
        
        
        _image = std::make_shared<vkd::Image>(_device);
        _image->create_image(VK_FORMAT_R32G32B32A32_SFLOAT, {_width, _height}, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT );
        _image->allocate(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        _image->create_view(VK_IMAGE_ASPECT_COLOR_BIT);
        
        auto buf = vkd::begin_immediate_command_buffer(_device->logical_device(), _device->command_pool());

        _image->set_layout(buf, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

        vkd::flush_command_buffer(_device->logical_device(), _device->queue(), _device->command_pool(), buf);

        _square = std::make_shared<Kernel>(_device, param_hash_name());
        _square->init("shaders/compute/square.comp.spv", "main", Kernel::default_local_sizes);
        register_params(*_square);
        _square->set_arg(0, _image);


        struct SquareData {
            glm::vec4 col;
        };
        SquareData d1;
        d1.col = glm::vec4{0.706f, 0.678f, 0.639f, 1.0f};
        _square->set_push_arg_by_name(command_buffer().get(), "col", d1.col);
    }

    bool ColourSquares::update(ExecutionType type) {
        bool update = false;
        for (auto&& pmap : _params) {
            for (auto&& el : pmap.second) {
                if (el.second->changed()) {
                    update = true;
                }
            }
        }

        if (update) {
            command_buffer().begin();
            glm::ivec4 rect = {10, 10, 40, 40};
            glm::ivec4 rect2 = {10, 50, 160, 160};

            _square->set_offset({rect.x, rect.y});
            _square->dispatch(command_buffer(), rect.z, rect.w);
            
            _square->set_offset({rect2.x, rect2.y});
            _square->dispatch(command_buffer(), rect2.z, rect2.w);

            command_buffer().end();
        }
        return update;
    }

    void ColourSquares::execute(ExecutionType type, Stream& stream) {
        stream.submit(command_buffer());
    }
}