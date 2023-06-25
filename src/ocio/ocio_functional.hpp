#pragma once

#include <memory>
#include <map>

#include "engine_node.hpp"
#include "make_param.hpp"

namespace vkd {
    class Image;
    class Kernel;
    class ParameterInterface;
    namespace ocio_functional {
        std::shared_ptr<ParameterInterface> make_ocio_param(EngineNode& node, const std::string& name);
        std::shared_ptr<ParameterInterface> make_ocio_param(EngineNode& node, const std::string& name, int default_index);
        std::shared_ptr<Kernel> make_shader(EngineNode& node, const std::shared_ptr<Image>& inp, const std::shared_ptr<Image>& outp, int src_index, int dst_index, std::map<int, std::shared_ptr<Image>>& ocio_images);
        
        const std::string& working_space();
        int32_t working_space_index();
        const std::string& default_input_space();
        int32_t default_input_space_index();
        const std::string& display_space();
        int32_t display_space_index();
        const std::string& scan_space();
        int32_t scan_space_index();
        const std::string& screenshot_space();
        int32_t screenshot_space_index();

        bool set_working_space(const std::string& sp);
        bool set_default_input_space(const std::string& sp);
        bool set_display_space(const std::string& sp);
        bool set_scan_space(const std::string& sp);
        bool set_screenshot_space(const std::string& sp);
    }

    struct OcioParams {
        OcioParams() = delete;
        ~OcioParams() = default;

        // make_param(EngineNode& node, const std::string& name, size_t offset, std::set<std::string> tags = {})

        explicit OcioParams(EngineNode& node) {
            working_space = make_param<int32_t>(node, "vkd_ocio_working_space", 0, {});
            default_input_space = make_param<int32_t>(node, "vkd_ocio_default_input_space", 0, {});
            display_space = make_param<int32_t>(node, "vkd_ocio_display_space", 0, {});
            scan_space = make_param<int32_t>(node, "vkd_ocio_scan_space", 0, {});
            screenshot_space = make_param<int32_t>(node, "vkd_ocio_screenshot_space", 0, {});
            update();
        }

        void update() {
            if (working_space->as<int32_t>().get() != ocio_functional::working_space_index()) {
                working_space->as<int32_t>().set_force(ocio_functional::working_space_index());
            }
            if (default_input_space->as<int32_t>().get() != ocio_functional::default_input_space_index()) {
                default_input_space->as<int32_t>().set_force(ocio_functional::default_input_space_index());
            }
            if (display_space->as<int32_t>().get() != ocio_functional::display_space_index()) {
                display_space->as<int32_t>().set_force(ocio_functional::display_space_index());
            }
            if (scan_space->as<int32_t>().get() != ocio_functional::scan_space_index()) {
                scan_space->as<int32_t>().set_force(ocio_functional::scan_space_index());
            }
            if (screenshot_space->as<int32_t>().get() != ocio_functional::screenshot_space_index()) {
                screenshot_space->as<int32_t>().set_force(ocio_functional::screenshot_space_index());
            }
        }

        int32_t working_space_index() const { return working_space->as<int32_t>().get(); }
        int32_t default_input_space_index() const { return default_input_space->as<int32_t>().get(); }
        int32_t display_space_index() const { return display_space->as<int32_t>().get(); }
        int32_t scan_space_index() const { return scan_space->as<int32_t>().get(); }
        int32_t screenshot_space_index() const { return screenshot_space->as<int32_t>().get(); }
        
        std::shared_ptr<ParameterInterface> working_space = nullptr;
        std::shared_ptr<ParameterInterface> default_input_space = nullptr;
        std::shared_ptr<ParameterInterface> display_space = nullptr; 
        std::shared_ptr<ParameterInterface> scan_space = nullptr; 
        std::shared_ptr<ParameterInterface> screenshot_space = nullptr; 
    };

    using OcioParamsPtr = std::unique_ptr<OcioParams>;

    struct OcioNode {
        enum class Type {
            In,
            Out
        };
        OcioNode(Type type) : _type(type) {}
        ~OcioNode() = default;

        void init(EngineNode& node);
        void init(EngineNode& node, int32_t format);
        void update(EngineNode& node, std::shared_ptr<vkd::Image> image);
        void update(EngineNode& node, std::shared_ptr<vkd::Image> image_in, std::shared_ptr<vkd::Image> image_out);
        void execute(CommandBuffer& buf, int32_t width, int32_t height);
        
        Type _type = Type::In;
        std::shared_ptr<ParameterInterface> _use_ocio_param = nullptr;
        std::shared_ptr<ParameterInterface> _ocio_in_space = nullptr;
        std::map<int, std::shared_ptr<Image>> _ocio_images;        
        std::shared_ptr<Kernel> _ocio_in_transform = nullptr;
        OcioParamsPtr _ocio_params = nullptr;
    };
}