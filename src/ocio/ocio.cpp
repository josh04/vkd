#include <iostream>

#include "ocio.hpp"
#include "ocio_static.hpp"
#include "ocio_functional.hpp"
#include "image.hpp"
#include "compute/kernel.hpp"

#include "inputs/image_uploader.hpp"


namespace vkd {    

    REGISTER_NODE("ocio", "ocio", Ocio);

    Ocio::Ocio() = default;
    Ocio::~Ocio() = default;

    void Ocio::_make_shader(const std::shared_ptr<Image>& inp, const std::shared_ptr<Image>& outp, int src_index, int dst_index) {
        _ocio_images.clear();
        _kernel = ocio_functional::make_shader(*this, inp, outp, src_index, dst_index, _ocio_images);
    }

    void Ocio::init() {
        

        _size = {0, 0};
        
        
        auto image = _image_node->get_output_image();
        _size = image->dim();

        _image = vkd::Image::float_image(_device, _size);

        _src_param = ocio_functional::make_ocio_param(*this, "src space");
        _src_param->order(0);
        _dst_param = ocio_functional::make_ocio_param(*this, "dst space");
        _dst_param->order(1);

        _make_shader(image, _image, _src_param->as<int>().get(), _dst_param->as<int>().get());
    }

    void Ocio::allocate(VkCommandBuffer buf) {
        _image->allocate(buf);
    }

    void Ocio::deallocate() { 
        _image->deallocate();
    }

    bool Ocio::update(ExecutionType type) {
        bool update = false;

        for (auto&& pmap : _params) {
            for (auto&& el : pmap.second) {
                if (el.second->changed()) {
                    update = true;
                }
            }
        }

        if (update) {
            auto image = _image_node->get_output_image();
            _make_shader(image, _image, _src_param->as<int>().get(), _dst_param->as<int>().get());
        }

        return update;
    }

    void Ocio::execute(ExecutionType type, Stream& stream) {
        command_buffer().begin();
        _kernel->dispatch(command_buffer(), _size.x, _size.y);
        command_buffer().end();

        stream.submit(command_buffer());
    }

}