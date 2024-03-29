#include <mutex>
#include <thread>
#include "exr.hpp"
#include "image.hpp"
#include "command_buffer.hpp"
#include "buffer.hpp"
#include "compute/kernel.hpp"
#include "imgui/imgui.h"
#include "ImfThreading.h"
#include "ImfRgbaFile.h"

#include "ocio/ocio_functional.hpp"

namespace vkd {
    REGISTER_NODE("exr", "exr", Exr);

    Exr::Exr() : _block() {

        static std::once_flag initFlag;
        std::call_once(initFlag, []() {
            Imf::setGlobalThreadCount(std::thread::hardware_concurrency());
        });

    }

    void Exr::allocate(VkCommandBuffer buf) {
        _uploader->allocate(buf);
    }

    void Exr::deallocate() { 
        _uploader->deallocate();
    }

    void Exr::post_setup() {
        _block = BlockEditParams{_params, param_hash_name()};
        _path_param = make_param<ParameterType::p_string>(param_hash_name(), "path", 0);
        _path_param->as<std::string>().set_default("");
        _path_param->tag("filepath");

        _frame_param = make_param<ParameterType::p_frame>(param_hash_name(), "frame", 0);
        _single_frame = make_param<ParameterType::p_bool>(param_hash_name(), "single_frame", 0);
        
        _params["_"].emplace(_path_param->name(), _path_param);
        _params["_"].emplace(_frame_param->name(), _frame_param);
        _params["_"].emplace(_single_frame->name(), _single_frame);
    }

    Exr::~Exr() {
    }

    void Exr::init() {
        if (_path_param->as<std::string>().get().size() < 3) {
            throw GraphException("No path provided to exr node.");
        } 

        try {
            Imf::RgbaInputFile in(_path_param->as<std::string>().get().c_str());
        
            Imath::Box2i win = in.dataWindow();
            
            Imath::V2i dim(win.max.x - win.min.x + 1, win.max.y - win.min.y + 1);
                
            int dx = win.min.x;
            int dy = win.min.y;

            _width = dim.x;
            _height = dim.y;

    /*
            _frame_count = _video_stream->nb_frames + 1 - ((_video_stream->nb_frames + 1) % 2);
            _block.total_frame_count->as<int>().set_force(_frame_count > 0 ? _frame_count : 100);
    */

            _uploader = std::make_unique<ImageUploader>(_device);
            _uploader->init(_width, _height, ImageUploader::InFormat::half_rgba, ImageUploader::OutFormat::float32, param_hash_name());
            for (auto&& kern : _uploader->kernels()) {
                register_params(*kern);
            }

            in.setFrameBuffer(reinterpret_cast<Imf::Rgba *>(_uploader->get_main()) - dx - dy * dim.x, 1, dim.x);
            in.readPixels(win.min.y, win.max.y);
        } catch (...) {
            throw GraphException("Error reading EXR file.");
        }

        _current_frame = 0;

        _ocio = std::make_unique<OcioNode>(OcioNode::Type::In);
        _ocio->init(*this);
    }

    bool Exr::update(ExecutionType type) {
        
        bool update = false;
        for (auto&& pmap : _params) {
            for (auto&& el : pmap.second) {
                if (el.second->changed()) {
                    update = true;
                }
            }
        }
        if (update) {
            if (_single_frame->as<bool>().get()) {

            }

            _ocio->update(*this, _uploader->get_gpu());
        }
/*
        auto frame = _frame_param->as<Frame>().get();

        auto start_block = _block.frame_start_block->as<Frame>().get();
        auto end_block = _block.frame_end_block->as<Frame>().get();

        if (frame.index < start_block.index || frame.index > end_block.index) {
            memset(_staging_buffer_ptr, 0, _buffer_size);
            if (!_blanked) {
                _blanked = true;
                update = true;
            }
            return update;
        }

        _blanked = false;
*/

        // Wait for new frame
        if (_decode_next_frame) {
            
            // read frame here
/*
            uint8_t * ptr = nullptr;

            for (int j = 0; j < _height; ++j) {
                memcpy((uint8_t*)_staging_buffer_ptr + j * _width * 4 *sizeof(uint16_t), ptr + j * _width * 4 * sizeof(uint16_t), _width * 4 * sizeof(uint16_t));
            }
*/
            _decode_next_frame = false;
            update = true;
        }


        return update;
    }

    void Exr::execute(ExecutionType type, Stream& stream) {
        command_buffer().begin();
        _uploader->commands(command_buffer());
        _ocio->execute(command_buffer(), _width, _height);
        command_buffer().end();

        stream.submit(command_buffer());
    }

}