#include "preferences.hpp"

#include <functional>

#include "platform_folders.h"

#include "ghc/filesystem.hpp"
#include <iostream>
#include <algorithm>

#include "cereal/cereal.hpp"

#include "imgui_drawer.hpp"

#include "ocio/ocio_static.hpp"
#include "ocio/ocio_functional.hpp"

#include "inputs/sane/sane_wrapper.hpp"

CEREAL_CLASS_VERSION(vkd::Preferences, 5);

namespace {
    std::string vkd_folder = "/vkd";
}

namespace vkd {    
    Preferences::Preferences() {
        _photo_project = vkd_folder + "/project.bin";
    }

    void Preferences::loaded() {

        bool success = ocio_functional::set_working_space(working_space());

        if (success) {
            console << "sucessfully set ocio working space" << std::endl;
        } else {
            console << "warning: failed to set ocio working space from prefs" << std::endl;
        }

        success = ocio_functional::set_default_input_space(default_input_space());

        if (success) {
            console << "sucessfully set ocio default input space" << std::endl;
        } else {
            console << "warning: failed to set ocio default input space from prefs" << std::endl;
        }

        success = ocio_functional::set_display_space(display_space());

        if (success) {
            console << "sucessfully set ocio display space" << std::endl;
        } else {
            console << "warning: failed to set ocio display space from prefs" << std::endl;
        }

        success = ocio_functional::set_scan_space(scan_space());

        if (success) {
            console << "sucessfully set ocio scan space" << std::endl;
        } else {
            console << "warning: failed to set ocio scan space from prefs" << std::endl;
        }

        success = ocio_functional::set_screenshot_space(screenshot_space());

        if (success) {
            console << "sucessfully set ocio screenshot space" << std::endl;
        } else {
            console << "warning: failed to set ocio screenshot space from prefs" << std::endl;
        }

        sane_wrapper::set_sane_library_location(sane_library());
    }

    namespace {
        void ocio_picker(std::string& space, const std::string& title, const std::vector<std::string>& names, std::function<void(std::string)> setter) {
            auto r = OcioStatic::GetOCIOConfig().index_from_space_name(space);

            if (r == -1) {
                r = 0;
            }

            r = std::clamp(r, 0, (int)names.size() - 1);

            if (ImGui::BeginCombo(title.c_str(), names[r].c_str())) {
                for (int i = 0; i < names.size(); ++i) {
                    auto&& name = names[i];
                    bool is_selected = (r == i);
                    if (ImGui::Selectable(name.c_str(), is_selected)) {
                        space = OcioStatic::GetOCIOConfig().space_name_at_index(i);
                        setter(space);
                    }
                    if (is_selected) {
                        ImGui::SetItemDefaultFocus(); 
                    }
                }
                ImGui::EndCombo();
            }
        }
    }

    void Preferences::draw() {
        if (!_open) {
            return;
        }

        ImGui::SetNextWindowPos(ImVec2(40, 100), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(450, 300), ImGuiCond_Once);
        ImGui::SetNextWindowCollapsed(false, ImGuiCond_FirstUseEver);

        ImGui::Begin("preferences", &_open);

        auto&& names = OcioStatic::GetOCIOConfig().space_names();

        ocio_picker(_working_space, "working space", names, &ocio_functional::set_working_space);
        ocio_picker(_default_input_space, "default input space", names, &ocio_functional::set_default_input_space);
        ocio_picker(_display_space, "display space", names, &ocio_functional::set_display_space);
        ocio_picker(_scan_space, "scanner space", names, &ocio_functional::set_scan_space);
        ocio_picker(_screenshot_space, "screenshot space", names, &ocio_functional::set_screenshot_space);
/* 
        {
            auto p = OcioStatic::GetOCIOConfig().index_from_space_name(_working_space);

            if (p == -1) {
                p = 0;
            }

            p = std::clamp(p, 0, (int)names.size() - 1);

            if (ImGui::BeginCombo("working space", names[p].c_str())) {
                for (int i = 0; i < names.size(); ++i) {
                    auto&& name = names[i];
                    bool is_selected = (p == i);
                    if (ImGui::Selectable(name.c_str(), is_selected)) {
                        _working_space = OcioStatic::GetOCIOConfig().space_name_at_index(i);
                        ocio_functional::set_working_space(_working_space);
                    }
                    if (is_selected) {
                        ImGui::SetItemDefaultFocus(); 
                    }
                }
                ImGui::EndCombo();
            }
        }

        {
            auto q = OcioStatic::GetOCIOConfig().index_from_space_name(_default_input_space);

            if (q == -1) {
                q = 0;
            }

            q = std::clamp(q, 0, (int)names.size() - 1);

            if (ImGui::BeginCombo("default input space", names[q].c_str())) {
                for (int i = 0; i < names.size(); ++i) {
                    auto&& name = names[i];
                    bool is_selected = (q == i);
                    if (ImGui::Selectable(name.c_str(), is_selected)) {
                        _default_input_space = OcioStatic::GetOCIOConfig().space_name_at_index(i);
                        ocio_functional::set_default_input_space(_default_input_space);
                    }
                    if (is_selected) {
                        ImGui::SetItemDefaultFocus(); 
                    }
                }
                ImGui::EndCombo();
            }
        }

        {
            auto r = OcioStatic::GetOCIOConfig().index_from_space_name(_display_space);

            if (r == -1) {
                r = 0;
            }

            r = std::clamp(r, 0, (int)names.size() - 1);

            if (ImGui::BeginCombo("display space", names[r].c_str())) {
                for (int i = 0; i < names.size(); ++i) {
                    auto&& name = names[i];
                    bool is_selected = (r == i);
                    if (ImGui::Selectable(name.c_str(), is_selected)) {
                        _display_space = OcioStatic::GetOCIOConfig().space_name_at_index(i);
                        ocio_functional::set_display_space(_display_space);
                    }
                    if (is_selected) {
                        ImGui::SetItemDefaultFocus(); 
                    }
                }
                ImGui::EndCombo();
            }
        } */

        {
            constexpr int strsize = 1024;
            char path[strsize];
            strncpy(path, _sane_library.c_str(), strsize);

            if (ImGui::InputText("sane library root path", path, strsize)) {
                _sane_library = path;
                sane_wrapper::set_sane_library_location(sane_library());
            }

        }

        ImGui::End();
    }

    std::string Preferences::Path() {
        ghc::filesystem::create_directory(sago::getConfigHome() + vkd_folder);
        return sago::getConfigHome() + vkd_folder+ "/settings.json";
    }
}