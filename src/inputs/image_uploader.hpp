#pragma once
        
#include <memory>

#include "buffer.hpp"
#include "image.hpp"
#include "compute/kernel.hpp"

namespace vkd {
    class ImageUploader {
    public:
        ImageUploader(const std::shared_ptr<Device>& device) : _device(device) {}
        ~ImageUploader() = default;
        ImageUploader(ImageUploader&&) = delete;
        ImageUploader(const ImageUploader&) = delete;

        enum class InFormat {
            yuv420p,
            half_rgba,
            libraw_short,
            bayer_short
        };

        enum class OutFormat {
            half16,
            float32
        };

        void init(int32_t width, int32_t height, InFormat ifmt, OutFormat ofmt, std::string param_hash_name);
        void commands(VkCommandBuffer buf);
        void execute();

        std::vector<std::shared_ptr<Kernel>> kernels() const {
            std::vector<std::shared_ptr<Kernel>> ks;
            if (_yuv420) {
                ks.push_back(_yuv420);
            }
            if (_half_buffer_to_image) {
                ks.push_back(_half_buffer_to_image);
            }
            if (_bayer) {
                ks.push_back(_bayer);
            }
            if (_libraw_short) {
                ks.push_back(_libraw_short);
            }
            return ks;
        }

        template<typename T>
        void set_push_arg_by_name(const std::string& name, const T& value) {
            if (_yuv420) {
                _yuv420->set_push_arg_by_name(name, value);
            }
            if (_half_buffer_to_image) {
                _half_buffer_to_image->set_push_arg_by_name(name, value);
            }
            if (_bayer) {
                _bayer->set_push_arg_by_name(name, value);
            }
            if (_libraw_short) {
                _libraw_short->set_push_arg_by_name(name, value);
            }
        }

        void * get_main() const { return _staging_buffer->get(); }
        auto get_gpu() const { return _image; }

        void allocate(VkCommandBuffer buf);
        void deallocate();
    private:
        size_t _buffer_size() const;
        VkFormat _output_format() const;
        InFormat _ifmt = InFormat::yuv420p;
        OutFormat _ofmt = OutFormat::float32;

        std::shared_ptr<Device> _device = nullptr;

        int32_t _width = 1, _height = 1;

        std::shared_ptr<AutoMapStagingBuffer> _staging_buffer = nullptr;
        std::shared_ptr<StorageBuffer> _gpu_buffer = nullptr;
        std::shared_ptr<Image> _image = nullptr;

        std::shared_ptr<Kernel> _yuv420 = nullptr;
        std::shared_ptr<Kernel> _half_buffer_to_image = nullptr;
        std::shared_ptr<Kernel> _bayer = nullptr;
        std::shared_ptr<Kernel> _libraw_short = nullptr;
        
    };
}