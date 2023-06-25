#include "crop.hpp"
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
    REGISTER_NODE("crop", "crop", Crop);

    void Crop::init() {
        

        _size = {0, 0};
        
        _crop = Kernel::make(*this, "shaders/compute/crop.comp.spv", "main", Kernel::default_local_sizes);
        
        
        auto image = _image_node->get_output_image();
        _size = image->dim();

        _image = Image::float_image(_device, _size, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

        _crop_margin = make_param<glm::ivec4>(*this, "crop", 0);
        _crop_margin->as<glm::ivec4>().set_default(glm::ivec4{0, 0, _size.x - 1, _size.y - 1});
        _crop_margin->as<glm::ivec4>().soft_min(glm::ivec4{0, 0, 0, 0});
        _crop_margin->as<glm::ivec4>().soft_max(glm::ivec4{_size.x - 1, _size.y - 1, _size.x - 1, _size.y - 1});

        _crop_offset = _crop->get_param_by_name("_crop_offset");
        _crop_offset->as<glm::ivec2>().set_default({0, 0});

        _crop->set_arg(0, image);
        _crop->set_arg(1, _image);
    }

    void Crop::allocate(VkCommandBuffer buf) {
        _image->allocate(buf);
    }

    void Crop::deallocate() { 
        _image->deallocate();
    }

    bool Crop::update(ExecutionType type) {
        bool update = false;

        for (auto&& pmap : _params) {
            for (auto&& el : pmap.second) {
                if (el.second->changed()) {
                    update = true;
                }
            }
        }

        if (update) {
            auto crop = _crop_margin->as<glm::ivec4>().get();
            _crop_offset->as<glm::ivec2>().set_force({crop.x, crop.y});
            //_check_param_sanity();
        }

        return update;
    }

    void Crop::execute(ExecutionType type, Stream& stream) {

        auto crop = _crop_margin->as<glm::ivec4>().get();

        if (crop.z - crop.x > 0 && crop.w - crop.y > 0) {

            command_buffer().begin();
            _crop->dispatch(command_buffer(), crop.z - crop.x, crop.w - crop.y);
            command_buffer().end();
            stream.submit(command_buffer());

        }
    }
}