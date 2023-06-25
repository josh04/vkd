#include "invert.hpp"
#include <random>
#include "command_buffer.hpp"
#include "device.hpp"
#include "pipeline.hpp"
#include "shader.hpp"
#include "vulkan.hpp"
#include "kernel.hpp"
#include "image.hpp"
#include "buffer.hpp"

namespace vkd {
    REGISTER_NODE("invert", "invert", Invert);

    namespace {

        static constexpr float default_min = -2.0f;
        static constexpr float default_max = 2.0f;
        std::vector<std::string> param_tags{"r", "g", "b"};

        void set_scale_param(Kernel& k, std::string prefix) {
            auto mn = k.get_param_by_name(prefix + "_min_point");
            mn->as<float>().set_default(0.0f);
            mn->as<float>().soft_min(default_min);
            mn->as<float>().soft_max(default_max);
            auto mx = k.get_param_by_name(prefix + "_max_point");
            mx->as<float>().set_default(1.0f);
            mx->as<float>().soft_min(default_min);
            mx->as<float>().soft_max(default_max);
        }

        void clamp_scale_param(Kernel& k, std::string prefix, float gmin, float gmax) {
            auto mn = k.get_param_by_name(prefix + "_min_point");
            float mnval = mn->as<float>().get();
            auto mx = k.get_param_by_name(prefix + "_max_point");
            float mxval = mx->as<float>().get();

            if (mxval < mnval) { mxval = mnval + 1e-8f; }

            if (mnval < gmin) { mnval = gmin; }
            if (mnval > gmax) { mnval = gmax; }
            mn->as<float>().set(mnval);
            if (mxval < gmin) { mxval = gmin; }
            if (mxval > gmax) { mxval = gmax; }
            mx->as<float>().set(mxval);
        }
    }

    void Invert::_check_param_sanity()
    {
        float gmin = _min_point->as<float>().get();
        float gmax = _max_point->as<float>().get();

        if (gmin >= gmax) {
            gmax = gmin + 1e-8f;
        }
        _max_point->as<float>().set(gmax);

        for (auto&& param : param_tags) {
            clamp_scale_param(*_invert, param, gmin, gmax);
        }
    }

    void Invert::post_setup() {
        _min_point = make_param<float>(param_hash_name(), "global_min_point", 0);
        _min_point->as<float>().set_default(0.0);
        _min_point->as<float>().soft_min(default_min);
        _min_point->as<float>().soft_max(default_max);

        _max_point = make_param<float>(param_hash_name(), "global_max_point", 0);
        _max_point->as<float>().set_default(1.0);
        _max_point->as<float>().soft_min(default_min);
        _max_point->as<float>().soft_max(default_max);

        _avg_margin = make_param<glm::vec4>(*this, "avg margin padding (percent)", 0);
        _avg_margin->as<glm::vec4>().set_default(glm::vec4{15.0, 15.0, 15.0, 15.0});
        _avg_margin->as<glm::vec4>().soft_min(glm::vec4{0.0, 0.0, 0.0, 0.0});
        _avg_margin->as<glm::vec4>().soft_max(glm::vec4{95.0, 95.0, 95.0, 95.0});
        
        _params["_"].emplace(_min_point->name(), _min_point);
        _params["_"].emplace(_max_point->name(), _max_point);
    }

