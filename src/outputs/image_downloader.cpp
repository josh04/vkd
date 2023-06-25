#include "image_downloader.hpp"
#include "command_buffer.hpp"

namespace vkd {
    
    void ImageDownloader::init(const std::shared_ptr<Image>& image, OutFormat ofmt, std::string param_hash_name) {
        _image = image;
        auto sz = image->dim();
        _width = sz[0];
        _height = sz[1];
        _ofmt = ofmt;

        auto buffer_size = _buffer_size();

        _staging_buffer = AutoMapStagingBuffer::make(_device, AutoMapStagingBuffer::Mode::Download, buffer_size);

        _gpu_buffer = std::make_shared<StorageBuffer>(_device);
        _gpu_buffer->debug_name(param_hash_name + " DL (GPU Buffer)");
        _gpu_buffer->create(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

        if (_ofmt == OutFormat::yuv420p) {
            
            _quantise_luma = std::make_shared<Kernel>(_device, param_hash_name);
            _quantise_luma->init("shaders/output/quantise_luma.comp.spv", "main", Kernel::default_local_sizes);
            _quantise_luma->set_push_arg_by_name("_size", glm::ivec2(_width, _height));

            //_quantise_luma->set_arg(0, _gpu_buffer);
            _quantise_luma->set_arg(0, image);
            _quantise_luma->set_arg(1, _gpu_buffer);

            _quantise_chroma_u = std::make_shared<Kernel>(_device, param_hash_name);
            _quantise_chroma_u->init("shaders/output/quantise_chroma_u.comp.spv", "main", Kernel::default_local_sizes);
            _quantise_chroma_u->set_push_arg_by_name("_size", glm::ivec2(_width, _height));
            _quantise_chroma_u->set_arg(0, image);
            _quantise_chroma_u->set_arg(1, _gpu_buffer);

            _quantise_chroma_v = std::make_shared<Kernel>(_device, param_hash_name);
            _quantise_chroma_v->init("shaders/output/quantise_chroma_v.comp.spv", "main", Kernel::default_local_sizes);
            _quantise_chroma_v->set_push_arg_by_name("_size", glm::ivec2(_width, _height));
            _quantise_chroma_v->set_arg(0, image);
            _quantise_chroma_v->set_arg(1, _gpu_buffer);

        } else if (_ofmt == OutFormat::half_rgba) {
            _image_to_half_buffer = std::make_shared<Kernel>(_device, param_hash_name);
            _image_to_half_buffer->init("shaders/compute/image_to_half_buffer.comp.spv", "main", Kernel::default_local_sizes);
            _image_to_half_buffer->set_arg(0, image);
            _image_to_half_buffer->set_arg(1, _gpu_buffer);
        } else if (_ofmt == OutFormat::uint8_rgba) {
            _image_to_uint8_rgba_buffer = std::make_shared<Kernel>(_device, param_hash_name);
            _image_to_uint8_rgba_buffer->init("shaders/compute/image_to_uint8_rgba_buffer.comp.spv", "main", Kernel::default_local_sizes);
            _image_to_uint8_rgba_buffer->set_arg(0, image);
            _image_to_uint8_rgba_buffer->set_arg(1, _gpu_buffer);
            _image_to_uint8_rgba_buffer->set_push_arg_by_name("_srgb", 1);
        }  else if (_ofmt == OutFormat::uint8_rgbx) {
            _image_to_uint8_rgbx_buffer = std::make_shared<Kernel>(_device, param_hash_name);
            _image_to_uint8_rgbx_buffer->init("shaders/compute/image_to_uint8_rgbx_buffer.comp.spv", "main", Kernel::default_local_sizes);
            _image_to_uint8_rgbx_buffer->set_arg(0, image);
            _image_to_uint8_rgbx_buffer->set_arg(1, _gpu_buffer);
            _image_to_uint8_rgbx_buffer->set_push_arg_by_name("_srgb", 1);
        } 

    }

    size_t ImageDownloader::_buffer_size() const {
        if (_ofmt == OutFormat::yuv420p) {
            return _width * _height * 1.5 * sizeof(uint8_t);
        } else if (_ofmt == OutFormat::half_rgba) {
            return _width * _height * 4 * sizeof(uint16_t);
        } else if (_ofmt == OutFormat::uint8_rgba || _ofmt == OutFormat::uint8_rgbx) {
            return _width * _height * 4 * sizeof(uint8_t);
        }
    }

    void ImageDownloader::commands(CommandBuffer& buf) {
        commands(buf.get());
    }

    void ImageDownloader::commands(VkCommandBuffer buf) {
        if (_ofmt == OutFormat::half_rgba) {
            _image_to_half_buffer->dispatch(buf, _width, _height);
        } else if (_ofmt == OutFormat::uint8_rgba) {
            _image_to_uint8_rgba_buffer->dispatch(buf, _width, _height);
        } else if (_ofmt == OutFormat::uint8_rgbx) {
            _image_to_uint8_rgbx_buffer->dispatch(buf, _width, _height);
        } else if (_ofmt == OutFormat::yuv420p) {
            _quantise_luma->dispatch(buf, (int32_t)std::ceil(_width / 4.0f), _height);
            _quantise_chroma_u->dispatch(buf, (int32_t)std::ceil(_width / 8.0f), _height/2);
            _quantise_chroma_v->dispatch(buf, (int32_t)std::ceil(_width / 8.0f), _height/2);
        }

        _gpu_buffer->barrier(buf, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        _staging_buffer->copy(*_gpu_buffer, _buffer_size(), buf);
        _gpu_buffer->barrier(buf, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    }

    void ImageDownloader::execute() {
        auto buf = begin_immediate_command_buffer(_device->logical_device(), _device->command_pool());
        allocate(buf);
        commands(buf);
        deallocate();
        flush_command_buffer(_device->logical_device(), _device->queue(), _device->command_pool(), buf);
    }

    void ImageDownloader::allocate(VkCommandBuffer buf) {
        _gpu_buffer->allocate();
    }

    void ImageDownloader::deallocate() { 
        _gpu_buffer->deallocate();
    }
}