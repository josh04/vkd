#include "image_uploader.hpp"
#include "command_buffer.hpp"

namespace vkd {
    
    void ImageUploader::init(int32_t width, int32_t height, InFormat ifmt, OutFormat ofmt, std::string param_hash_name) {

        _width = width;
        _height = height;
        _ifmt = ifmt;
        _ofmt = ofmt;

        auto buffer_size = _buffer_size();

        _staging_buffer = AutoMapStagingBuffer::make(_device, AutoMapStagingBuffer::Mode::Upload, buffer_size);

        _gpu_buffer = std::make_shared<StorageBuffer>(_device);
        _gpu_buffer->debug_name(param_hash_name + " UL (GPU Buffer)");
        _gpu_buffer->create(buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT);

        _image = Image::float_image(_device, {_width, _height}, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

        if (_ifmt == InFormat::yuv420p) {
            _yuv420 = std::make_shared<Kernel>(_device, param_hash_name);
            _yuv420->init("shaders/compute/yuv420.comp.spv", "main", Kernel::default_local_sizes);
            //register_params(*_yuv420);
            _yuv420->set_arg(0, _gpu_buffer);
            _yuv420->set_arg(1, _image);
        } else if (_ifmt == InFormat::half_rgba) {
            _half_buffer_to_image = std::make_shared<Kernel>(_device, param_hash_name);
            _half_buffer_to_image->init("shaders/compute/half_buffer_to_image.comp.spv", "main", Kernel::default_local_sizes);
            //register_params(*_half_buffer_to_image);
            _half_buffer_to_image->set_arg(0, _gpu_buffer);
            _half_buffer_to_image->set_arg(1, _image);
        } else if (_ifmt == InFormat::bayer_short) {
            _bayer = std::make_shared<Kernel>(_device, param_hash_name);
            _bayer->init("shaders/compute/short_buffer_to_image.comp.spv", "main", Kernel::default_local_sizes);
            //register_params(*_half_buffer_to_image);
            _bayer->set_arg(0, _gpu_buffer);
            _bayer->set_arg(1, _image);
        } else if (_ifmt == InFormat::libraw_short) {
            _libraw_short = std::make_shared<Kernel>(_device, param_hash_name);
            _libraw_short->init("shaders/compute/libraw_short.comp.spv", "main", Kernel::default_local_sizes);
            _libraw_short->set_arg(0, _gpu_buffer);
            _libraw_short->set_arg(1, _image);
        } else if (_ifmt == InFormat::r8) {
            _r8 = std::make_shared<Kernel>(_device, param_hash_name);
            _r8->init("shaders/compute/r8_buffer_to_image.comp.spv", "main", Kernel::default_local_sizes);
            _r8->set_arg(0, _gpu_buffer);
            _r8->set_arg(1, _image);
        } else if (_ifmt == InFormat::rgb8) {
            _rgb8 = std::make_shared<Kernel>(_device, param_hash_name);
            _rgb8->init("shaders/compute/rgb8_buffer_to_image.comp.spv", "main", Kernel::default_local_sizes);
            _rgb8->set_arg(0, _gpu_buffer);
            _rgb8->set_arg(1, _image);
        }  else if (_ifmt == InFormat::r16) {
            _r16 = std::make_shared<Kernel>(_device, param_hash_name);
            _r16->init("shaders/compute/r16_buffer_to_image.comp.spv", "main", Kernel::default_local_sizes);
            _r16->set_arg(0, _gpu_buffer);
            _r16->set_arg(1, _image);
        } else if (_ifmt == InFormat::rgb16) {
            _rgb16 = std::make_shared<Kernel>(_device, param_hash_name);
            _rgb16->init("shaders/compute/rgb16_buffer_to_image.comp.spv", "main", Kernel::default_local_sizes);
            _rgb16->set_arg(0, _gpu_buffer);
            _rgb16->set_arg(1, _image);
        } 
    }

    void ImageUploader::allocate(VkCommandBuffer buf) {
        _image->allocate(buf);
        _gpu_buffer->allocate();
    }

    void ImageUploader::deallocate() { 
        _image->deallocate();
        _gpu_buffer->deallocate();
    }

    size_t ImageUploader::_buffer_size() const {
        if (_ifmt == InFormat::half_rgba) {
            return _width * _height * 4 * sizeof(uint16_t);
        } else if (_ifmt == InFormat::yuv420p) {
            return _width * _height * 1.5 * sizeof(uint8_t);
        } else if (_ifmt == InFormat::bayer_short) {
            return _width * _height * 4 * sizeof(uint16_t);
        } else if (_ifmt == InFormat::libraw_short) {
            return _width * _height * 4 * sizeof(uint16_t);
        } else if (_ifmt == InFormat::r8) {
            return _width * _height * 1 * sizeof(uint8_t);
        } else if (_ifmt == InFormat::rgb8) {
            return _width * _height * 3 * sizeof(uint8_t);
        } else if (_ifmt == InFormat::r16) {
            return _width * _height * 1 * sizeof(uint16_t);
        } else if (_ifmt == InFormat::rgb16) {
            return _width * _height * 3 * sizeof(uint16_t);
        }
    }

    VkFormat ImageUploader::_output_format() const {
        if (_ofmt == OutFormat::half16) {
            return VK_FORMAT_R16G16B16A16_SFLOAT;
        } else if (_ofmt == OutFormat::float32) {
            return VK_FORMAT_R32G32B32A32_SFLOAT;
        }
    }

    void ImageUploader::commands(CommandBuffer& buf) {
        commands(buf.get());
    }

    void ImageUploader::commands(VkCommandBuffer buf) {
        _gpu_buffer->copy(*_staging_buffer, _buffer_size(), buf);
        _gpu_buffer->barrier(buf, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

        if (_ifmt == InFormat::half_rgba) {
            _half_buffer_to_image->dispatch(buf, _width, _height);
        } else if (_ifmt == InFormat::yuv420p) {
            _yuv420->dispatch(buf, _width, _height);
        } else if (_ifmt == InFormat::bayer_short) {
            _bayer->dispatch(buf, _width, _height);
        } else if (_ifmt == InFormat::libraw_short) {
            _libraw_short->dispatch(buf, _width, _height);
        } else if (_ifmt == InFormat::r8) {
            _r8->dispatch(buf, _width, _height);
        } else if (_ifmt == InFormat::rgb8) {
            _rgb8->dispatch(buf, _width, _height);
        } else if (_ifmt == InFormat::r16) {
            _r16->dispatch(buf, _width, _height);
        } else if (_ifmt == InFormat::rgb16) {
            _rgb16->dispatch(buf, _width, _height);
        }
    }

    void ImageUploader::execute() {
        auto buf = begin_immediate_command_buffer(_device->logical_device(), _device->command_pool());
        commands(buf);
        flush_command_buffer(_device->logical_device(), _device->queue(), _device->command_pool(), buf);
    }
}