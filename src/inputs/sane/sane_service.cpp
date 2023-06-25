#include "sane_service.hpp"
#include "sane_wrapper.hpp"
#include "console.hpp"
#include "graph_exception.hpp"
#include "make_param.hpp"

namespace vkd {
    namespace sane {
        std::mutex Service::_mutex;
        std::unique_ptr<Service> Service::_singleton = nullptr;

        void Service::init() {
            try {
                sane_wrapper::init();
            } catch (std::runtime_error& e) {
                console << "SANE failed to load: " << e.what() << std::endl;
                throw ServiceException(e.what());
            }
        }

        void Service::shutdown() {
            sane_wrapper::clear_current_device();
        }

        std::vector<std::string> Service::devices() {
            std::scoped_lock lock(_sane_mutex);
            return _devices();
        }

        std::vector<std::string> Service::_devices() {
            auto devices = sane_wrapper::devices();
            std::vector<std::string> names;
            for (auto&& d : devices) {
                names.push_back(d.name);
            }
            return names;
        }

        std::map<int32_t, ParamPtr> Service::params(EngineNode& node, int32_t device) {
            std::scoped_lock lock(_sane_mutex);

            auto devices = _devices();
            sane_wrapper::set_current_device(device, devices[device]);

            auto options = sane_wrapper::options();

            std::map<int32_t, ParamPtr> dynamic_params;

            for (auto&& option : options) {
                if (option.read_only) {
                    continue;
                }
                ParamPtr param = nullptr;

                switch (option.type) {
                case sane_wrapper::SaneOptionType::TBool:
                    param = make_param<ParameterType::p_bool>(node, option.name, 0, {});
                    param->as<bool>().set_default(option.i_value);
                    break;
                case sane_wrapper::SaneOptionType::TInt:
                    if (option.size > sizeof(int32_t)) {
                        continue;
                    }
                    param = make_param<ParameterType::p_int>(node, option.name, 0, {});

                    if (option.constraint_type == sane_wrapper::SaneConstraint::IntList) {
                        param->tag("enum");
                        param->enum_values(option.int_value_list);
                        std::vector<std::string> enum_names;
                        for (auto&& i : option.int_value_list) {
                            enum_names.push_back(std::to_string(i));
                        }
                        param->enum_names(std::move(enum_names));
                    } else if (option.constraint_type == sane_wrapper::SaneConstraint::Range) {
                        param->tag("raw");
                        param->as<int>().min(option.i_minimum);
                        param->as<int>().max(option.i_maximum);
                        param->as<int>().set_default(option.i_value);
                    } else {
                        param->tag("raw");
                        param->as<int>().set_default(option.i_value);
                    }
                    break;
                case sane_wrapper::SaneOptionType::TString:
                    if (option.constraint_type == sane_wrapper::SaneConstraint::StringList) {
                        param = make_param<ParameterType::p_int>(node, option.name, 0, {"enum", "SaneTString"});
                        int i = 0;
                        for (auto&& entry : option.string_value_list) {
                            if (entry == option.s_value) {
                                param->as<int>().set_default(i);
                            }
                            i++;
                        }
                        param->enum_names(option.string_value_list);
                    } else {
                        param = make_param<ParameterType::p_string>(node, option.name, 0, {});
                        param->as<std::string>().set_default(option.s_value);
                    }
                    break;
                case sane_wrapper::SaneOptionType::TButton:
                    param = make_param<ParameterType::p_bool>(node, option.name, 0, {"button"});
                    //param->as<bool>().set_force(false);
                    break;
                default:
                    continue;
                }

                if (param) {
                    param->order(option.index + 100);
                    dynamic_params.emplace(option.index, param);
                }
            }

            return dynamic_params;
        }

        std::optional<UploadFormat> Service::format(int32_t device, const std::map<int32_t, ParamPtr>& params) {
            std::scoped_lock lock(_sane_mutex);
            return _format(device, params);
        }

