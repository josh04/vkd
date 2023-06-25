#include <mutex>
#include <thread>
#include "sane.hpp"
#include "image.hpp"
#include "command_buffer.hpp"
#include "buffer.hpp"
#include "compute/kernel.hpp"
#include "imgui/imgui.h"
#include "ImfThreading.h"
#include "ImfRgbaFile.h"

#include "sane/sane_wrapper.hpp"
#include "sane/sane_service.hpp"
#include "host_cache.hpp"

#include "ocio/ocio_functional.hpp"
#include "ocio/ocio_static.hpp"

namespace vkd {
    REGISTER_NODE("sane", "sane", Sane);
    
    Sane::Sane() : _block() {
    }

    void Sane::allocate(VkCommandBuffer buf) {
        _uploader->allocate(buf);
    }

    void Sane::deallocate() { 
        _uploader->deallocate();
    }

    void Sane::post_setup() {
        _block = BlockEditParams{_params, param_hash_name()};

        _new_scan = make_param<ParameterType::p_bool>(*this, "scan", 0, {"button"});

        auto devices = sane::Service::Get().devices();
        
        _scan_device = make_param<int>(*this, "sane device", 0, {"enum"});
        _scan_device->as<int>().max(devices.size());
        _scan_device->as<int>().set_default(0);
        _scan_device->enum_names(devices);

        _format_width = make_param<int>(*this, "_format_width", 0, {});
        _format_height = make_param<int>(*this, "_format_height", 0, {});
        _format_format = make_param<int>(*this, "_format_format", 0, {});
    }

    Sane::~Sane() {
    }

    void Sane::init() {
        try {
            read_options();
        } catch (UpdateException& e) {
            throw GraphException{e.what()};
        }

        _format.width = _format_width->as<int>().get();
        _format.height = _format_height->as<int>().get();
        _format.format = (ImageUploader::InFormat)_format_format->as<int>().get();
        
        try {
            auto f = sane::Service::Get().format(_scan_device->as<int>().get(), _dynamic_params);
            if (f.has_value()) {
                _format = *f;
                _format_width->as<int>().set_force(_format.width);
                _format_height->as<int>().set_force(_format.height);
                _format_format->as<int>().set_force((int)_format.format);
            } else {
                throw GraphException("Sane no format");
            }
        } catch (RebakeException& e) {
        } catch (UpdateException& e) {
        }
        recreate_uploader();

        _ocio = std::make_unique<OcioNode>(OcioNode::Type::In);
        _ocio->init(*this, ocio_functional::scan_space_index());
    }

    void Sane::clear_dynamic_params() {
        for (auto&& param : _dynamic_params) {
            _params["_"].erase(param.second->name()); 
        }
        _dynamic_params.clear();
    }

    void Sane::read_options() {
        clear_dynamic_params();

        // service params
        _dynamic_params = sane::Service::Get().params(*this, _scan_device->as<int>().get());

        update_params();
        
    }

    void Sane::recreate_uploader() {
        if (_format.width < 1 || _format.height < 1) {
            throw GraphException("Bad format at Sane node.");
        }

        _uploader = std::make_unique<ImageUploader>(_device);
        
        _uploader->init(_format.width, _format.height, _format.format, ImageUploader::OutFormat::float32, param_hash_name());

        for (auto&& kern : _uploader->kernels()) {
            register_params(*kern);
        }

        console << "recreated uploader" << std::endl;
    }

    void Sane::sane_scan() {
        try {
            auto cr = sane::Service::Get().scan(_scan_device->as<int>().get(), _dynamic_params);
            if (!cr.has_value()) {
                console << "Sane scan did not produce image." << std::endl;
            } else {
                _device->host_cache().add(hash(), std::move(*cr));
            }
        } catch (UpdateException& e) {
            console << e.what() << std::endl;
        }

        _scan_complete = true;
    }

    void Sane::queue_new_scan() {
        _device->host_cache().remove(hash());
        if (!_uploader) {
            throw UpdateException("Failed to launch scan.");
        }
        _scan_complete = false;
        auto task = std::make_unique<enki::TaskSet>(1, [this](enki::TaskSetPartition range, uint32_t threadnum) mutable {
            sane_scan();
        });
        _process_task = task.get();
        ts().add(std::move(task));
        throw PendingException{"SANE processing..."};
    }

    bool Sane::working() const {
        if (_process_task && !ts().is_complete(_process_task)) {
            return true;
        }
        return false;
    }

    bool Sane::update(ExecutionType type) {
        
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

            if (_new_scan->as<bool>().get()) {
                _new_scan->as<bool>().set(false);

                if (_uploader) {
                    queue_new_scan();
                }
            }

            bool update_sane = false;
            for (auto&& dyn_param : _dynamic_params) {
                if (dyn_param.second->changed()) {
                    update_sane = true;
                }
            }

            if (update_sane) {
                auto f = sane::Service::Get().format(_scan_device->as<int>().get(), _dynamic_params);
                if (f.has_value() && *f == _format) {
                    
                } else {
                    throw RebakeException("Sane option changed");
                }
            }

            _ocio->update(*this, _uploader->get_gpu());
        }

        if (!_uploader) {
            throw GraphException("Uploader not valid in SANE node. Perhaps you need to configure your scanner options?");
        }

        if (_scan_complete.exchange(false)) {
            update = true;
            //throw GraphException("Scan complete.");
        }

        auto ptr = _device->host_cache().get(hash());
        if (ptr) {
            if (_uploader->get_main_size() != ptr->size()) {
                console << "Uploader size " << _uploader->get_main_size() << " wasn't equal to host image size " << ptr->size() << " at Sane node. " << std::endl;
            }
            memcpy(_uploader->get_main(), ptr->data(), std::min(ptr->size(), _uploader->get_main_size()));
        } else {
            throw UpdateException("No image at SANE node yet.");
        }

        return update;
    }

    void Sane::execute(ExecutionType type, Stream& stream) {

        command_buffer().begin();
        _uploader->commands(command_buffer());
        _ocio->execute(command_buffer(), _format.width, _format.height);
        command_buffer().end();

        stream.submit(command_buffer());
    }

}