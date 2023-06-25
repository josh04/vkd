#include "main_ui.hpp"
#include "console_window.hpp"

namespace vkd {
        
    void ConsoleWindow::draw() {
        if (!_open) {
            return;
        }

        ImGui::SetNextWindowPos(ImVec2(40, 100), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(450, 300), ImGuiCond_Once);
        ImGui::SetNextWindowCollapsed(false, ImGuiCond_FirstUseEver);

        ImGui::Begin("console", &_open);

        ImGui::TextWrapped("%s", vkd::console.log().c_str());

        ImGui::End();

    }

}