#pragma once
        
#include <memory>
#include <chrono>
#include <array>

#include "memory/memory_manager.hpp"

#include "glm/glm.hpp"

#include "parameter.hpp"

namespace vkd {
    class Device;
    class Image;
    class Inspector {
    public:
        Inspector() = default;
        ~Inspector() = default;
        Inspector(Inspector&&) = delete;
        Inspector(const Inspector&) = delete;

        void draw(Device& d, Image& image_source, glm::ivec4 offset_w_h);
        void open(bool set) { _open = set; }
        bool open() const { return _open; }
        void hold(bool set) { _hold = set; }
        bool hold() const { return _hold; }
        
        glm::vec4 pixel() const { return _pixel; }

        void set_current_parameter(const std::shared_ptr<Parameter<glm::ivec2>>& param) { _open = true; _current_parameter = param; }
        
        template<class Archive>
        void serialize(Archive& archive, const uint32_t version) {
            if (version == 0) {
                archive(_open, _hold, _pixel, _sample_loc);
            }
        }
    private:
        bool _open = false;

        bool _hold = false;
        glm::vec4 _pixel = {0.0, 0.0, 0.0, 0.0};

        glm::ivec2 _sample_loc = {0, 0};
        
        std::shared_ptr<Parameter<glm::ivec2>> _current_parameter = nullptr;
    };
}