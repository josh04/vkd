#include <mutex>
#include "raw.hpp"
#include "image.hpp"
#include "command_buffer.hpp"
#include "buffer.hpp"
#include "compute/kernel.hpp"
#include "imgui/imgui.h"
#include "host_cache.hpp"

#define LIBRAW_NO_WINSOCK2 
#include "libraw/libraw.h"

namespace vkd {
    REGISTER_NODE("raw", "raw", Raw);

    Raw::Raw() : _block() {

    }

    void Raw::post_setup() {
        _block = BlockEditParams{_params, param_hash_name()};
        _path_param = make_param<ParameterType::p_string>(*this, "path", 0, {"filepath"});
        _path_param->as<std::string>().set_default("");

        _frame_param = make_param<ParameterType::p_frame>(*this, "frame", 0);
        _single_frame = make_param<ParameterType::p_bool>(*this, "single_frame", 0);
        _info_box = make_param<ParameterType::p_string>(*this, "info_box", 0, {"multiline"});
        _info_box->as<std::string>().set_default("");

        _reset = make_param<ParameterType::p_bool>(*this, "reset", 0, {"button"});
    }

    Raw::~Raw() {
    }


    std::string Raw::get_metadata(LibRaw& imProc) const {
            std::stringstream strm;

            strm << "make: " << imProc.imgdata.idata.make << "\n";
            strm << "model: " << imProc.imgdata.idata.model << "\n";
            strm << "n-make: " << imProc.imgdata.idata.normalized_make << "\n";
            strm << "n-model: " << imProc.imgdata.idata.normalized_model << "\n";
            strm << "software: " << imProc.imgdata.idata.software << "\n";
            strm << "raw count: " << imProc.imgdata.idata.raw_count << "\n";
            strm << "colours: " << imProc.imgdata.idata.colors << "\n";
            strm << "filters: " << std::hex << imProc.imgdata.idata.colors << std::dec <<"\n";
            strm << "raw width: " << imProc.imgdata.sizes.raw_width <<"\n";        
            strm << "raw height: " << imProc.imgdata.sizes.raw_height <<"\n";
            strm << "width: " << imProc.imgdata.sizes.width <<"\n";        
            strm << "height: " << imProc.imgdata.sizes.height <<"\n";     
            strm << "black level: " << imProc.imgdata.color.black <<"\n";  
            strm << "c-black level: r: " << imProc.imgdata.color.cblack[0] << " g: " << imProc.imgdata.color.cblack[1] << " b: " << imProc.imgdata.color.cblack[2] << "\n";  
            strm << "c-black level: r2: " << imProc.imgdata.color.cblack[3] << " g2: " << imProc.imgdata.color.cblack[4] << " b2: " << imProc.imgdata.color.cblack[5] << "\n"; 
            strm << "data max: " << imProc.imgdata.color.data_maximum <<"\n";
            strm << "max: " << imProc.imgdata.color.maximum <<"\n";
            strm << "iso: " << imProc.imgdata.other.iso_speed <<"\n";
            strm << "shutter: " << imProc.imgdata.other.shutter <<"\n";
            strm << "aperture: " << imProc.imgdata.other.aperture <<"\n";
            strm << "focal length: " << imProc.imgdata.other.focal_len <<"\n";
            strm << "time: " << imProc.imgdata.other.timestamp <<"\n";
            strm << "num: " << imProc.imgdata.other.shot_order <<"\n";

            return strm.str();
    }

    void Raw::init() {
        
        if (_path_param->as<std::string>().get().size() < 3) {
            throw GraphException("No path provided to raw node.");
        } 

        _compute_command_buffer = create_command_buffer(_device->logical_device(), _device->command_pool());
        _compute_complete = Semaphore::make(_device);

        auto ptr = _device->host_cache().get(_path_param->as<std::string>().get());

        if (ptr) {
            auto dim = ptr->dim();
            _width = dim.x;
            _height = dim.y;
        } else {
            LibRaw imProc{};

            imProc.imgdata.params.output_bps = 16;
            imProc.imgdata.params.output_color = 0;

            imProc.open_file(_path_param->as<std::string>().get().c_str());

            _width = imProc.imgdata.sizes.width;
            _height = imProc.imgdata.sizes.height;

            _info_box->as<std::string>().set(get_metadata(imProc));

            imProc.unpack();
            imProc.dcraw_process();

            auto cr = StaticHostImage::make(_width, _height, 4, sizeof(uint16_t));
            ptr = cr.get();
            memcpy(ptr->data(), imProc.imgdata.image, ptr->size());
            _device->host_cache().add(_path_param->as<std::string>().get(), std::move(cr));
        }

        _uploader = std::make_unique<ImageUploader>(_device);
        _uploader->init(_width, _height, ImageUploader::InFormat::libraw_short, ImageUploader::OutFormat::float32, param_hash_name());
        for (auto&& kern : _uploader->kernels()) {
            register_params(*kern);
        }

        _uploader->set_push_arg_by_name("vkd_shortmax", (float)16000.0f);//imProc.imgdata.color.maximum);

        //in.setFrameBuffer(reinterpret_cast<Imf::Rgba *>(_uploader->get_main()) - dx - dy * dim.x, 1, dim.x);
        //in.readPixels(win.min.y, win.max.y);

        //memcpy(_uploader->get_main(), imProc.imgdata.image, sizeof(uint16_t) * 4 * _width * _height);
        memcpy(_uploader->get_main(), ptr->data(), ptr->size());

        _current_frame = 0;
    }

    void Raw::allocate(VkCommandBuffer buf) {
        _uploader->allocate(buf);
    }

    void Raw::deallocate() { 
        _uploader->deallocate();
    }

    void Raw::post_init()
    {
    }

    bool Raw::update(ExecutionType type) {
        
        bool update = false;
        for (auto&& pmap : _params) {
            for (auto&& el : pmap.second) {
                if (el.second->changed()) {
                    update = true;
                }
            }
        }
        if (update) {

            if (_reset->as<bool>().get()) {
                _reset->as<bool>().set(false);

                LibRaw imProc{};

                imProc.imgdata.params.output_bps = 16;
                imProc.imgdata.params.output_color = 0;

                imProc.open_file(_path_param->as<std::string>().get().c_str());

                _info_box->as<std::string>().set(get_metadata(imProc));
            }
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

    void Raw::execute(ExecutionType type, const SemaphorePtr& wait_semaphore, Fence * fence) {
        begin_command_buffer(_compute_command_buffer);
        _uploader->commands(_compute_command_buffer);
        end_command_buffer(_compute_command_buffer);

        _last_fence = fence;
        submit_compute_buffer(*_device, _compute_command_buffer, wait_semaphore, _compute_complete, fence);
    }

}