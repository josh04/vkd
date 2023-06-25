#include <mutex>
#include "raw.hpp"
#include "image.hpp"
#include "command_buffer.hpp"
#include "buffer.hpp"
#include "compute/kernel.hpp"
#include "imgui/imgui.h"
#include "host_cache.hpp"
#include "ocio/ocio_functional.hpp"
#include "ocio/ocio_static.hpp"

#include "host_scheduler.hpp"

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
        _info_box = make_param<ParameterType::p_string>(*this, "info_box", 0, {"multiline"});
        _info_box->as<std::string>().set_default("");

        _reset = make_param<ParameterType::p_bool>(*this, "reset", 0, {"button"});
    }

    Raw::~Raw() {
        if (_process_task) {
            ts().wait(_process_task);
        }
        
        _process_task = nullptr;
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

        

        _dcraw_colour_space = make_param<int>(*this, "dcraw colour", 0, {"enum"});
        /*

  static const char *name[] = {"sRGB",          "Adobe RGB (1998)",
                               "WideGamut D65", "ProPhoto D65",
                               "XYZ",           "ACES",
                               "DCI-P3 D65",    "Rec. 2020"};

        */

        std::vector<std::string> dcraw_names = {
            "srgb/rec709", "adobe rgb", 
            "widegamut d65", "prophoto d65", 
            "xyz", "aces 2065-1",
            "p3d65", "rec2020"
        };

        _dcraw_colour_space->as<int>().min(0);
        _dcraw_colour_space->as<int>().max(dcraw_names.size());
        _dcraw_colour_space->as<int>().set_default(5);
        _dcraw_colour_space->enum_names(dcraw_names);

        _uploader = std::make_unique<ImageUploader>(_device);
        _uploader->init(_width, _height, ImageUploader::InFormat::libraw_short, ImageUploader::OutFormat::float32, param_hash_name());
        for (auto&& kern : _uploader->kernels()) {
            register_params(*kern);
        }

        _uploader->set_push_arg_by_name("vkd_shortmax", (float)16000.0f);//imProc.imgdata.color.maximum);

        _current_frame = 0;

        _ocio = std::make_unique<OcioNode>(OcioNode::Type::In);
        _ocio->init(*this);

        std::unique_ptr<LibRaw> imProcPtr = std::make_unique<LibRaw>();
        auto&& imProc = *imProcPtr;
        imProc.open_file(_path_param->as<std::string>().get().c_str());
        _width = imProc.imgdata.sizes.width;
        _height = imProc.imgdata.sizes.height;

        _uploader->init(_width, _height, ImageUploader::InFormat::libraw_short, ImageUploader::OutFormat::float32, param_hash_name());
    }

    void Raw::load_to_uploader() {        
        auto ptr = _device->host_cache().get(hash());

        if (ptr) {
            auto dim = ptr->dim();
            _width = dim.x;
            _height = dim.y;
        } else {
            auto task = std::make_unique<enki::TaskSet>(1, [this](enki::TaskSetPartition range, uint32_t threadnum) mutable {
                libraw_process();
            });
            _process_task = task.get();
            ts().add(std::move(task));
            throw PendingException{"dcraw processing..."};
        }

        _current_dcraw_col_space = _dcraw_colour_space->as<int>().get();
        
        memcpy(_uploader->get_main(), ptr->data(), ptr->size());
    }

    bool Raw::working() const {
        if (_process_task && !ts().is_complete(_process_task)) {
            return true;
        }
        return false;
    }

    void Raw::libraw_process() {
        
        std::unique_ptr<LibRaw> imProcPtr = std::make_unique<LibRaw>();
        auto&& imProc = *imProcPtr;

        imProc.imgdata.params.output_bps = 16;
        imProc.imgdata.params.output_color = _dcraw_colour_space->as<int>().get() + 1;

        imProc.open_file(_path_param->as<std::string>().get().c_str());

        imProc.unpack();
        imProc.dcraw_process();

        _info_box->as<std::string>().set(get_metadata(imProc));

        auto cr = StaticHostImage::make(_width, _height, 4, sizeof(uint16_t));
        memcpy(cr->data(), imProc.imgdata.image, cr->size());

        _device->host_cache().add(hash(), std::move(cr));
    }

    void Raw::allocate(VkCommandBuffer buf) {
        _uploader->allocate(buf);
    }

    void Raw::deallocate() { 
        _uploader->deallocate();
    }

    bool Raw::update(ExecutionType type) {

        bool update = false;
        if (_process_task && !_process_task->GetIsComplete()) {
            return false;
        } else if (_process_task) {
            _process_task = nullptr;
            update = true;
        }
        
        for (auto&& pmap : _params) {
            for (auto&& el : pmap.second) {
                if (el.second->changed()) {
                    update = true;
                }
            }
        }
        if (update) {

            if (_current_dcraw_col_space != _dcraw_colour_space->as<int>().get()) {
                load_to_uploader();
            }

            if (_reset->as<bool>().get()) {
                _reset->as<bool>().set(false);

                Hash h{_path_param->as<std::string>().get(), _current_dcraw_col_space};
                _device->host_cache().remove(h);
            }

            _ocio->update(*this, _uploader->get_gpu());
        }

        return update;
    }

    void Raw::execute(ExecutionType type, Stream& stream) {
        command_buffer().begin();
        _uploader->commands(command_buffer());
        _ocio->execute(command_buffer(), _width, _height);
        command_buffer().end();

        stream.submit(command_buffer());
    }

}