    void Invert::init() {
        

        _size = {0, 0};
        
        _invert = Kernel::make(*this, "shaders/compute/invert.comp.spv", "main", Kernel::default_local_sizes);
        _avg = Kernel::make(*this, "shaders/compute/averages.comp.spv", "main", Kernel::default_local_sizes);
        _avg_clone = Kernel::make(*this, "shaders/compute/averages_clone_channel.comp.spv", "main", Kernel::default_local_sizes);

        _run_averages = make_param<ParameterType::p_bool>(*this, "run averages", 0, {"button"});
        
        _hmean_param = make_param<glm::vec4>(*this, "calculated hmean", 0, {"label", "rgba"});
        _min_param = make_param<glm::vec4>(*this, "calculated min", 0, {"label", "rgba"});
        _max_param = make_param<glm::vec4>(*this, "calculated max", 0, {"label", "rgba"});
        _mean_param = make_param<glm::vec4>(*this, "calculated mean", 0, {"label", "rgba"});

        auto avg_scale = _avg->get_param_by_name("_scale");
        avg_scale->as<int>().set_force(2);

        auto max_threshold = _avg->get_param_by_name("max_threshold");
        max_threshold->as<float>().set_default(3.0f);
        max_threshold->as<float>().min(0.0001f);
        max_threshold->as<float>().max(10.0f);

        for (auto&& param : param_tags) {
            set_scale_param(*_invert, param);
        }

        
        auto image = _image_node->get_output_image();
        if (!image) {
            throw GraphException("No image received at Invert node");
        }
        _size = image->dim();

        _image = vkd::Image::float_image(_device, _size);

        _avg_temp_image_a = vkd::Image::float_image(_device, _size, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
        _avg_temp_image_b = vkd::Image::float_image(_device, _size / glm::uvec2(2), VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

        _invert->set_arg(0, image);
        _invert->set_arg(1, _image);

        _fence = Fence::create(_device, false);

    }

    void Invert::allocate(VkCommandBuffer buf) {
        _image->allocate(buf);
        _avg_temp_image_a->allocate(buf);
        _avg_temp_image_b->allocate(buf);
    }

    void Invert::deallocate() { 
        _image->deallocate();
        _avg_temp_image_a->deallocate();
        _avg_temp_image_b->deallocate();
    }

    bool Invert::update(ExecutionType type) {
        bool update = false;

        for (auto&& pmap : _params) {
            for (auto&& el : pmap.second) {
                if (el.second->changed()) {
                    update = true;
                }
            }
        }

        if (update) {
            if (_run_averages->as<bool>().get()) {
                _run_averages->as<bool>().set(false);
                _run_calculate_averages = true;
            }
            _check_param_sanity();
        }

        return update;
    }

    void Invert::_calculate_averages() {

        glm::vec4 _hmean = {0.0f, 0.0f, 0.0f, 0.0f};
        glm::vec4 _min = {0.0f, 0.0f, 0.0f, 0.0f};
        glm::vec4 _max = {0.0f, 0.0f, 0.0f, 0.0f};
        glm::vec4 _mean = {0.0f, 0.0f, 0.0f, 0.0f};

        for (int i = 0; i < 3; ++i) {
            

            auto avg_margin = _avg_margin->as<glm::vec4>().get();

            glm::uvec2 offs = {_size.x * (avg_margin.x / 100.0f), _size.y * (avg_margin.y / 100.0f)};
            glm::uvec2 size_margins = {_size.x * (100.0f - avg_margin.x - avg_margin.z) / 100.0f, _size.y * (100.0f - avg_margin.y - avg_margin.w) / 100.0f};
            auto size_next = size_margins;

            auto image_source = _avg_temp_image_a;
            auto image_dest = _avg_temp_image_b;

            auto image = _image_node->get_output_image();

            _avg_clone->set_arg(0, image);
            _avg_clone->set_arg(1, _avg_temp_image_a);
            _avg_clone->set_push_arg_by_name("_channel", i);
            _avg_clone->set_push_arg_by_name("_offset", offs);

            auto buf = vkd::begin_immediate_command_buffer(_device->logical_device(), _device->command_pool());
            _avg_clone->dispatch(buf, size_margins.x, size_margins.y);        
            vkd::flush_command_buffer(_device->logical_device(), _device->queue(), _device->command_pool(), buf);

            while (size_next.x > 1 && size_next.y > 1) {
                _avg->set_arg(0, image_source);
                _avg->set_arg(1, image_dest);
                
                auto buf = vkd::begin_immediate_command_buffer(_device->logical_device(), _device->command_pool());
                _avg->dispatch(buf, size_next.x, size_next.y);
                vkd::flush_command_buffer(_device->logical_device(), _device->queue(), _device->command_pool(), buf);

                auto itemp = image_dest;
                image_dest = image_source;
                image_source = itemp;
                
                size_next = size_next / glm::uvec2(2);
            }

            glm::vec4 dl2 = image_source->sample<glm::vec4>({0, 0});

            _hmean[i] = dl2.x;
            _max[i] = dl2.y;
            _min[i] = dl2.z;
            _mean[i] = dl2.w / (double)(size_margins.x * size_margins.y);

            //console << i << " hmean: " << dl2.x << " max: " << dl2.y << " min: " << dl2.z << " avg: " << dl2.w / (double)(_size.x * _size.y) << std::endl;
        }

        _hmean_param->as<glm::vec4>().set_force(_hmean);
        _min_param->as<glm::vec4>().set_force(_min);
        _max_param->as<glm::vec4>().set_force(_max);
        _mean_param->as<glm::vec4>().set_force(_mean);

        int i = 0;
        for (auto&& prefix : param_tags) {
            auto mn = _invert->get_param_by_name(prefix + "_min_point");
            mn->as<float>().set_force(_min[i]);
            auto mx = _invert->get_param_by_name(prefix + "_max_point");
            mx->as<float>().set_force(_max[i]);

            if (_max_point->as<float>().get() < _max[i]) {
                _max_point->as<float>().set_force(_max[i]);
            }
            if (_min_point->as<float>().get() > _min[i]) {
                _min_point->as<float>().set_force(_min[i]);
            }

            i++;
        }

    }

    void Invert::execute(ExecutionType type, Stream& stream) {
        command_buffer().begin();
        _invert->dispatch(command_buffer(), _size.x, _size.y);
        command_buffer().end();

        _fence->submit();
        stream.submit(command_buffer());
        
        _fence->wait();
        _fence->reset();

        if (_run_calculate_averages) {
            _calculate_averages();
            _run_calculate_averages = false;
        }
    }

    void Invert::post_execute(ExecutionType type) {
    }
}