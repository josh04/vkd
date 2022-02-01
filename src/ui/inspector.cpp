#include "main_ui.hpp"
#include "inspector.hpp"
#include "image.hpp"
#include "device.hpp"

namespace vkd {

    void Inspector::draw(Device& d, Image& image_source, glm::ivec4 offset_w_h)
    {
        if (!_open) {
            return;
        }
        auto&& io = ImGui::GetIO();

        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 400, 100), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(300, 300), ImGuiCond_Once);
        ImGui::SetNextWindowCollapsed(false, ImGuiCond_FirstUseEver);

        ImGui::Begin("inspector", &_open);

        auto loc = io.MousePos;
        auto idim = (glm::ivec2)image_source.dim();

        if (io.MouseClicked[2]) {
            if (_current_parameter == nullptr) {
                _hold = !_hold;
            }
        }

        if (!_hold || _current_parameter) {
            glm::ivec2 floc = {loc.x - offset_w_h.x, loc.y - offset_w_h.y};

            floc.x = std::min(floc.x, offset_w_h.z);
            floc.y = std::min(floc.y, offset_w_h.w);

            floc.x = floc.x * (idim.x / (float)offset_w_h.z);
            floc.y = floc.y * (idim.y / (float)offset_w_h.w);

            _sample_loc = floc;
        }

        if (io.MouseClicked[2]) {
            if (_current_parameter != nullptr) {
                _current_parameter->set_force(_sample_loc);
                _current_parameter = nullptr;
            }
        }

        _pixel = image_source.sample<glm::vec4>(_sample_loc);
 
        ImGui::Text("loc x: %d y: %d w: %d h: %d", _sample_loc.x, _sample_loc.y, idim.x, idim.y);
        ImGui::ColorButton("inspector", ImVec4(_pixel.x, _pixel.y, _pixel.z, _pixel.w));
        const char * format_string = " r: %f\n g: %f\n b: %f\n a: %f\n %s";

        ImGui::Text(format_string, _pixel.x, _pixel.y, _pixel.z, _pixel.w, _current_parameter ? "(picking for parameter...)": (_hold ? "(locked)" : ""));

        ImGui::End();

    }

}