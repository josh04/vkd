#include "exr.hpp"
#include <mutex>
#include "image.hpp"
#include "command_buffer.hpp"
#include "buffer.hpp"
#include "compute/kernel.hpp"

#include "image_downloader.hpp"

#include "compute/image_node.hpp"

#include "ImfRgbaFile.h"
#include "ghc/filesystem.hpp"

namespace vkd {
    REGISTER_NODE("exr_output", "exr_output", ExrOutput);

    ExrOutput::ExrOutput() {
        _path_param = make_param<ParameterType::p_string>(param_hash_name(), "path", 0);
        _path_param->as<std::string>().set_default("");
        _path_param->tag("filepath");

        _params["_"].emplace(_path_param->name(), _path_param);
    }

    ExrOutput::~ExrOutput() {
    }

    void ExrOutput::init() {
        auto image = _input_node->get_output_image();
        auto sz = image->dim();

        _width = sz[0];
        _height = sz[1];

        _downloader = std::make_unique<ImageDownloader>(_device);
        _downloader->init(_input_node->get_output_image(), ImageDownloader::OutFormat::half_rgba, param_hash_name());
        for (auto&& kern : _downloader->kernels()) {
            register_params(*kern);
        }

        _compute_command_buffer = create_command_buffer(_device->logical_device(), _device->command_pool());
        _compute_complete = Semaphore::make(_device);

        begin_command_buffer(_compute_command_buffer);
        _downloader->commands(_compute_command_buffer);
        end_command_buffer(_compute_command_buffer);
        
        std::string path = _path_param->as<std::string>().get();

        if (path.size() < 3) {
            throw GraphException("No path provided to exr node.");
        } 

        _fence = Fence::create(_device, false);
    }

    void ExrOutput::post_init()
    {
    }

    bool ExrOutput::update(ExecutionType type) {
        bool updated = false;

        for (auto&& pmap : _params) {
            for (auto&& el : pmap.second) {
                if (el.second->changed()) {
                    updated = true;
                }
            }
        }

        return updated;
    }

    void ExrOutput::execute(ExecutionType type, const SemaphorePtr& wait_semaphore, Fence * fence) {
        if (type != ExecutionType::Execution) {
            submit_compute_buffer(*_device, VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE, fence);
            return;
        }

        submit_compute_buffer(*_device, _compute_command_buffer, wait_semaphore, _compute_complete, _fence.get());
        
        if (_fence) {
            _fence->wait();
            _fence->reset();        
        }

        uint8_t * buffer = (uint8_t *)_downloader->get_main();

        //if (pixelBuffer == NULL) {throw Exception("Buffer NULL. Not created or deleted.");}
	
        Imf::RgbaOutputFile file(_filename().c_str(), _width, _height, Imf::WRITE_RGBA, 1.0f, Imath::V2f (0, 0), 1.0f, Imf::INCREASING_Y, Imf::ZIP_COMPRESSION);
        file.setFrameBuffer((Imf::Rgba* )buffer, 1, _width);
        file.writePixels(_height);

        _frame_count++;

        submit_compute_buffer(*_device, VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE, fence);
    }

    std::string ExrOutput::_filename() const {
        ghc::filesystem::path path{_path_param->as<std::string>().get()};

        path.replace_extension("exr");
        path.replace_filename(path.stem().string() + std::string("_") + std::to_string(_frame_count));
        path.replace_extension("exr");
        
        return path.string();
    }

}