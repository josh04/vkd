#pragma once
        
#include <memory>

#include "buffer.hpp"
#include "image.hpp"
#include "compute/kernel.hpp"

namespace vkd {
    class ImageDownloader {
    public:
        ImageDownloader(const std::shared_ptr<Device>& device) : _device(device) {}
        ~ImageDownloader() = default;
        ImageDownloader(ImageDownloader&&) = delete;
        ImageDownloader(const ImageDownloader&) = delete;

        enum class OutFormat {
            yuv420p,
            half_rgba,
            uint8_rgba,
            uint8_rgbx
        };

        void init(const std::shared_ptr<Image>& image, OutFormat ofmt, std::string param_hash_name);
        void commands(CommandBuffer& buf);
        void commands(VkCommandBuffer buf);
        void execute();

        std::vector<std::shared_ptr<Kernel>> kernels() const {
            std::vector<std::shared_ptr<Kernel>> ks;
            if (_quantise_luma) { ks.push_back(_quantise_luma); }
            if (_quantise_chroma_u) { ks.push_back(_quantise_chroma_u); }
            if (_quantise_chroma_v) { ks.push_back(_quantise_chroma_v); }
            if (_image_to_half_buffer) { ks.push_back(_image_to_half_buffer); }
            if (_image_to_uint8_rgba_buffer) { ks.push_back(_image_to_uint8_rgba_buffer); }
            if (_image_to_uint8_rgbx_buffer) { ks.push_back(_image_to_uint8_rgbx_buffer); }
            return ks;
        }

        void * get_main() const { return _staging_buffer->get(); }

        void allocate(VkCommandBuffer buf);
        void deallocate();
    private:
        size_t _buffer_size() const;
        OutFormat _ofmt = OutFormat::yuv420p;

        std::shared_ptr<Device> _device = nullptr;

        int32_t _width = 1, _height = 1;

        std::shared_ptr<AutoMapStagingBuffer> _staging_buffer = nullptr;
        std::shared_ptr<StorageBuffer> _gpu_buffer = nullptr;
        std::shared_ptr<Image> _image = nullptr;

        // yuv420p
        std::shared_ptr<Kernel> _quantise_luma = nullptr;
        std::shared_ptr<Kernel> _quantise_chroma_u = nullptr;
        std::shared_ptr<Kernel> _quantise_chroma_v = nullptr;

        // half

        std::shared_ptr<Kernel> _image_to_half_buffer = nullptr;
        std::shared_ptr<Kernel> _image_to_uint8_rgba_buffer = nullptr;
        std::shared_ptr<Kernel> _image_to_uint8_rgbx_buffer = nullptr;
        std::shared_ptr<Kernel> _image_to_uint8_rgb_buffer = nullptr;
    };
}