        std::optional<UploadFormat> Service::_format(int32_t device, const std::map<int32_t, ParamPtr>& params) {
            auto devices = _devices();

            sane_wrapper::set_current_device(device, devices[device]);
            set_sane_options(params);

            try {
                SaneFormat format = sane_wrapper::format();

                if (format.width <= 0 || format.height <= 0) {
                    return std::nullopt;
                }

                auto in_format = ImageUploader::InFormat::rgb8;

                if (format.channels == sane::SaneChannels::RGB) {
                    if (format.depth == 8) {
                        in_format = ImageUploader::InFormat::rgb8;
                    } else if (format.depth == 16) {
                        in_format = ImageUploader::InFormat::rgb16;
                    }
                } else {
                    if (format.depth == 8) {
                        in_format = ImageUploader::InFormat::r8;
                    } else if (format.depth == 16) {
                        in_format = ImageUploader::InFormat::r16;
                    }
                }

                return UploadFormat{in_format, format.width, format.height};
            } catch (UpdateException&) {
            }
            return std::nullopt;
        }

        std::optional<std::unique_ptr<StaticHostImage>> Service::scan(int32_t device, const std::map<int32_t, ParamPtr>& params) {
            std::scoped_lock lock(_sane_mutex);

            auto devices = _devices();

            sane_wrapper::set_current_device(device, devices[device]);
            set_sane_options(params);

            auto sformat = _format(device, params);

            if (!sformat.has_value()) {
                return std::nullopt;
            }

            auto _format = *sformat;
            
            int32_t channels = 3;
            if (_format.format == ImageUploader::InFormat::r8 || _format.format == ImageUploader::InFormat::r16) {
                channels = 1;
            }
            int32_t pix_size = sizeof(uint8_t);
            if (_format.format == ImageUploader::InFormat::r16 || _format.format == ImageUploader::InFormat::rgb16) {
                pix_size = sizeof(uint16_t);
            }
            auto cr = StaticHostImage::make(_format.width, _format.height, channels, pix_size);
            sane_wrapper::read_image(cr->data(), cr->size());

            return {std::move(cr)};
        }

        void Service::set_sane_options(const std::map<int32_t, ParamPtr>& dynamic_params) {
            for (auto&& pair : dynamic_params) {
                try {
                    auto type = pair.second->type();
                    
                    switch (type) {
                    case ParameterType::p_bool:
                    {
                        if (pair.second->tags().find("button") != pair.second->tags().end() && pair.second->as<bool>().get() == false) {
                            continue;
                        }
                        int result = (int)pair.second->as<bool>().get();
                        sane_wrapper::set_option(pair.first, pair.second->name(), &result);
                        break;
                    }
                    case ParameterType::p_int:
                    {
                        if (pair.second->tags().find("SaneTString") != pair.second->tags().end()) {
                            const auto& names = pair.second->enum_names();
                            const auto& values = pair.second->enum_values();
                            int r = pair.second->as<int>().get();
                            if (values.size() > r) {
                                int result = values[r];
                                sane_wrapper::set_option(pair.first, pair.second->name(), &result);
                            } else if (names.size() > r) {
                                sane_wrapper::set_option(pair.first, pair.second->name(), names[r].c_str());
                            }
                        } else {
                            int result = pair.second->as<int>().get();
                            sane_wrapper::set_option(pair.first, pair.second->name(), &result);
                        }
                        break;
                    }
                    case ParameterType::p_string:
                    {
                        std::string result = pair.second->as<std::string>().get();
                        sane_wrapper::set_option(pair.first, pair.second->name(), result.c_str());
                    }
                    default:
                        continue;
                    }
                } catch (UpdateException& e) {
                    console << "Failed to set option on sane device: " << e.what() << std::endl;
                }
            }
        }

        Service& Service::Get() {
            std::scoped_lock lock(_mutex);
            if (!_singleton) {
                _singleton = std::make_unique<Service>();
            }
            return *_singleton;
        }

        void Service::Shutdown() {
            std::scoped_lock lock(_mutex);
            if (_singleton) {
                _singleton->shutdown();
                _singleton = nullptr;
            }
        }
    }
}